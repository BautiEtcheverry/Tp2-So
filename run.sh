#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_PATH="$SCRIPT_DIR/x64BareBones/Image/x64BareBonesImage.qcow2"

audio_opts() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "-audiodev id=audio0,driver=coreaudio -machine pcspk-audiodev=audio0"
    else
        echo "-audiodev id=audio0,driver=alsa -machine pcspk-audiodev=audio0"
    fi
}

require_image() {
    if [ ! -f "$IMAGE_PATH" ]; then
        echo "Error: no se encontro $IMAGE_PATH." >&2
        echo "Compilar primero con: ./dev.sh -> (dentro del contenedor) make" >&2
        exit 1
    fi
}

main() {
    require_image
    # shellcheck disable=SC2046
    qemu-system-x86_64 \
        -hda "$IMAGE_PATH" \
        -m 512 \
        -rtc base=localtime,clock=host \
        $(audio_opts)
}

main "$@"
