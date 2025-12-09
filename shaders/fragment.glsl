#version 330 core

out vec4 FragColor;

// Couleur passée par le CPU
uniform vec4 colorOverride;

void main() {
    FragColor = colorOverride;
}
