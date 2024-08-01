#version 150

in vec3 position;

//out vec3 ourColor;
in vec2 texCoord;
out vec2 s_TexCoord;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  s_TexCoord = texCoord;
}