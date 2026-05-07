# Contributing to peli

Thanks for taking a look. peli is a small project. The contribution loop is fast and the surface is bounded; most patches land in a single round of review.

## Quick start

```bash
git clone --recursive https://github.com/MarcosAsh/peli.git
cd peli
# install system FFmpeg dev headers (see README "Install" section for your platform)
python -m pip install -e . --no-build-isolation
python -m pip install pytest
python -m pytest tests/python/unittests/ -v
```

If `pip install -e .` works and the test suite is green, you have a working dev environment.

## Project scope

peli decodes video. That's the whole job. In rough order of what's interesting to land:

- **Decoder correctness**: handling specific codecs, containers, broken-but-recoverable files, frame-rate edge cases, color spaces.
- **Performance**: faster random access, smarter prefetch, lower memory.
- **Backend interop**: improvements to the DLPack output path, framework-specific zero-copy gotchas.
- **Build / packaging**: cross-platform wheel issues, FFmpeg version handling, install ergonomics.
- **Docs and examples**: real-world recipes (training pipeline integration, HF datasets, etc.).

Out of scope:
- Encoding (peli only decodes).
- Frame transformations beyond what FFmpeg's `swscale` already does (resize, color convert). For augmentation, use the framework you're feeding into.
- Generic ML utilities. peli is the decoder, not the dataloader.

## How to test

### CPU tests

```bash
python -m pytest tests/python/unittests/test_video_reader.py tests/python/unittests/test_peli_api.py -v
```

These are the inherited decord parity tests plus peli's `output=` and DLPack tests. They run against the example clips in `examples/`.

### GPU tests

GPU tests are gated behind `pytest.mark.skipif(not torch.cuda.is_available())` and live in `tests/python/unittests/test_gpu_decode.py`. To run them:

- **Locally** (if you have an NVIDIA GPU + CUDA + a peli build with `USE_CUDA=ON`):

  ```bash
  cmake -S . -B build -DUSE_CUDA=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j
  PYTHONPATH=python PELI_LIBRARY_PATH=build python -m pytest tests/python/unittests/test_gpu_decode.py -v
  ```

- **Via Modal** (no local GPU needed): the `gpu-tests` GitHub Actions workflow runs them on an L4 via Modal. Trigger via the Actions tab. If you're a contributor without write access to the workflow secrets, ask a maintainer to run it on your PR.

### Benchmarks

```bash
pip install av  # PyAV; torchcodec is optional
python tests/benchmark/run_benchmarks.py --random-frames 20 --repeats 3
```

Results land in `tests/benchmark/results.md`. If you're claiming a perf change in a PR, attach a before/after snippet.

## Code style

- **C++**: matches the inherited decord style. New code should not introduce `using namespace std;` or `auto` for FFmpeg types where the explicit type aids review. Keep public headers in `include/peli/`, internal headers next to the `.cc` files.
- **Python**: 4-space indent, type hints encouraged but not required, no f-string-only formatting in error messages that need to survive without f-strings (the FFI layer is sensitive to this).
- **Comments**: write them when the *why* is non-obvious. Don't restate the code. Don't reference current PRs, issues, or roadmap items in code comments — those rot.
- **Tests**: every fix should add or update a test. Bug-fix PRs without a regression test will be asked for one.

## FFmpeg version policy

peli supports FFmpeg 5+ for now. Don't add unconditional uses of FFmpeg 6+ APIs (e.g., `av_packet_side_data_get`) without a version-gated fallback. The CMake build pulls FFmpeg version macros via `<libavutil/version.h>` if you need to gate something.

## Commit messages

One-line subject in present tense, no period, lowercase first word unless a proper noun. Body optional, used for the *why* if non-obvious. Examples from the existing log:

- `peli v0.1`
- `nvdec dlpack -> cuda torch.tensor verified zero-copy on L4`
- `build ffmpeg 7.1 from source in linux wheels; bump macos deploy target`

No co-author tags or AI tool attribution.

## Pull requests

1. Fork or create a branch off `main`.
2. Keep the PR focused. One conceptual change per PR.
3. Push. CI runs automatically (`ci`, `wheels` on push to PRs targeting `main`). For changes touching the CUDA path, also trigger `gpu-tests` if you have access; otherwise note in the PR description so a maintainer can.
4. PR title should be a usable commit subject (we squash-merge by default).
5. Link the issue if one exists.

## Reporting bugs

Open a GitHub issue with:
- The failing input (a short clip, or the URL of one), or a description if the clip is private.
- The exact `peli.VideoReader(...)` call that fails.
- The traceback or error output.
- Output of `python -c "import peli; print(peli.__version__)"` and `ffmpeg -version | head -1`.

Reproducible repros get fixed faster.

## Security

If you find a memory-safety issue (UAF, OOB read in the C++ decoder), please **don't** open a public issue. Email the maintainer or use GitHub's private security reporting if enabled on the repo.

## License

peli is licensed under [Apache-2.0](LICENSE), inheriting from the upstream [dmlc/decord](https://github.com/dmlc/decord). Contributions are accepted under the same license. By submitting a PR you agree your work is licensed under Apache-2.0.

## Questions

Open a GitHub Discussion or an issue tagged `question`. Email is fine for sensitive things.
