# Using peli with 🤗 Datasets

## Status (as of 2026-05)

HuggingFace `datasets` uses **torchcodec exclusively** for video. The `Video` feature in `src/datasets/features/video.py` returns a `torchcodec.decoders.VideoDecoder` directly. There is no backend plugin system, no `VIDEO_BACKEND` registry, no environment variable to swap in another decoder.

That means there are two integration paths, with very different effort profiles.

## Path 1: use peli alongside `datasets` today (no PR)

Cast the column to return raw bytes instead of a decoded video, then build a peli `VideoReader` from those bytes. Works on every released version of `datasets`.

```python
from datasets import load_dataset, Video
import io
import peli

# Tell datasets *not* to decode; return bytes.
ds = load_dataset("some/video-dataset")
ds = ds.cast_column("video", Video(decode=False))

def to_peli(example, output="numpy"):
    raw = example["video"]["bytes"]
    if raw is None:
        # Some datasets store path-only; fall back to disk
        return peli.VideoReader(example["video"]["path"], output=output)
    return peli.VideoReader(io.BytesIO(raw), output=output)

ds = ds.map(lambda ex: {"vr": to_peli(ex, output="torch")})

# Now ds["vr"] is a peli.VideoReader; vr[i] is a torch.Tensor.
```

Pros: zero upstream changes, works immediately, lets users opt in.
Cons: not the default; users have to know about peli first.

We should ship this recipe in our docs as **the** way to use peli with `datasets` until path 2 lands.

## Path 2: upstream `datasets` to support peli

Three options, in increasing order of invasiveness:

### Option A — sibling `VideoPeli` feature

Add a new `Video` subclass, e.g. `VideoPeli`, that mirrors the existing `Video` API but constructs `peli.VideoReader` instead of `torchcodec.decoders.VideoDecoder`. Existing code unchanged.

```python
# src/datasets/features/video_peli.py  (new file)
from .video import Video

@dataclass
class VideoPeli(Video):
    output: Literal["numpy", "torch", "jax", "tf", "keras"] = "numpy"

    def decode_example(self, value, ...):
        import io, peli
        raw = value.get("bytes")
        path = value.get("path")
        src = io.BytesIO(raw) if raw is not None else path
        return peli.VideoReader(src, output=self.output)
```

Smallest patch. ~40 lines + tests. Easiest to get merged because it doesn't refactor anything that exists. Downside: documentation gets fragmented (now there's `Video` and `VideoPeli`).

### Option B — backend kwarg on existing `Video`

Add `backend: Literal["torchcodec", "peli"] = "torchcodec"` to `Video.__init__`. `decode_example` dispatches.

```python
@dataclass
class Video:
    backend: Literal["torchcodec", "peli"] = "torchcodec"
    # ... rest unchanged ...

    def decode_example(self, value, ...):
        if self.backend == "torchcodec":
            return self._decode_torchcodec(value, ...)
        elif self.backend == "peli":
            return self._decode_peli(value, ...)
```

More changes to the existing class. Cleaner UX. ~80 lines + tests.

### Option C — full plugin system

Refactor `Video` to dispatch through a registry, with torchcodec and peli as the first two registered backends, and an entry-points mechanism for third-party backends. ~300 lines, big design discussion required, probably not worth chasing for v0.1.

## Recommendation

For v0.1: **ship Path 1** (the bytes recipe in our docs).

For v0.2 once peli has install numbers: **open Option A as a draft PR** to `huggingface/datasets`. It's the smallest, lowest-risk change that gets peli into the ecosystem. Pre-empt the "why not torchcodec" question with concrete pitch lines:

- peli is framework-neutral via DLPack (JAX/TF/Keras users currently have no good `datasets` story)
- peli is a maintained successor to decord, which `datasets` users still ask for

## Concrete PR checklist (Option A)

- [ ] Add `peli` as an optional dependency: `pip install datasets[peli]`
- [ ] `src/datasets/features/video_peli.py`: new file, `VideoPeli` class
- [ ] `src/datasets/features/__init__.py`: export `VideoPeli`
- [ ] `src/datasets/config.py`: add `PELI_AVAILABLE = importlib.util.find_spec("peli") is not None`
- [ ] `tests/features/test_video_peli.py`: parity tests against the existing torchcodec ones
- [ ] `docs/source/about_dataset_features.mdx`: short paragraph + example
- [ ] PR body: pitch why peli exists, link to benchmarks, link to peli repo
- [ ] Mention `@lhoestq` and other recent video-feature contributors

## Alternatives if datasets won't merge

If the HF maintainers push back, the next-most-leveraged integration is a **standalone helper package**: `peli-datasets` or `peli.integrations.datasets` inside peli itself. Provides the bytes-recipe glue out of the box. Less reach than upstream, but no gatekeeper.
