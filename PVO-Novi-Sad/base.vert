#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;
out vec3 chCol;


uniform vec2 translation;
uniform bool applyTranslation;

void main()
{
	if (applyTranslation)
        gl_Position = vec4(inPos + translation, 0.0, 1.0);
    else
        gl_Position = vec4(inPos, 0.0, 1.0); // Nema translacije
	chCol = inCol;
}