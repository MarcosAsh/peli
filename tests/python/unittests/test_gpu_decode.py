import os

import numpy as np
import pytest


def _has_cuda():
    try:
        import torch
        return torch.cuda.is_available()
    except Exception:
        return False


pytestmark = pytest.mark.skipif(not _has_cuda(), reason="no CUDA GPU")


EXAMPLES = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'examples'))
CLIP = os.path.join(EXAMPLES, 'flipping_a_pancake.mkv')


def test_gpu_context_constructs():
    import peli
    ctx = peli.gpu(0)
    assert ctx.device_type == 2


def test_gpu_videoreader_opens():
    import peli
    vr = peli.VideoReader(CLIP, ctx=peli.gpu(0))
    assert len(vr) == 310


def test_gpu_frame_shape_matches_cpu():
    import peli
    cpu_shape = peli.VideoReader(CLIP, ctx=peli.cpu(0))[0].shape
    gpu_shape = peli.VideoReader(CLIP, ctx=peli.gpu(0))[0].shape
    assert cpu_shape == gpu_shape


def test_gpu_dlpack_device_is_cuda():
    import peli
    f = peli.VideoReader(CLIP, ctx=peli.gpu(0))[0]
    device_type, device_id = f.__dlpack_device__()
    assert device_type == 2
    assert device_id == 0


def test_gpu_dlpack_to_torch_is_cuda_tensor():
    import peli, torch
    f = peli.VideoReader(CLIP, ctx=peli.gpu(0))[0]
    t = torch.from_dlpack(f)
    assert t.device.type == 'cuda'
    assert t.dtype == torch.uint8
    assert tuple(t.shape) == tuple(f.shape)


def test_gpu_decode_content_matches_cpu_within_tolerance():
    import peli, torch
    cpu_arr = peli.VideoReader(CLIP, ctx=peli.cpu(0))[0].asnumpy().astype(np.float32)
    gpu_t = torch.from_dlpack(peli.VideoReader(CLIP, ctx=peli.gpu(0))[0])
    gpu_arr = gpu_t.cpu().numpy().astype(np.float32)
    assert cpu_arr.shape == gpu_arr.shape
    assert abs(cpu_arr.mean() - gpu_arr.mean()) < 1.0


def test_gpu_batch_get():
    import peli
    vr = peli.VideoReader(CLIP, ctx=peli.gpu(0))
    batch = vr.get_batch([0, 5, 10, 50, 100])
    assert batch.shape == (5, 240, 426, 3)


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
