#version 330 core

out vec4 v_frag_color;

in vec3 v_normal;
in vec2 v_uv;
flat in int v_texture_index;

uniform sampler2D u_textures[16];

const vec3 sun_dir = normalize(vec3(15, 100, 45));
const vec4 sun_color = vec4(1.0, 0.9, 0.9, 1.0);

void main()
{
    v_frag_color = vec4(v_uv / 2.0 + 0.5, 0.0, 1.0);

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

    float light = max(dot(sun_dir, v_normal), 0.0) + 0.15;

    v_frag_color = tex_color * sun_color * light;

    //v_frag_color = vec4(v_uv, 0.0, 1.0);
    //v_frag_color = vec4(v_normal, 1.0);
    if (v_frag_color.a < 0.1)
        discard;
}
