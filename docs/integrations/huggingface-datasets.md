# Using peli with 🤗 Datasets

HF `datasets` ships with torchcodec as the only video backend. Until that changes, the working pattern is to skip HF's decode and hand bytes to `peli` yourself.

```python
from datasets import load_dataset, Video
import io
import peli

ds = load_dataset("some/video-dataset")
ds = ds.cast_column("video", Video(decode=False))

def to_peli(example, output="torch"):
    raw = example["video"]["bytes"]
    src = io.BytesIO(raw) if raw is not None else example["video"]["path"]
    return peli.VideoReader(src, output=output)

ds = ds.map(lambda ex: {"vr": to_peli(ex)})
```

`vr[i]` is now whatever tensor type matches `output=`. No copy through `.asnumpy()`.

Upstream support would mean adding peli as an option in `Video.decode_example` or shipping a parallel `VideoPeli` feature class. The smaller patch is the second; it doesn't refactor existing code paths.
