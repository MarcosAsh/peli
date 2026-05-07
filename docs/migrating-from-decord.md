# From decord to peli

Most decord code works in peli with an import rename.

```python
# decord
import decord
decord.bridge.set_bridge("torch")
vr = decord.VideoReader("clip.mp4")
frame = vr[0]                           # torch.Tensor

# peli
import peli
vr = peli.VideoReader("clip.mp4", output="torch")
frame = vr[0]                           # torch.Tensor
```

`.asnumpy()` still works; if your code calls it everywhere, just rename the import.

## What changed

`peli` replaces `decord.bridge.set_bridge(...)` (process-global) with a per-instance `output=` kwarg on `VideoReader`. Values: `numpy`, `dlpack`, `torch`, `jax`, `tf`, `keras`, `native`. Framework imports are lazy.

`peli.NDArray` implements the DLPack dunder protocol, so `torch.from_dlpack(vr[0])` works zero-copy without going through `.asnumpy()`.

The package, library, and headers were renamed: `decord` → `peli`, `libdecord.so` → `libpeli.so`, `DECORD_LIBRARY_PATH` → `PELI_LIBRARY_PATH`.

Minimum FFmpeg version is now 5. Decord's last release targeted 4 and stopped working when 5 dropped the BSF header into `libavcodec/bsf.h`.

## Equivalent calls

| decord                                         | peli                                              |
| ---------------------------------------------- | ------------------------------------------------- |
| `decord.VideoReader("v.mp4")`                  | `peli.VideoReader("v.mp4")`                       |
| `vr[0].asnumpy()`                              | `vr[0].asnumpy()`                                 |
| `decord.bridge.set_bridge("torch"); vr[0]`     | `peli.VideoReader("v.mp4", output="torch")[0]`    |
| `decord.gpu(0)`                                | `peli.gpu(0)`                                     |
| `vr.get_batch([0, 5, 10]).asnumpy()`           | `vr.get_batch([0, 5, 10]).asnumpy()`              |
