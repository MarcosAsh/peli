"""Video Reader."""
from __future__ import absolute_import

import ctypes
import numpy as np

from ._ffi.base import c_array, c_str
from ._ffi.function import _init_api
from ._ffi.ndarray import PELIContext
from .base import PELIError
from . import ndarray as _nd
from .ndarray import cpu, gpu
from .bridge import bridge_out

VideoReaderHandle = ctypes.c_void_p


_VALID_OUTPUTS = ("native", "numpy", "dlpack", "torch", "jax", "tf", "keras")


def _make_converter(output):
    """Build a per-frame converter for the requested output type.

    Framework imports are deferred to first call so peli stays free of
    torch/jax/tf/keras dependencies unless the user opts in.
    """
    if output is None or output == "native":
        return None  # caller falls through to legacy bridge_out path
    if output == "numpy":
        return lambda arr: arr.asnumpy()
    if output == "dlpack":
        return lambda arr: arr.__dlpack__()
    if output == "torch":
        import torch
        return lambda arr: torch.from_dlpack(arr)
    if output == "jax":
        import jax
        return lambda arr: jax.dlpack.from_dlpack(arr.__dlpack__())
    if output == "tf":
        import tensorflow as tf
        return lambda arr: tf.experimental.dlpack.from_dlpack(arr.__dlpack__())
    if output == "keras":
        import keras
        backend = keras.config.backend()
        if backend == "torch":
            import torch
            return lambda arr: torch.from_dlpack(arr)
        if backend == "jax":
            import jax
            return lambda arr: jax.dlpack.from_dlpack(arr.__dlpack__())
        if backend == "tensorflow":
            import tensorflow as tf
            return lambda arr: tf.experimental.dlpack.from_dlpack(arr.__dlpack__())
        if backend == "numpy":
            return lambda arr: arr.asnumpy()
        return lambda arr: keras.ops.convert_to_tensor(arr.asnumpy())
    raise ValueError("Invalid output={!r}; expected one of {}".format(output, _VALID_OUTPUTS))


