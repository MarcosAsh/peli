# peli

Video decoder for ML training pipelines. Open a file, index into it, get back NumPy arrays or PyTorch / JAX / TF tensors via DLPack.

```python
import peli

vr = peli.VideoReader("clip.mp4")
frame = vr[0].asnumpy()
batch = vr.get_batch([0, 30, 60]).asnumpy()
```

Different output type per instance:

```python
vr = peli.VideoReader("clip.mp4", output="torch")
t = vr[0]                                 # torch.Tensor
```

Framework imports are lazy. Installing `peli` doesn't pull in torch.

## DLPack

`peli.NDArray` exposes the DLPack dunder protocol, so any DLPack-aware framework can adopt the buffer zero-copy:

```python
import torch
t = torch.from_dlpack(vr[0])
```

## GPU decode

NVDEC works for CUDA pipelines:

```python
vr = peli.VideoReader("clip.mp4", ctx=peli.gpu(0))
t = torch.from_dlpack(vr[0])              # cuda:0 tensor
```

Build from source with `-DUSE_CUDA=ON` until `peli-gpu` ships on PyPI.

## Install

```bash
pip install peli
```

System FFmpeg ≥ 7 is required.
