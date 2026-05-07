import glob
import os

import numpy as np
import pytest

import peli


CORPUS = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'data', 'corpus'))


def _list_clips():
    if not os.path.isdir(CORPUS):
        return []
    return sorted(
        glob.glob(os.path.join(CORPUS, '*.mp4'))
        + glob.glob(os.path.join(CORPUS, '*.mkv'))
        + glob.glob(os.path.join(CORPUS, '*.mov'))
        + glob.glob(os.path.join(CORPUS, '*.webm'))
    )


CLIPS = _list_clips()
pytestmark = pytest.mark.skipif(
    not CLIPS,
    reason="no corpus; run tests/data/download_corpus.sh first",
)


@pytest.mark.parametrize("clip", CLIPS, ids=[os.path.basename(c) for c in CLIPS])
def test_open_and_read_first_frame(clip):
    vr = peli.VideoReader(clip)
    assert len(vr) > 0
    f = vr[0].asnumpy()
    assert f.ndim == 3
    assert f.dtype == np.uint8
    assert f.shape[2] == 3


@pytest.mark.parametrize("clip", CLIPS, ids=[os.path.basename(c) for c in CLIPS])
def test_random_access_consistency(clip):
    vr = peli.VideoReader(clip)
    n = len(vr)
    for idx in (0, n // 2, n - 1):
        a = vr[idx].asnumpy()
        b = vr[idx].asnumpy()
        assert np.array_equal(a, b), f"frame {idx} of {clip} returned different data on second read"


@pytest.mark.parametrize("clip", CLIPS, ids=[os.path.basename(c) for c in CLIPS])
def test_batch_matches_individual(clip):
    vr = peli.VideoReader(clip, output='numpy')
    n = len(vr)
    indices = [0, min(5, n - 1), min(20, n - 1)]
    batch = vr.get_batch(indices)
    for i, idx in enumerate(indices):
        single = peli.VideoReader(clip, output='numpy')[idx]
        assert np.array_equal(batch[i], single), f"batch[{i}] != vr[{idx}] for {clip}"


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
