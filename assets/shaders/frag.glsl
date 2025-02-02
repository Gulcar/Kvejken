#version 330 core

out vec4 v_frag_color;

in vec3 v_normal;
in vec2 v_uv;
flat in int v_texture_index;
in vec3 v_world_pos;

uniform sampler2D u_textures[16];

uniform float u_shading;
uniform float u_sun_light;

uniform int u_num_point_lights;
uniform vec3 u_point_lights_pos[4];
uniform vec3 u_point_lights_color[4];
uniform float u_point_lights_strength[4];

const vec3 sun_dir = normalize(vec3(-15, 100, -45));
const vec3 sun_color = vec3(0.8, 0.45, 0.45);

void main()
{
    //v_frag_color = vec4(v_uv / 2.0 + 0.5, 0.0, 1.0);

    vec4 tex_color;
    switch (v_texture_index)
    {
    case 0: tex_color = texture(u_textures[0], v_uv); break;
    case 1: tex_color = texture(u_textures[1], v_uv); break;
    case 2: tex_color = texture(u_textures[2], v_uv); break;
    case 3: tex_color = texture(u_textures[3], v_uv); break;
    case 4: tex_color = texture(u_textures[4], v_uv); break;
    case 5: tex_color = texture(u_textures[5], v_uv); break;
    case 6: tex_color = texture(u_textures[6], v_uv); break;
    case 7: tex_color = texture(u_textures[7], v_uv); break;
    case 8: tex_color = texture(u_textures[8], v_uv); break;
    case 9: tex_color = texture(u_textures[9], v_uv); break;
    case 10: tex_color = texture(u_textures[10], v_uv); break;
    case 11: tex_color = texture(u_textures[11], v_uv); break;
    case 12: tex_color = texture(u_textures[12], v_uv); break;
    case 13: tex_color = texture(u_textures[13], v_uv); break;
    case 14: tex_color = texture(u_textures[14], v_uv); break;
    case 15: tex_color = texture(u_textures[15], v_uv); break;
    }

    vec3 light = vec3(0);

    float sun_dot = max(dot(sun_dir, v_normal), 0.0) + 0.2;
    light += u_sun_light * sun_dot * sun_color;

    for (int i = 0; i < u_num_point_lights; i++)
    {
        vec3 dir = u_point_lights_pos[i] - v_world_pos;
        float attenuation = u_point_lights_strength[i] / (dot(dir, dir) + 1.0);
        float normal_dot = max(dot(normalize(dir), v_normal), 0.0);
        light += u_point_lights_color[i] * attenuation * normal_dot;
    }


    light = mix(vec3(1.0), light, u_shading);

    v_frag_color = tex_color * vec4(light, 1.0);

    //v_frag_color = vec4(v_uv, 0.0, 1.0);
    //v_frag_color = vec4(v_normal, 1.0);
    if (v_frag_color.a < 0.1)
        discard;
}
