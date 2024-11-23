#version 330 core

layout (location = 0) in vec3 a_pos;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in int a_texture_index;

uniform mat4 u_view_proj;
uniform mat4 u_normal_mat;

out vec3 v_normal;
out vec2 v_uv;
flat out int v_texture_index;

void main()
{
    gl_Position = u_view_proj * vec4(a_pos, 1.0);
    v_normal = vec3(u_normal_mat * vec4(a_normal, 1.0));
    v_uv = a_uv * 3 - 1;
    v_texture_index = a_texture_index;
}
