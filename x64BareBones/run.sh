#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_PATH="$SCRIPT_DIR/Image/x64BareBonesImage.qcow2"


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
        -d int,cpu_reset,guest_errors -no-reboot -no-shutdown -D qemu.log

}

main "$@"
