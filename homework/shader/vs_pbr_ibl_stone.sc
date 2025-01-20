$input a_position, a_normal, a_texcoord0, a_tangent
$output v_pos, v_normal, v_texcoord0, v_tangent, v_bitangent, v_shadowcoord

#include "../common/common.sh"
#include "uniforms.sh"
uniform mat4 u_lightMtx;
void main()
{
	//gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
	vec3 normal  = a_normal*2.0 - 1.0;
	vec4 tangent = a_tangent * 2.0 - 1.0;
	vec3 wnormal = mul(u_model[0], vec4(normal.xyz, 0.0) ).xyz;
	vec3 wtangent = mul(u_model[0], vec4(tangent.xyz, 0.0) ).xyz;

	v_normal = normalize(wnormal);
	v_tangent = normalize(wtangent);
	v_bitangent = cross(v_normal, v_tangent) * tangent.w;


	v_pos = mul(u_model[0], vec4(a_position, 1.0)).xyz;

	v_texcoord0 = a_texcoord0;


	v_shadowcoord = mul(u_lightMtx, vec4(a_position, 1.0) );
}

