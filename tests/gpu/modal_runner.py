import modal


REPO_ROOT = "/workspace/peli"


image = (
    modal.Image.from_registry(
        "nvidia/cuda:12.6.3-cudnn-devel-ubuntu24.04",
        add_python="3.12",
    )
    .apt_install(
        "build-essential",
        "cmake",
        "git",
        "pkg-config",
        "libavcodec-dev",
        "libavformat-dev",
        "libavutil-dev",
        "libswresample-dev",
        "libavfilter-dev",
        "libavdevice-dev",
    )
    .pip_install("numpy", "pytest")
    .pip_install("torch", index_url="https://download.pytorch.org/whl/cu126")
    .add_local_dir(
        ".",
        REPO_ROOT,
        ignore=[
            "build/**",
            "dist/**",
            ".git/**",
            ".cache/**",
            "tools/modal/**",
            "**/__pycache__/**",
            "**/*.pyc",
        ],
    )
)

app = modal.App("peli-gpu-ci", image=image)


@app.function(gpu="L4", timeout=1800)
def build_and_test() -> int:
    import os
    import shlex
    import subprocess
    import sys

    def run(cmd, **kwargs):
        print(f"$ {' '.join(shlex.quote(c) for c in cmd)}", flush=True)
        return subprocess.run(cmd, **kwargs)

    subprocess.run(
        [
            "ln",
            "-sf",
            "/usr/lib/x86_64-linux-gnu/libnvcuvid.so.1",
            "/usr/lib/x86_64-linux-gnu/libnvcuvid.so",
        ],
        check=False,
    )

    os.makedirs("/tmp/build", exist_ok=True)
    r = run(
        [
            "cmake",
            "-S", REPO_ROOT,
            "-B", "/tmp/build",
            "-DUSE_CUDA=ON",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_CUDA_ARCHITECTURES=89",
        ]
    )
    if r.returncode != 0:
        return r.returncode

    r = run(["make", "-C", "/tmp/build", "-j", "4"])
    if r.returncode != 0:
        return r.returncode

    env = {
        **os.environ,
        "PYTHONPATH": f"{REPO_ROOT}/python",
        "PELI_LIBRARY_PATH": "/tmp/build",
    }
    r = run(
        [
            "python3", "-m", "pytest",
            f"{REPO_ROOT}/tests/python/unittests/test_gpu_decode.py",
            f"{REPO_ROOT}/tests/python/unittests/test_peli_api.py",
            f"{REPO_ROOT}/tests/python/unittests/test_video_reader.py",
            "-v",
        ],
        env=env,
    )
    return r.returncode


@app.local_entrypoint()
def main():
    rc = build_and_test.remote()
    if rc != 0:
        raise SystemExit(rc)
