#version 330

uniform vec4 MaterialDiffuseColor;

out vec4 fragColor;

void main(void)
{
    // picking color
    fragColor = MaterialDiffuseColor;
}
