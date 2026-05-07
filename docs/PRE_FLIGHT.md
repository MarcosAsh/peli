# peli v0.1 pre-flight checklist

Items that require your action (PyPI account, GitHub access, hardware), in order.
Each step gates the next.

## 1. Reserve `peli` on PyPI before you tell anyone about the project

PyPI search shows the literal name `peli` is currently unclaimed. Squat-prevention reservation is critical — once peli is mentioned anywhere public, someone can register the name first.

```bash
# Create an account at https://pypi.org/account/register/
# Then install build tooling and reserve a placeholder
pip install build twine

# In a scratch directory, NOT this repo:
mkdir /tmp/peli-reserve && cd /tmp/peli-reserve
cat > pyproject.toml <<'EOF'
[build-system]
requires = ["setuptools"]
build-backend = "setuptools.build_meta"

[project]
name = "peli"
version = "0.0.0"
description = "Reserved. Real release coming soon."
readme = "README.md"
requires-python = ">=3.10"
EOF
echo "Reserved by <your name>. Real release coming soon at https://github.com/<your-handle>/peli" > README.md
python -m build
twine upload dist/*
```

The placeholder `0.0.0` will be replaced by the real `0.1.0` once CI publishes. PyPI lets you upload higher versions any time.

## 2. Create the GitHub repo

```bash
# Public, with issues enabled. Description:
# "Backend-agnostic video decoder for ML training pipelines. Modernized fork of dmlc/decord."

gh repo create <your-handle>/peli --public --description "..." --license=Apache-2.0
```

## 3. Replace placeholder URLs in this repo

Search for `<peli-repo-url>` and `<your-handle>` in `README.md` and `pyproject.toml` and replace with your real GitHub URL.

```bash
grep -rln '<peli-repo-url>\|<your-handle>' README.md pyproject.toml docs/
# Manually edit each; should be ~3 places
```

In `pyproject.toml`, also uncomment `Homepage` and `Issues` URLs under `[project.urls]`.

## 4. First push and CI shakedown

```bash
git remote add origin https://github.com/<your-handle>/peli.git
git push -u origin main
```

`ci.yml` will run on this push (Linux + macOS, py 3.10/3.12/3.13 build + smoke). Expect 1-3 small breakages:

- **manylinux ffmpeg-devel availability**: `ci.yml` uses native distro FFmpeg, not manylinux. Should work on `ubuntu-latest` (uses 24.04 with FFmpeg 6+) and `macos-14`. Verify.
- **Python 3.13 wheel availability for transitive deps**: numpy 2.x wheels exist for 3.13, fine. PyAV (only used in benchmarks, not in main install) may or may not have 3.13 wheels; not a blocker for `ci.yml`.
- **Submodule recursion**: `actions/checkout` with `submodules: recursive` is in both workflows; should pull dlpack and dmlc-core.

Do not move on until `ci.yml` is green on `main`.

## 5. Configure PyPI Trusted Publisher

This is the modern way (no API tokens stored anywhere). One-time setup.

1. On PyPI, go to your `peli` project page → Settings → Publishing → "Add a new pending publisher"
2. Fill in: owner `<your-handle>`, repository `peli`, workflow `wheels.yml`, environment `pypi`.
3. In the GitHub repo: Settings → Environments → New environment named `pypi`. Add a "required reviewers" rule pointing to yourself for safety.
4. Edit `.github/workflows/wheels.yml`: uncomment the `publish` job at the bottom.

## 6. Cut the first release

```bash
# Make sure the working tree is clean and CI is green
git tag v0.1.0
git push origin v0.1.0
```

`wheels.yml` will:
- Build wheels for Linux x86_64 + aarch64, macOS x86_64 + arm64, Windows AMD64
- Build an sdist
- Upload everything to PyPI under your trusted-publisher config

This is the moment `pip install peli` becomes real.

Watch the run in Actions. **Realistic first-run failures, in rough order of likelihood**:

| Likely failure                                                                            | Fix                                                                          |
| ----------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------- |
| Linux: ffmpeg-devel package not in default `manylinux_2_28` repos                         | Add EPEL+RPMFusion to `before-all` (already configured but untested)         |
| macOS: `MACOSX_DEPLOYMENT_TARGET` mismatch with Homebrew FFmpeg                           | Bump deployment target to 12.0 or build FFmpeg from source                   |
| Windows: vcpkg + scikit-build-core cmake toolchain handoff                                | Set `CMAKE_TOOLCHAIN_FILE` env var explicitly per Python version             |
| auditwheel complains about missing manylinux platform tag                                 | Add `--plat manylinux_2_28_x86_64` to `repair-wheel-command`                 |
| Smoke test fails on a wheel because `examples/count.mov` isn't packaged                   | Add `tests/data/` to wheel via `[tool.scikit-build] sdist.include`           |

Budget half a day. The configs are spec-correct but unverified against real CI runners.

## 7. Verify and announce

```bash
# In a fresh venv on a different machine:
pip install peli
python -c "import peli; print(peli.__version__)"

# If a small clip is at hand:
python -c "
import peli
vr = peli.VideoReader('clip.mp4', output='numpy')
print(len(vr), 'frames at', vr.get_avg_fps(), 'fps')
"
```

Once that works, you can announce. **Lead with concrete pain, not slogans** — see `README.md`'s rewritten intro for the pitch.

## 8. After v0.1 ships: real codec corpus

Task #11 is partially done. Current example clips: `count.mov` (mov, h264), `flipping_a_pancake.mkv` (mkv, h264 short), `Javelin_standing_throw_drill.mkv` (mkv, h264 longer). Missing: HEVC, VP9, AV1, MJPEG, and pathological cases (B-frames, VFR, tiny, broken).

Public-domain candidates:
- **Big Buck Bunny** has H.265, VP9, AV1, MJPEG variants on bunny.amara.org and archive.org
- **Sintel** for big resolution H.264
- **Netflix Public Test Patterns** (`netflix.com/blog/dynamic-optimizer/`) for stress

Add a `tests/data/` directory, write a `tests/python/test_codec_coverage.py` that runs the same VideoReader assertions per codec.

## 9. Open the HF datasets PR (Option A in `docs/integrations/huggingface-datasets.md`)

Concrete checklist is in that file. Don't open this PR until peli has been on PyPI for a couple of weeks and you have install numbers + at least one real downstream user. HF maintainers move faster on PRs from libraries that already have momentum.

## What you do not need to do

- Set up mkdocs site (task #13). v0.1 README is enough.
- Write a sphinx site (we explicitly chose not to).
- Build NVDEC for v0.1 (it's v0.2).
- Worry about Cython binding path (task notes track it; v0.3+).
