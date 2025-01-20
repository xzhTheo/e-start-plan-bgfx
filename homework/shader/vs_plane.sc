$input a_position, a_normal
$output v_pos, v_normal, v_shadowcoord

#include "../common/common.sh"
#include "uniforms.sh"
uniform mat4 u_lightMtx;
void main()
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
	
	vec3 normal  = a_normal*2.0 - 1.0;
	v_normal = mul(u_model[0], vec4(normal.xyz, 0.0) ).xyz;

	v_pos = mul(u_model[0], vec4(a_position, 1.0)).xyz;

	v_shadowcoord = mul(u_lightMtx, vec4(a_position, 1.0) );
}

