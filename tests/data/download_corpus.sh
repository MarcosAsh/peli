#!/usr/bin/env bash
set -euo pipefail

DEST="$(cd "$(dirname "$0")" && pwd)/corpus"
mkdir -p "$DEST"
cd "$DEST"

fetch() {
  local url="$1" out="$2"
  if [ -f "$out" ]; then
    echo "have $out"
    return 0
  fi
  echo "fetching $url -> $out"
  if curl -fsSL --retry 3 -o "$out.partial" "$url"; then
    mv "$out.partial" "$out"
  else
    rm -f "$out.partial"
    echo "  ! skipped: $url unavailable"
    return 0
  fi
}

fetch "https://download.blender.org/peach/bigbuckbunny_movies/BigBuckBunny_320x180.mp4" \
      "bbb_h264_320x180.mp4"

fetch "https://archive.org/download/Sintel/sintel-2048-stereo.mp4" \
      "sintel_h264_480p.mp4"

fetch "https://media.xiph.org/video/derf/y4m/akiyo_qcif.y4m" \
      "akiyo_qcif.y4m"

fetch "https://www.webmproject.org/media/learning-center/video/big-buck-bunny_trailer.webm" \
      "bbb_vp8.webm"

echo
echo "downloaded:"
ls -la "$DEST"
