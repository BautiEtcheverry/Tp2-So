#!/bin/bash
# filepath: /Users/benicio/Desktop/ArquiTPE/TPE/TPE-Arqui/buildTP.sh

# Script para compilar automáticamente con contenedor agodio/itba-so:2.0

CONTAINER_NAME="TPE-Arqui"
IMAGE_NAME="agodio/itba-so:2.0"
PROJECT_PATH="/root/tpe-arqui"
LOCAL_PATH="$(pwd)"
SKIP_STOP=false

# Verificar si se pasó el argumento -no-out
if [[ "$1" == "-no-out" || "$1" == "--no-out" ]]; then
    SKIP_STOP=true
fi

# Función para verificar si Docker está corriendo
check_docker() {
    if ! docker info >/dev/null 2>&1; then
        echo "❌ Error: Docker no está corriendo"
        exit 1
    fi
}

# Función para verificar si la imagen existe localmente
check_image() {
    if ! docker image inspect $IMAGE_NAME >/dev/null 2>&1; then
        echo "📥 Descargando imagen $IMAGE_NAME..."
        docker pull $IMAGE_NAME
        if [ $? -ne 0 ]; then
            echo "❌ Error: No se pudo descargar la imagen $IMAGE_NAME"
            exit 1
        fi
    else
        echo "✅ Imagen $IMAGE_NAME encontrada localmente"
    fi
}

# Función para verificar si el contenedor existe
check_container() {
    if docker container inspect $CONTAINER_NAME >/dev/null 2>&1; then
        echo "✅ Contenedor $CONTAINER_NAME encontrado"
        return 0
    else
        echo "📦 Contenedor $CONTAINER_NAME no existe"
        return 1
    fi
}

# Función para crear el contenedor
create_container() {
    echo "🔨 Creando contenedor $CONTAINER_NAME..."
    docker run -d \
        --name $CONTAINER_NAME \
        -v "$LOCAL_PATH:$PROJECT_PATH" \
        -w $PROJECT_PATH \
        --privileged \
        $IMAGE_NAME \
        tail -f /dev/null
    
    if [ $? -eq 0 ]; then
        echo "✅ Contenedor $CONTAINER_NAME creado exitosamente"
    else
        echo "❌ Error: No se pudo crear el contenedor"
        exit 1
    fi
}

# Función para iniciar el contenedor si está parado
start_container() {
    local status=$(docker container inspect -f '{{.State.Status}}' $CONTAINER_NAME 2>/dev/null)
    
    if [ "$status" = "exited" ] || [ "$status" = "created" ]; then
        echo "🚀 Iniciando contenedor $CONTAINER_NAME..."
        docker start $CONTAINER_NAME
    elif [ "$status" = "running" ]; then
        echo "✅ Contenedor $CONTAINER_NAME ya está corriendo"
    else
        echo "❌ Estado desconocido del contenedor: $status"
        exit 1
    fi
}

# Función principal de compilación
compile_project() {
    echo "🧹 Limpiando Toolchain..."
    docker exec -u root $CONTAINER_NAME make clean -C $PROJECT_PATH/x64BareBones/Toolchain
    
    echo "🧹 Limpiando proyecto principal..."
    docker exec -u root $CONTAINER_NAME make clean -C $PROJECT_PATH/x64BareBones/
    
    echo "🔧 Compilando Toolchain..."
    docker exec -u root $CONTAINER_NAME make -C $PROJECT_PATH/x64BareBones/Toolchain
    
    if [ $? -ne 0 ]; then
        echo "❌ Error en compilación de Toolchain"
        exit 1
    fi
    
    echo "🏗️  Compilando proyecto principal..."
    docker exec -u root $CONTAINER_NAME make -C $PROJECT_PATH/x64BareBones/
    
    if [ $? -ne 0 ]; then
        echo "❌ Error en compilación del proyecto principal"
        exit 1
    fi
}

# Función para detener contenedor
stop_container() {
    if ! $SKIP_STOP; then
        echo "🛑 Deteniendo contenedor..."
        docker stop $CONTAINER_NAME >/dev/null
    else
        echo "⚙️  Contenedor dejado corriendo (por opción -no-out)."
    fi
}

# MAIN SCRIPT EXECUTION
echo "🔍 Verificando Docker..."
check_docker

echo "🔍 Verificando imagen..."
check_image

echo "🔍 Verificando contenedor..."
if ! check_container; then
    create_container
fi

start_container
compile_project
stop_container

echo "✅ Proceso completado con éxito."