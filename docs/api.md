# API

## `peli.VideoReader`

```python
peli.VideoReader(uri, ctx=peli.cpu(0), width=-1, height=-1,
                 num_threads=0, fault_tol=-1, output=None)
```

Parameters:

- `uri` — file path, URL, file-like object with `.read()`, or bytes.
- `ctx` — `peli.cpu(0)` or `peli.gpu(i)`.
- `width`, `height` — optional resize. `-1` keeps the source size.
- `num_threads` — `0` lets FFmpeg pick.
- `fault_tol` — corrupt-frame tolerance. `-1` disables. Float in (0,1) is fraction of total frames; int ≥ 1 is absolute count. Hits raise `peli.PELILimitReachedError`.
- `output` — one of `numpy`, `dlpack`, `torch`, `jax`, `tf`, `keras`, `native`, or `None` (default, returns `peli.NDArray`).

Methods:

- `len(vr)` — frame count.
- `vr[i]` — frame at index `i`. Negative indices wrap.
- `vr[i:j]`, `vr.get_batch(indices)` — batch fetch.
- `for f in vr` — sequential iter.
- `vr.get_avg_fps()` — average frames-per-second.
- `vr.get_frame_timestamp(idx)` — `(N, 2)` ndarray of `(start, end)` seconds.
- `vr.get_key_indices()` — list of keyframe indices.
- `vr.seek(pos)`, `vr.seek_accurate(pos)` — seek.
- `vr.skip_frames(n)`.
- `vr.next()` — next frame; raises `StopIteration` at EOF.

## `peli.AudioReader`

```python
peli.AudioReader(uri, ctx=peli.cpu(0), sample_rate=-1, mono=True)
```

Returns the entire audio as a `(channels, samples)` float32 NDArray under `ar._array`. `_duration`, `_num_samples_per_channel`, `_num_channels` are also exposed.

## Contexts

```python
peli.cpu(device_id=0)
peli.gpu(device_id=0)
```

`peli.gpu` only works in NVDEC builds (`-DUSE_CUDA=ON`).

## NDArray

`peli.NDArray` is the default return type from `VideoReader`. It owns a buffer that can be:

- copied to NumPy: `arr.asnumpy()`
- exported via DLPack: `arr.__dlpack__()`, `arr.__dlpack_device__()`
- consumed by `np.from_dlpack`, `torch.from_dlpack`, `jax.dlpack.from_dlpack`, `tf.experimental.dlpack.from_dlpack`

Properties: `arr.shape`, `arr.dtype`, `arr.ctx`.

## Logging

```python
peli.logging.set_level(peli.logging.QUIET)   # suppress C++ stderr
```

Levels match FFmpeg's: `QUIET`, `PANIC`, `FATAL`, `ERROR`, `WARNING`, `INFO`, `VERBOSE`, `DEBUG`, `TRACE`.

## Exceptions

- `peli.PELIError` — generic decode failure (raised by `get_batch` on corrupt input).
- `peli.PELILimitReachedError` — `fault_tol` threshold hit.
- `FileNotFoundError` — local path doesn't exist.
- `ValueError` — empty bytes input or invalid `output=`.
- `RuntimeError` — file unreadable for other reasons.