class VideoReader(object):
    """Individual video reader with convenient indexing and seeking functions.

    Parameters
    ----------
    uri : str
        Path of video file.
    ctx : peli.Context
        The context to decode the video file, can be peli.cpu() or peli.gpu().
    width : int, default is -1
        Desired output width of the video, unchanged if `-1` is specified.
    height : int, default is -1
        Desired output height of the video, unchanged if `-1` is specified.
    num_threads : int, default is 0
        Number of decoding thread, auto if `0` is specified.
    fault_tol : int, default is -1
        The threshold of corupted and recovered frames. This is to prevent silent fault
        tolerance when for example 50% frames of a video cannot be decoded and duplicate
        frames are returned. You may find the fault tolerant feature sweet in many cases,
        but not for training models. Say `N = # recovered frames`
        If `fault_tol` < 0, nothing will happen.
        If 0 < `fault_tol` < 1.0, if N > `fault_tol * len(video)`, raise `PELILimitReachedError`.
        If 1 < `fault_tol`, if N > `fault_tol`, raise `PELILimitReachedError`.


    """
    def __init__(self, uri, ctx=cpu(0), width=-1, height=-1, num_threads=0, fault_tol=-1,
                 output=None):
        self._handle = None
        assert isinstance(ctx, PELIContext)
        if output is not None and output not in _VALID_OUTPUTS:
            raise ValueError(
                "Invalid output={!r}; expected one of {}".format(output, _VALID_OUTPUTS))
        fault_tol = str(fault_tol)
        if hasattr(uri, 'read'):
            ba = bytearray(uri.read())
            display_uri = '{} bytes'.format(len(ba))
            if len(ba) == 0:
                raise ValueError("Cannot open an empty bytes input")
            self._handle = _CAPI_VideoReaderGetVideoReader(
                ba, ctx.device_type, ctx.device_id, width, height, num_threads, 2, fault_tol)
        else:
            display_uri = uri
            # Fail fast with a familiar exception type for missing paths.
            # FFmpeg also handles URIs (http://, rtsp://, etc.), so only
            # check the filesystem when the input looks like a local path.
            import os as _os
            if isinstance(uri, str) and '://' not in uri and not _os.path.exists(uri):
                raise FileNotFoundError("No such video file: {!r}".format(uri))
            self._handle = _CAPI_VideoReaderGetVideoReader(
                uri, ctx.device_type, ctx.device_id, width, height, num_threads, 0, fault_tol)
        if self._handle is None:
            raise RuntimeError("Could not open video {!r}".format(display_uri))
        self._num_frame = _CAPI_VideoReaderGetFrameCount(self._handle)
        assert self._num_frame > 0, "Invalid frame count: {}".format(self._num_frame)
        self._key_indices = None
        self._frame_pts = None
        self._avg_fps = None
        self._output = output
        self._convert = _make_converter(output)

    def __del__(self):
        try:
            if self._handle is not None:
                _CAPI_VideoReaderFree(self._handle)
        except TypeError:
            pass

    def _emit(self, arr):
        # Per-instance output= overrides the legacy global bridge.
        if self._convert is not None:
            return self._convert(arr)
        return bridge_out(arr)

    def __len__(self):
        """Get length of the video. Note that sometimes FFMPEG reports inaccurate number of frames,
        we always follow what FFMPEG reports.

        Returns
        -------
        int
            The number of frames in the video file.

        """
        return self._num_frame

    def __getitem__(self, idx):
        """Get frame at `idx`.

        Parameters
        ----------
        idx : int or slice
            The frame index, can be negative which means it will index backwards,
            or slice of frame indices.

        Returns
        -------
        ndarray
            Frame of shape HxWx3 or batch of image frames with shape NxHxWx3,
            where N is the length of the slice.
        """
        if isinstance(idx, slice):
            return self.get_batch(range(*idx.indices(len(self))))
        if idx < 0:
            idx += self._num_frame
        if idx >= self._num_frame or idx < 0:
            raise IndexError("Index: {} out of bound: {}".format(idx, self._num_frame))
        self.seek_accurate(idx)
        return self.next()

    def next(self):
        """Grab the next frame.

        Returns
        -------
        ndarray
            Frame with shape HxWx3.

        """
        assert self._handle is not None
        arr = _CAPI_VideoReaderNextFrame(self._handle)
        if not arr.shape:
            raise StopIteration()
        return self._emit(arr)

    def _validate_indices(self, indices):
        """Validate int64 integers and convert negative integers to positive by backward search"""
        assert self._handle is not None
        indices = np.array(indices, dtype=np.int64)
        # process negative indices
        indices[indices < 0] += self._num_frame
        if not (indices >= 0).all():
            raise IndexError(
                'Invalid negative indices: {}'.format(indices[indices < 0] + self._num_frame))
        if not (indices < self._num_frame).all():
            raise IndexError('Out of bound indices: {}'.format(indices[indices >= self._num_frame]))
        return indices

    def get_frame_timestamp(self, idx):
        """Get frame playback timestamp in unit(second).

        Parameters
        ----------
        indices: list of integers or slice
            A list of frame indices. If negative indices detected, the indices will be indexed from backward.

        Returns
        -------
        numpy.ndarray
            numpy.ndarray of shape (N, 2), where N is the size of indices. The format is `(start_second, end_second)`.
        """
        assert self._handle is not None
        if isinstance(idx, slice):
            idx = self.get_batch(range(*idx.indices(len(self))))
        idx = self._validate_indices(idx)
        if self._frame_pts is None:
            self._frame_pts = _CAPI_VideoReaderGetFramePTS(self._handle).asnumpy()
        return self._frame_pts[idx, :]


    def get_batch(self, indices):
        """Get entire batch of images. `get_batch` is optimized to handle seeking internally.
        Duplicate frame indices will be optmized by copying existing frames rather than decode
        from video again.

        Parameters
        ----------
        indices : list of integers
            A list of frame indices. If negative indices detected, the indices will be indexed from backward

        Returns
        -------
        ndarray
            An entire batch of image frames with shape NxHxWx3, where N is the length of `indices`.

        """
        assert self._handle is not None
        indices = _nd.array(self._validate_indices(indices))
        arr = _CAPI_VideoReaderGetBatch(self._handle, indices)
        return self._emit(arr)

    def get_key_indices(self):
        """Get list of key frame indices.

        Returns
        -------
        list
            List of key frame indices.

        """
        if self._key_indices is None:
            self._key_indices = _CAPI_VideoReaderGetKeyIndices(self._handle).asnumpy().tolist()
        return self._key_indices

    def get_avg_fps(self):
        """Get average FPS(frame per second).

        Returns
        -------
        float
            Average FPS.

        """
        if self._avg_fps is None:
            self._avg_fps = _CAPI_VideoReaderGetAverageFPS(self._handle)
        return self._avg_fps

    def seek(self, pos):
        """Fast seek to frame position, this does not guarantee accurate position.
        To obtain accurate seeking, see `accurate_seek`.

        Parameters
        ----------
        pos : integer
            Non negative seeking position.

        """
        assert self._handle is not None
        assert pos >= 0 and pos < self._num_frame
        success = _CAPI_VideoReaderSeek(self._handle, pos)
        if not success:
            raise RuntimeError("Failed to seek to frame {}".format(pos))

    def seek_accurate(self, pos):
        """Accurately seek to frame position, this is slower than `seek`
        but guarantees accurate position.

        Parameters
        ----------
        pos : integer
            Non negative seeking position.

        """
        assert self._handle is not None
        assert pos >= 0 and pos < self._num_frame
        success = _CAPI_VideoReaderSeekAccurate(self._handle, pos)
        if not success:
            raise RuntimeError("Failed to seek_accurate to frame {}".format(pos))

    def skip_frames(self, num=1):
        """Skip reading multiple frames. Skipped frames will still be decoded
        (required by following frames) but it can save image resize/copy operations.


        Parameters
        ----------
        num : int, default is 1
            The number of frames to be skipped.

        """
        assert self._handle is not None
        assert num > 0
        _CAPI_VideoReaderSkipFrames(self._handle, num)

_init_api("peli.video_reader")
