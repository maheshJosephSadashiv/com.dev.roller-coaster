#version 150

in vec3 position;
//in vec4 color;
in vec3 normals;
out vec4 col;

uniform vec3 l_d;
uniform float K_d;
uniform float L_d;


uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
  // compute the transformed and projected vertex position (into gl_Position) 
  // compute the vertex color (into col)
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0f);
  vec3 temp = normalize(l_d - position);
  float intensity = abs(K_d * L_d * dot(normals, temp));
  //col = vec4(sqrt(normals.x *normals.x) * intensity,sqrt(normals.y *normals.y) * intensity,sqrt(normals.z *normals.z) * intensity, 1);
  col = vec4( intensity,intensity, intensity, 1);
}

