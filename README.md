# peli

A video decoder for ML training pipelines. Open a video, index into it, get back frames as NumPy arrays, PyTorch tensors, JAX arrays, or TensorFlow tensors via zero-copy DLPack. Nothing imports your framework unless you ask.

**Why peli exists:**
- [decord](https://github.com/dmlc/decord) doesn't build on modern FFmpeg (last release 2022, broken on FFmpeg 5+).
- [torchcodec](https://github.com/meta-pytorch/torchcodec) is excellent but hard-imports `torch`, so JAX/TF/Keras users are stuck.
- [PyAV](https://github.com/PyAV-Org/PyAV) is too low-level for indexed random access.
- [DALI](https://github.com/NVIDIA/DALI) and [PyNvVideoCodec](https://github.com/NVIDIA/PyNvVideoCodec) are NVIDIA-only and GPU-only.

`peli` is a fork of decord, modernized for FFmpeg 7+/8 and Python 3.10–3.14, rebuilt around DLPack rather than a per-framework bridge system. v0.1 is CPU-first with first-class indexed random access; v0.2 adds NVDEC GPU decode.

## Status

**v0.1 — building from source on Linux.** macOS, Windows wheels, and `pip install peli` are pending.

| Feature                                                             | Status                                  |
| ------------------------------------------------------------------- | --------------------------------------- |
| `VideoReader` random access, batch fetch, sequential iter           | working                                 |
| FFmpeg 7+ / 8 compatibility                                         | working                                 |
| Python 3.10–3.14                                                    | working                                 |
| NumPy output                                                        | working                                 |
| DLPack output (`__dlpack__`) for torch / jax / tf / keras zero-copy | working                                 |
| `output="numpy"` / `"dlpack"` constructor kwarg                     | working                                 |
| `output="torch"` / `"jax"` / `"tf"` / `"keras"` constructor kwarg   | working (lazy import; not unit-tested)  |
| `AudioReader` (FFmpeg 7+ channel-layout migration done, untested)   | v0.2                                    |
| NVDEC GPU decode (CUDA 12+, returns CUDA tensors via DLPack)        | v0.2                                    |
| VideoToolbox / AMD GPU decode                                       | v0.3+                                   |
| Cross-platform wheels (`pip install peli`)                          | v0.1+                                   |

## Quickstart

```python
import peli

vr = peli.VideoReader("clip.mp4")
print(len(vr), vr.get_avg_fps())  # 143 15.998

frame = vr[0].asnumpy()           # uint8 (H, W, 3) RGB
batch = vr.get_batch([0, 30, 60, 90]).asnumpy()  # (4, H, W, 3)

for f in vr:                      # sequential iteration
    process(f.asnumpy())
```

## Install

Wheels are not published to PyPI yet. Once they are, install will be:

```bash
pip install peli
```

Until then, build from source.

### System FFmpeg

`peli` links against FFmpeg 7 or 8. Install the dev headers for your platform.

```bash
# Debian/Ubuntu
sudo apt-get install -y build-essential cmake python3-dev \
    libavcodec-dev libavformat-dev libavutil-dev libswresample-dev \
    libavfilter-dev libavdevice-dev

# Arch
sudo pacman -S base-devel cmake ffmpeg

# macOS
brew install cmake ffmpeg
```

### Build and install

```bash
git clone --recursive https://github.com/MarcosAsh/peli.git
cd peli
pip install .
```

That's it. `pip` drives the CMake build via [scikit-build-core](https://github.com/scikit-build/scikit-build-core) and produces a wheel that includes `libpeli.so`. For development:

```bash
pip install -e . --no-build-isolation  # editable install
```

If you want the C++ library on its own (e.g., to use it from another language), build with CMake directly:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Backend interop

`peli.VideoReader` returns `peli.NDArray` by default. The NDArray exposes `__dlpack__` and `__dlpack_device__`, so any DLPack-aware framework can adopt the buffer with zero copies:

```python
import peli, torch, jax, tensorflow as tf, numpy as np

vr = peli.VideoReader("clip.mp4")
frame = vr[0]                                       # peli.NDArray

n = np.from_dlpack(frame)                            # NumPy 1.22+
t = torch.from_dlpack(frame)                         # PyTorch
j = jax.dlpack.from_dlpack(frame.__dlpack__())       # JAX
x = tf.experimental.dlpack.from_dlpack(frame.__dlpack__())  # TensorFlow
```

For convenience, the constructor takes an `output=` kwarg that wraps the conversion for you. Framework imports are lazy: nothing imports torch unless you ask for it.

```python
vr = peli.VideoReader("clip.mp4", output="numpy")  # vr[i] is np.ndarray
vr = peli.VideoReader("clip.mp4", output="torch")  # vr[i] is torch.Tensor
vr = peli.VideoReader("clip.mp4", output="jax")    # vr[i] is jax.Array
vr = peli.VideoReader("clip.mp4", output="tf")     # vr[i] is tf.Tensor
vr = peli.VideoReader("clip.mp4", output="keras")  # uses active Keras 3 backend
```

`output="dlpack"` returns the raw PyCapsule, and `output="native"` (or `output=None`) returns `peli.NDArray`. Unlike `decord.bridge.set_bridge(...)`, the choice is per-`VideoReader` instance, so different parts of a pipeline can use different backends without stomping on each other.

## Why fork decord?

Upstream `decord` (last release 2022) does not build against FFmpeg 5+, modern GCC, or Python 3.12+, and is no longer actively maintained. `peli` keeps decord's core architecture (C++ FFmpeg core, ctypes FFI, `VideoReader` API) and modernizes the parts that bit-rotted: header includes, removed FFmpeg APIs, the channel-layout migration, the filter graph init order. Public API differences from decord:

- DLPack is the primary output contract; `decord.bridge.set_bridge(...)` is replaced by per-instance `output=` (planned).
- `decord.bridge` is not exported.

## License and acknowledgments

`peli` is licensed under the Apache License, Version 2.0 (see [LICENSE](LICENSE)). It is derived from [dmlc/decord](https://github.com/dmlc/decord), also Apache-2.0. See [NOTICE](NOTICE) for the full attribution.

Bundled third-party software:

- [dlpack](https://github.com/dmlc/dlpack) — Apache-2.0
- [dmlc-core](https://github.com/dmlc/dmlc-core) — Apache-2.0
