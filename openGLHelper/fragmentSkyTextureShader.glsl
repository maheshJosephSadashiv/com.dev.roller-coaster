#version 150

out vec4 FragColor;
in vec2 s_TexCoord;

uniform sampler2D skybox;

void main()
{
  FragColor = texture(skybox, s_TexCoord);
}