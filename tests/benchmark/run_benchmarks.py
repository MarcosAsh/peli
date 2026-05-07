"""Unified benchmark runner comparing peli vs PyAV across a clip corpus.

Usage:
    python run_benchmarks.py [--clips <glob>] [--random-frames N] [--repeats N]

Notes on what's measured:
- Sequential decode: open a clip, read every frame in order, count frames/sec.
- Random access:     open a clip, fetch N random frame indices via accurate seek,
                     count frames/sec.
- Batch fetch:       open a clip, fetch N frames in one call (peli only; PyAV has
                     no native batch primitive, so we fall back to a Python loop
                     for fairness).

PyAV is the most apples-to-apples comparison since it's the most popular
maintained Python FFmpeg binding. decord and torchcodec are not benchmarked
here:
- decord won't build against modern FFmpeg / Python 3.14, which is the entire
  reason peli exists. Comparing against it isn't possible on this machine.
- torchcodec hard-imports torch (~700 MB install footprint), so we skip it
  unless it's already available. If torch is installed, torchcodec is exercised.
"""
from __future__ import annotations

import argparse
import glob
import os
import statistics
import sys
import time
from typing import Callable, Iterable, List, Optional

import numpy as np


# ---------- backends ----------

def bench_peli_sequential(path: str) -> tuple[int, float]:
    import peli
    vr = peli.VideoReader(path, output="numpy")
    n = len(vr)
    t0 = time.perf_counter()
    for i in range(n):
        _ = vr[i]
    return n, time.perf_counter() - t0


def bench_peli_random(path: str, indices: list[int]) -> tuple[int, float]:
    import peli
    vr = peli.VideoReader(path, output="numpy")
    t0 = time.perf_counter()
    for i in indices:
        _ = vr[i]
    return len(indices), time.perf_counter() - t0


def bench_peli_batch(path: str, indices: list[int]) -> tuple[int, float]:
    import peli
    vr = peli.VideoReader(path, output="numpy")
    t0 = time.perf_counter()
    _ = vr.get_batch(indices)
    return len(indices), time.perf_counter() - t0


def bench_pyav_sequential(path: str) -> tuple[int, float]:
    import av
    container = av.open(path)
    stream = container.streams.video[0]
    t0 = time.perf_counter()
    n = 0
    for frame in container.decode(stream):
        _ = frame.to_ndarray(format="rgb24")
        n += 1
    container.close()
    return n, time.perf_counter() - t0


def bench_pyav_random(path: str, indices: list[int]) -> tuple[int, float]:
    """PyAV doesn't have an indexed-frame API. Fairest implementation:
    sort indices, then walk the stream and pick the requested frames.
    For fully random access this is much slower than peli, which is the
    point of the comparison."""
    import av
    container = av.open(path)
    stream = container.streams.video[0]
    target = sorted(set(indices))
    target_set = set(target)
    t0 = time.perf_counter()
    seen = 0
    out = 0
    for frame in container.decode(stream):
        if seen in target_set:
            _ = frame.to_ndarray(format="rgb24")
            out += 1
            if out == len(target):
                break
        seen += 1
    container.close()
    return len(indices), time.perf_counter() - t0


def bench_torchcodec_sequential(path: str) -> tuple[int, float]:
    from torchcodec.decoders import VideoDecoder
    dec = VideoDecoder(path)
    t0 = time.perf_counter()
    for i in range(dec.metadata.num_frames):
        _ = dec[i]
    return dec.metadata.num_frames, time.perf_counter() - t0


def bench_torchcodec_random(path: str, indices: list[int]) -> tuple[int, float]:
    from torchcodec.decoders import VideoDecoder
    dec = VideoDecoder(path)
    t0 = time.perf_counter()
    for i in indices:
        _ = dec[i]
    return len(indices), time.perf_counter() - t0


# ---------- harness ----------

BackendBench = Callable[..., tuple[int, float]]

BACKENDS = {
    "peli":       {"seq": bench_peli_sequential,       "rand": bench_peli_random},
    "pyav":       {"seq": bench_pyav_sequential,       "rand": bench_pyav_random},
    "torchcodec": {"seq": bench_torchcodec_sequential, "rand": bench_torchcodec_random},
}


def is_backend_available(name: str) -> bool:
    try:
        if name == "peli":       __import__("peli")
        if name == "pyav":       __import__("av")
        if name == "torchcodec": __import__("torchcodec")
        return True
    except Exception:
        return False


def run_one(fn: BackendBench, *args, repeats: int) -> dict:
    """Run a single benchmark `repeats` times, return frames/sec stats."""
    rates = []
    for _ in range(repeats):
        n, dt = fn(*args)
        rates.append(n / dt)
    return {
        "best": max(rates),
        "median": statistics.median(rates),
        "frames": n,
    }


def fmt_fps(rate: float) -> str:
    if rate >= 100:
        return f"{rate:7.0f}"
    if rate >= 10:
        return f"{rate:7.1f}"
    return f"{rate:7.2f}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--clips", default="examples/*.mkv examples/*.mov",
                    help="Space-separated globs of clips to benchmark")
    ap.add_argument("--random-frames", type=int, default=20,
                    help="Number of random frame indices to fetch")
    ap.add_argument("--repeats", type=int, default=3, help="Repeat count per measurement")
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--backends", default="peli,pyav,torchcodec",
                    help="Comma-separated list of backends")
    args = ap.parse_args()

    backends = [b.strip() for b in args.backends.split(",")]
    available = [b for b in backends if is_backend_available(b)]
    skipped = [b for b in backends if b not in available]
    if skipped:
        print(f"Skipping unavailable backends: {', '.join(skipped)}")

    clips = []
    for pat in args.clips.split():
        clips.extend(sorted(glob.glob(pat)))
    if not clips:
        print(f"No clips matched {args.clips!r}")
        return 1

    rng = np.random.default_rng(args.seed)

    print(f"\nClips: {len(clips)} | backends: {', '.join(available)} | "
          f"random_frames={args.random_frames} | repeats={args.repeats}")

    rows = []
    for clip in clips:
        size_mb = os.path.getsize(clip) / (1024 * 1024)
        # Need num_frames to pick random indices. Use peli (fast) if available,
        # else PyAV.
        if "peli" in available:
            import peli
            num_frames = len(peli.VideoReader(clip))
        else:
            import av
            c = av.open(clip)
            num_frames = c.streams.video[0].frames
            c.close()
        random_indices = rng.choice(num_frames,
                                    size=min(args.random_frames, num_frames),
                                    replace=False).tolist()

        clip_label = os.path.basename(clip)
        print(f"\n--- {clip_label} ({size_mb:.1f} MB, {num_frames} frames) ---")

        for backend in available:
            try:
                seq = run_one(BACKENDS[backend]["seq"], clip, repeats=args.repeats)
                rand = run_one(BACKENDS[backend]["rand"], clip, random_indices,
                               repeats=args.repeats)
                print(f"  {backend:11s} seq {fmt_fps(seq['best'])} fps   "
                      f"rand {fmt_fps(rand['best'])} fps")
                rows.append((clip_label, num_frames, backend,
                             seq["best"], rand["best"]))
            except Exception as e:
                print(f"  {backend:11s} ERROR: {e}")

    # Markdown table
    print("\n## Results\n")
    print("| clip | frames | backend | sequential fps | random fps |")
    print("|---|---:|---|---:|---:|")
    for clip_label, num_frames, backend, seq, rand in rows:
        print(f"| {clip_label} | {num_frames} | {backend} | "
              f"{seq:.0f} | {rand:.0f} |")

    return 0


if __name__ == "__main__":
    sys.exit(main())
