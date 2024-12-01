#version 330 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec3 inCol;
out vec3 chCol;

uniform vec2 uTranslation;

void main()
{
    gl_Position = vec4(aPosition + uTranslation, 0.0, 1.0);
    chCol = inCol;
}
