# Migrating from decord

`peli` is a fork of [dmlc/decord](https://github.com/dmlc/decord) modernized for FFmpeg 7+/8 and current Python toolchains. The public API is intentionally close to decord's so most code needs only an import rename.

## TL;DR

Most decord code works in peli with three character-level changes:

```python
# decord
import decord
decord.bridge.set_bridge("torch")
vr = decord.VideoReader("clip.mp4")
frame = vr[0]            # torch.Tensor

# peli
import peli
vr = peli.VideoReader("clip.mp4", output="torch")
frame = vr[0]            # torch.Tensor
```

If you were using `.asnumpy()` everywhere, your code keeps working with no changes beyond the `decord` → `peli` import.

## What stays the same

- `VideoReader(uri, ctx, width, height, num_threads, fault_tol)` — identical signature.
- `__getitem__`, `__len__`, `get_avg_fps`, `get_batch`, `seek`, `seek_accurate`, `skip_frames`, `next`, `get_key_indices`, `get_frame_timestamp`.
- `peli.cpu(0)`, `peli.gpu(i)` context constructors.
- `NDArray.asnumpy()` returns the same `numpy.ndarray` it did in decord.
- `peli.bridge.set_bridge("torch" | "tensorflow" | ...)` still works as a global, for backward compat.

## What's new

### `output=` constructor kwarg

Per-instance output type, replacing decord's global `bridge.set_bridge`. Framework imports are lazy.

```python
peli.VideoReader("clip.mp4")                       # default: returns peli.NDArray
peli.VideoReader("clip.mp4", output="numpy")       # returns numpy.ndarray
peli.VideoReader("clip.mp4", output="dlpack")      # returns DLPack PyCapsule
peli.VideoReader("clip.mp4", output="torch")       # returns torch.Tensor
peli.VideoReader("clip.mp4", output="jax")         # returns jax.Array
peli.VideoReader("clip.mp4", output="tf")          # returns tf.Tensor
peli.VideoReader("clip.mp4", output="keras")       # uses active Keras 3 backend
```

Different `VideoReader` instances in the same process can now use different output types without stomping on each other (decord's bridge was process-global).

### `__dlpack__` / `__dlpack_device__` on `peli.NDArray`

`peli.NDArray` implements the modern DLPack dunder protocol, so DLPack-aware frameworks can adopt the buffer zero-copy without `.asnumpy()`:

```python
vr = peli.VideoReader("clip.mp4")
frame = vr[0]                          # peli.NDArray

import torch; t = torch.from_dlpack(frame)              # zero-copy, no detour
import jax;   j = jax.dlpack.from_dlpack(frame.__dlpack__())
import numpy as np; n = np.from_dlpack(frame)          # numpy 1.22+
```

decord didn't expose `__dlpack__`, so torch users had to go through `.asnumpy()` (which copies) or use the `bridge.set_bridge("torch")` global.

## What's different

### Package name

`decord` → `peli` everywhere. Library file: `libdecord.so` → `libpeli.so`. Env var: `DECORD_LIBRARY_PATH` → `PELI_LIBRARY_PATH`.

### Default output type

Same as decord: `vr[i]` returns a `peli.NDArray`. To get numpy, call `.asnumpy()` or pass `output="numpy"` to the constructor.

### FFmpeg version

`peli` requires FFmpeg ≥ 7. Decord's last release (2022) targeted FFmpeg 4.x and won't build against current FFmpeg.

### Python version

`peli` supports Python 3.10–3.14. Decord's last wheels were for 3.6–3.10.

## What's not yet in v0.1

- `AudioReader` is included and compiles, but the runtime path hasn't been validated end-to-end yet. Expected v0.2.
- `VideoLoader` (multi-file batched loader) is inherited from decord but not specifically tested.
- GPU decode (NVDEC) is in the codebase but disabled by default in wheels. Expected first-class support v0.3+.

If any of these are blocking for your migration, file an issue.

## Side-by-side cheat sheet

| Task                                  | decord                                         | peli                                                 |
| ------------------------------------- | ---------------------------------------------- | ---------------------------------------------------- |
| Open file                             | `decord.VideoReader("v.mp4")`                  | `peli.VideoReader("v.mp4")`                          |
| Get a frame as numpy                  | `vr[0].asnumpy()`                              | `vr[0].asnumpy()`                                    |
| Get a frame as torch (decord style)   | `decord.bridge.set_bridge('torch'); vr[0]`     | `peli.VideoReader("v.mp4", output='torch')[0]`       |
| Get a frame as torch (DLPack, peli)   | `torch.from_dlpack(vr[0].asnumpy())` (copies)  | `torch.from_dlpack(vr[0])` (zero-copy)               |
| Batch                                 | `vr.get_batch([0, 5, 10]).asnumpy()`           | `vr.get_batch([0, 5, 10]).asnumpy()`                 |
| Sequential                            | `for f in vr: ...`                             | `for f in vr: ...`                                   |
| Set up GPU context                    | `decord.gpu(0)`                                | `peli.gpu(0)` (decode itself is v0.3+)               |
