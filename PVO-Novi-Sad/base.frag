#version 330 core

in vec3 chCol;
out vec4 outCol;
uniform vec3 color;

void main()
{
	outCol = vec4(chCol.rgb + color, 1.0);
}