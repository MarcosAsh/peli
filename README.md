# peli

Video decoder for ML training pipelines. Open a file, index into it, get back NumPy arrays or PyTorch / JAX / TF tensors via DLPack. No framework imports unless you ask.

Forked from [dmlc/decord](https://github.com/dmlc/decord). Modernized for FFmpeg 7+/8 and Python 3.10–3.14, rebuilt around DLPack.

## Install

```bash
pip install peli
```

System FFmpeg ≥ 7 needs to be available. On Debian/Ubuntu:

```bash
sudo apt-get install -y libavcodec-dev libavformat-dev libavutil-dev \
                        libswresample-dev libavfilter-dev libavdevice-dev
```

On macOS: `brew install ffmpeg`. On Arch: `pacman -S ffmpeg`.

## Usage

```python
import peli

vr = peli.VideoReader("clip.mp4")
len(vr), vr.get_avg_fps()

frame = vr[0].asnumpy()                 # uint8 (H, W, 3)
batch = vr.get_batch([0, 30, 60]).asnumpy()
```

Returning the framework's native tensor type:

```python
vr = peli.VideoReader("clip.mp4", output="torch")  # or jax / tf / numpy / keras
t = vr[0]                                # torch.Tensor
```

The constructor lazy-imports the chosen framework, so installing `peli` doesn't pull in torch.

DLPack is exposed directly on `peli.NDArray`:

```python
import torch
t = torch.from_dlpack(vr[0])             # zero-copy
```

## GPU decode

NVDEC works for GPU-decoded pipelines:

```python
vr = peli.VideoReader("clip.mp4", ctx=peli.gpu(0))
t = torch.from_dlpack(vr[0])             # cuda:0 tensor, no host copy
```

CUDA 12+, libnvcuvid from the NVIDIA driver. Build from source with `-DUSE_CUDA=ON` until `peli-gpu` ships on PyPI.

## Building from source

```bash
git clone --recursive https://github.com/MarcosAsh/peli
cd peli
pip install .
```

Or for the C++ library on its own:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j
```

## Status

Working: random access, batch fetch, sequential iter, audio reader, DLPack, NumPy / Torch / JAX / TF / Keras output, NVDEC (verified on L4).

Not yet: VideoToolbox / AMD GPU decode, `pip install peli` (PyPI publish gated on first release tag).

## License

Apache-2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE) for upstream attribution.
