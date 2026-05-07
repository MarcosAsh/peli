# Contributing

## Setup

```bash
git clone --recursive https://github.com/MarcosAsh/peli
cd peli
pip install -e . --no-build-isolation
pip install pytest
pytest tests/python/unittests/ -v
```

## Tests

CPU tests run from the example clips in `examples/`. GPU tests live in `tests/python/unittests/test_gpu_decode.py` and skip themselves when CUDA isn't available; CI runs them on an L4 via Modal.

For wider codec coverage:

```bash
bash tests/data/download_corpus.sh
pytest tests/python/unittests/test_codec_coverage.py -v
```

## Style

C++: same style as the surrounding code. Public headers in `include/peli/`, internal headers next to the `.cc` file. No `using namespace std`.

Python: 4-space, type hints where they help. The FFI layer in `peli/_ffi/` is sensitive — read `_ffi/function.py` before touching it.

Comments: only for the non-obvious *why*. Don't restate the code, don't reference issue numbers in comments.

## FFmpeg compat

Minimum FFmpeg 5. If you reach for a 6+ API, gate it on `LIBAVCODEC_VERSION_INT`.

## PRs

One conceptual change per PR. CI is `ci`, `wheels`, `gpu-tests`. Squash-merge on green.

Commit subjects are short, imperative, lowercase. No co-author lines.

## License

Apache-2.0, inherited from upstream [decord](https://github.com/dmlc/decord). Submitting a patch is consent to that license.
