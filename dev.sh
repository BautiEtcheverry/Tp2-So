#!/usr/bin/env bash
set -euo pipefail

IMAGE="agodio/itba-so-multiarch:3.1"
CONTAINER="tp2-so"
MOUNT_PATH="/root/tp2-so"
WORKDIR="$MOUNT_PATH/x64BareBones"
LOCAL_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

require_docker() {
    if ! docker info >/dev/null 2>&1; then
        echo "Error: Docker no esta corriendo. Iniciar Docker Desktop y reintentar." >&2
        exit 1
    fi
}

ensure_image() {
    if docker image inspect "$IMAGE" >/dev/null 2>&1; then
        return
    fi
    echo "Imagen $IMAGE no encontrada localmente. Descargando..."
    docker pull "$IMAGE"
}

container_exists() {
    docker container inspect "$CONTAINER" >/dev/null 2>&1
}

container_running() {
    [ "$(docker container inspect -f '{{.State.Running}}' "$CONTAINER" 2>/dev/null)" = "true" ]
}

create_container() {
    echo "Creando contenedor $CONTAINER..."
    docker run -d \
        --name "$CONTAINER" \
        -v "$LOCAL_PATH:$MOUNT_PATH" \
        -w "$WORKDIR" \
        --privileged \
        "$IMAGE" \
        tail -f /dev/null >/dev/null
}

ensure_container() {
    if ! container_exists; then
        create_container
        return
    fi
    if ! container_running; then
        echo "Iniciando contenedor $CONTAINER..."
        docker start "$CONTAINER" >/dev/null
    fi
}

enter_shell() {
    echo "Entrando al contenedor $CONTAINER (workdir: $WORKDIR). Salir con 'exit'."
    docker exec -it -w "$WORKDIR" "$CONTAINER" bash
}

main() {
    require_docker
    ensure_image
    ensure_container
    enter_shell
}

main "$@"
