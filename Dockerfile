FROM ubuntu:22.04

# Installer les dépendances nécessaires
RUN apt update && apt install -y \
    cmake \
    g++ \
    git \
    libglfw3-dev \
    libglm-dev \
    libglew-dev \
    libx11-dev \
    libxi-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev

# Dossier de travail
WORKDIR /app

# Copier tout le projet dans le conteneur
COPY . .

# Générer et compiler le projet
RUN cmake -S . -B build && cmake --build build

# On ne lance PAS le programme ici (pas de GUI dans le conteneur)
CMD ["bash", "-c", "echo 'Build OK (exécutable dans ./build/vr)'"]

