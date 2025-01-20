
$input v_pos,v_normal, v_shadowcoord
#include "../common/common.sh"
#include "uniforms.sh"
uniform vec4 Blinn_Phong;
#define SHADOW_PACKED_DEPTH 1
uniform vec4 u_lightPos;



SAMPLER2D(s_shadowMap, 0);
#	define Sampler sampler2D


float shadowmap(Sampler _sampler, vec4 _shadowCoord, float _bias)
{
	//vec3 texCoord = _shadowCoord.xyz/_shadowCoord.w;
	//return step(texCoord.z-_bias, unpackRgbaToFloat(texture2D(_sampler, texCoord.xy) ) );

    vec3 projCoords = _shadowCoord.xyz / _shadowCoord.w;
    //projCoords = projCoords * 0.5 + 0.5;

    float lightingDepth = unpackRgbaToFloat(texture2D(_sampler,projCoords.xy) ) ;
    float currentDepth = projCoords.z * 0.5 + 0.5;

	//vec3 lightDir = normalize(-lightPos.xyz);
	//vec3 normal = normalize(v_normal);
	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);


    float shadow = currentDepth  > lightingDepth  ? 0.0 : 1.0;
	return shadow;

}

float PCF(Sampler _sampler, vec4 _shadowCoord, float _bias, vec2 _texelSize)
{
	vec2 texCoord = _shadowCoord.xy/_shadowCoord.w;

	bool outside = any(greaterThan(texCoord, vec2_splat(1.0)))
				|| any(lessThan   (texCoord, vec2_splat(0.0)))
				 ;

	if (outside)
	{
		return 1.0;
	}

	float result = 0.0;
	vec2 offset = _texelSize * _shadowCoord.w;

	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-1.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-1.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-1.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-1.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-0.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-0.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-0.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(-0.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(0.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(0.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(0.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(0.5,  1.5) * offset, 0.0, 0.0), _bias);

	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(1.5, -1.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(1.5, -0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(1.5,  0.5) * offset, 0.0, 0.0), _bias);
	result += shadowmap(_sampler, _shadowCoord + vec4(vec2(1.5,  1.5) * offset, 0.0, 0.0), _bias);

	return result / 16.0;
}


void main()
{	

	vec3 normal = normalize(v_normal);

	vec3 lightcolor = vec3(1.0, 1.0, 1.0);
	vec3 lightDir = vec3(1.0, 1.0, 1.0);
	lightDir = normalize(-lightPos.xyz);

	//vec3 normal = normalize(v_normal);
	vec3 viewdir = normalize(u_camPos - v_pos);

	float shadowMapBias = 0.005;
	vec2 texelSize = vec2_splat(1.0/512.0);
	float visibility = 1.0;
	if(pcf == 1.0 ){
	visibility = PCF(s_shadowMap, v_shadowcoord, shadowMapBias, texelSize);
	}

	if(shadmap == 1.0 ){
	visibility = shadowmap(s_shadowMap, v_shadowcoord, shadowMapBias);
	}

	float ndotv = max(dot(normal, viewdir),0.0);
	float ndotl = max(dot(normal, lightDir),0.0);
	vec3 halfwayDir = normalize(lightDir + viewdir);  
	float ndoth = max(dot(normal, halfwayDir),0.0);
	float vdoth = max(dot(viewdir, halfwayDir),0.0);

	float diff = max(0.0, ndotl);
	float spec = max(0.0, ndoth);
	gl_FragColor.xyz = (Blinn_Phong.x + Blinn_Phong.y*lightcolor*diff*visibility + Blinn_Phong.z*lightcolor*pow(spec, Blinn_Phong.w)*visibility)*0.69;
	gl_FragColor.w = 1.0;
	}