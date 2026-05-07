import os

import numpy as np
import pytest

import peli


EXAMPLES = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'examples'))
MP3 = os.path.join(EXAMPLES, 'example.mp3')


def test_audio_reader_opens_mp3():
    ar = peli.AudioReader(MP3)
    assert ar._duration > 0
    assert ar._num_samples_per_channel > 0


def test_audio_reader_mono_default():
    ar = peli.AudioReader(MP3)
    assert ar._num_channels == 1


def test_audio_reader_returns_float32():
    ar = peli.AudioReader(MP3)
    assert ar._array.dtype == np.float32
    assert ar._array.shape[0] == ar._num_channels


def test_audio_reader_stereo_when_not_mono():
    ar_mono = peli.AudioReader(MP3, mono=True)
    ar_stereo = peli.AudioReader(MP3, mono=False)
    assert ar_mono._num_channels == 1
    assert ar_stereo._num_channels >= 1


def test_audio_reader_resample():
    target = 8000
    ar = peli.AudioReader(MP3, sample_rate=target)
    expected = int(round(ar._duration * target))
    assert abs(ar._num_samples_per_channel - expected) / expected < 0.01


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
