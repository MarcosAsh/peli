# peli vs PyAV: preliminary benchmark

Run on a single machine: Linux, FFmpeg 8.0.1, gcc 15.2.1, Python 3.14.2, peli v0.1.0, PyAV 14.x.

| clip                              | frames | backend | sequential fps | random fps |
| --------------------------------- | -----: | ------- | -------------: | ---------: |
| Javelin_standing_throw_drill.mkv  |    303 | peli    |        **616** |     **22** |
| Javelin_standing_throw_drill.mkv  |    303 | pyav    |            178 |         16 |
| flipping_a_pancake.mkv            |    310 | peli    |       **7335** |         78 |
| flipping_a_pancake.mkv            |    310 | pyav    |           1082 |    **264** |
| count.mov                         |    143 | peli    |       **1525** |     **47** |
| count.mov                         |    143 | pyav    |            211 |         40 |

## Headline

**Sequential decode: peli is 3.5×–6.8× faster than PyAV** on this corpus.

## Random access caveat

PyAV has no indexed-frame API. The "random fps" column for PyAV is implemented as: sort the requested indices, walk the stream once, pick out the requested frames as we hit them. That amortizes the seek cost over the full stream walk, which is why PyAV looks competitive on small clips here.

`peli` does accurate per-index seek (`vr.seek_accurate(i); vr.next()`), which always pays per-frame seek-and-decode cost regardless of stream length.

The difference matters at scale: for a short clip (300 frames), walking the stream is cheap. For a long clip (10k+ frames), walking-and-filtering for a random subset becomes prohibitive while accurate-seek stays constant per-index. **The current example corpus is too short to show this**; codec-coverage work (task #11) will add long clips that exercise the architectural difference properly.

## What this benchmark does NOT include

- **decord**: won't build against FFmpeg 5+ on Python 3.14, which is the entire reason peli exists. Not testable on this machine.
- **torchcodec**: requires `torch` (~700 MB install). Skipped in this run; supported in `run_benchmarks.py` if torch is installed.
- **OpenCV**: skipped for now; opencv's VideoCapture is known-slow for random access and would inflate peli's numbers spuriously.
- **GPU decode**: v0.2 work.

## Reproduce

```bash
pip install peli av
python tests/benchmark/run_benchmarks.py --random-frames 20 --repeats 3
```
