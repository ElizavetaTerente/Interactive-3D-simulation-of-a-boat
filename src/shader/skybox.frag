#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube uSkybox;
uniform vec3 uDirectionalLightColor;

void main()
{
    FragColor = vec4(vec3(texture(uSkybox, TexCoords)) * uDirectionalLightColor, 1);
}
