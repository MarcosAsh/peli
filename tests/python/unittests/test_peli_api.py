"""Tests for peli-specific API surface that diverges from decord.

The inherited test_video_reader.py covers behavior parity. This file covers
what peli adds: the output= constructor kwarg, the DLPack dunder protocol on
peli.NDArray, and per-instance output isolation.
"""
import os

import numpy as np
import pytest

import peli
from peli import VideoReader, cpu

EXAMPLES = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'examples'))
CLIP = os.path.join(EXAMPLES, 'flipping_a_pancake.mkv')


# ---------- output= kwarg ----------

def test_output_default_returns_ndarray():
    vr = VideoReader(CLIP)
    frame = vr[0]
    assert type(frame).__name__ == 'NDArray'


def test_output_native_returns_ndarray():
    vr = VideoReader(CLIP, output='native')
    frame = vr[0]
    assert type(frame).__name__ == 'NDArray'


def test_output_numpy_returns_ndarray():
    vr = VideoReader(CLIP, output='numpy')
    frame = vr[0]
    assert isinstance(frame, np.ndarray)
    assert frame.dtype == np.uint8
    assert frame.ndim == 3
    assert frame.shape[-1] == 3


def test_output_numpy_get_batch_returns_4d_ndarray():
    vr = VideoReader(CLIP, output='numpy')
    batch = vr.get_batch([0, 5, 10])
    assert isinstance(batch, np.ndarray)
    assert batch.shape[0] == 3
    assert batch.shape[-1] == 3


def test_output_dlpack_returns_pycapsule():
    vr = VideoReader(CLIP, output='dlpack')
    frame = vr[0]
    assert type(frame).__name__ == 'PyCapsule'


def test_output_invalid_raises_value_error():
    with pytest.raises(ValueError, match='Invalid output'):
        VideoReader(CLIP, output='not_a_real_backend')


def test_output_torch_lazy_import_clean_error_when_missing():
    # If torch isn't installed, we should get a clear ImportError, not a crash.
    # If it IS installed, this test is a no-op (different code path).
    try:
        import torch  # noqa: F401
        pytest.skip('torch is installed; lazy-import-error path not exercised')
    except ImportError:
        pass
    with pytest.raises(ImportError):
        VideoReader(CLIP, output='torch')


def test_output_isolation_between_instances():
    """Per-instance output= must not leak to other instances (decord's bridge was global)."""
    vr_np = VideoReader(CLIP, output='numpy')
    vr_native = VideoReader(CLIP, output='native')
    assert isinstance(vr_np[0], np.ndarray)
    assert type(vr_native[0]).__name__ == 'NDArray'


# ---------- DLPack dunder protocol ----------

def test_ndarray_has_dlpack_dunder():
    vr = VideoReader(CLIP)
    frame = vr[0]
    assert hasattr(frame, '__dlpack__')
    assert hasattr(frame, '__dlpack_device__')


def test_ndarray_dlpack_device_is_cpu_for_default_ctx():
    vr = VideoReader(CLIP, ctx=cpu(0))
    frame = vr[0]
    device_type, device_id = frame.__dlpack_device__()
    # DLPack device type 1 == kDLCPU
    assert device_type == 1
    assert device_id == 0


def test_ndarray_dlpack_consumed_by_numpy_zero_copy_equivalent():
    """np.from_dlpack(frame) and frame.asnumpy() should produce identical content."""
    vr = VideoReader(CLIP)
    frame = vr[0]
    via_dlpack = np.from_dlpack(frame)
    # Re-fetch since the dlpack capsule consumed the previous one
    via_asnumpy = VideoReader(CLIP)[0].asnumpy()
    assert via_dlpack.shape == via_asnumpy.shape
    assert via_dlpack.dtype == via_asnumpy.dtype
    assert np.array_equal(via_dlpack, via_asnumpy)


def test_ndarray_dlpack_can_be_consumed_multiple_times():
    """Calling __dlpack__() produces a fresh capsule each time, so the source
    NDArray can be exported repeatedly without being invalidated."""
    vr = VideoReader(CLIP)
    frame = vr[0]
    a1 = np.from_dlpack(frame)
    a2 = np.from_dlpack(frame)
    assert a1.shape == a2.shape
    assert np.array_equal(a1, a2)


# ---------- Version + import surface ----------

def test_version_is_set():
    assert peli.__version__ == '0.1.0'


def test_top_level_exports():
    # Public names users should be able to import from peli directly
    assert hasattr(peli, 'VideoReader')
    assert hasattr(peli, 'cpu')
    assert hasattr(peli, 'gpu')
    assert hasattr(peli, '__version__')


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
