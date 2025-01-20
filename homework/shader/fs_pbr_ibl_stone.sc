$input v_pos, v_normal,  v_tangent, v_bitangent,v_texcoord0, v_shadowcoord

#include "../common/common.sh"
#include "uniforms.sh"
#define SHADOW_PACKED_DEPTH 1

uniform vec4 Blinn_Phong;
uniform vec4 u_lightPos;

SAMPLER2D(s_texColor,  0);
SAMPLER2D(s_ao_rou_meta,  1);
SAMPLER2D(s_texnormal,2);
SAMPLERCUBE(s_texCube, 3);
SAMPLERCUBE(s_texCubeIrr, 4);
SAMPLER2D(s_texLUT, 5);

#define PI 3.1415926

SAMPLER2D(s_shadowMap, 6);
#	define Sampler sampler2D



float DistributionGGX(float ndoth, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float ndoth2 = ndoth*ndoth;

    float nom   = a2;
    float denom = (ndoth2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926 * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float ndotv, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = ndotv;
    float denom = ndotv * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(float ndotv, float ndotl, float roughness)
{
    float ggx2 = GeometrySchlickGGX(ndotv, roughness);
    float ggx1 = GeometrySchlickGGX(ndotl, roughness);

    return ggx1 * ggx2;
}
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3_splat(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   
float hardShadow(Sampler _sampler, vec4 _shadowCoord, float _bias)
{
	vec3 texCoord = _shadowCoord.xyz/_shadowCoord.w;
	return step(texCoord.z-_bias, unpackRgbaToFloat(texture2D(_sampler, texCoord.xy) ) );

    //vec3 projCoords = _shadowCoord.xyz / _shadowCoord.w;
   // projCoords = projCoords* 0.5 + 0.5;

   // float lightingDepth = unpackRgbaToFloat(texture2D(_sampler,projCoords.xy) ) ;
  //  float currentDepth = projCoords.z;

	//vec3 lightDir = normalize(-lightPos.xyz);
	//vec3 normal = normalize(v_normal);
	//float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

   // float shadow = currentDepth - _bias > lightingDepth  ? 1.0 : 0.0;
	//return shadow;

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

	for(float x = -1.5; x <= 1.5; ++x)
    {
        for(float y = -1.5; y <= 1.5; ++y)
        {
            result += hardShadow(_sampler, _shadowCoord + vec4(vec2(x, y) * offset, 0.0, 0.0), _bias);
        }    
    }
	return result / 16.0;
	
	//for(float x = -0.5; x <= 1.5; ++x)
   // {
    //    for(float y = -0.5; y <= 1.5; ++y)
   //     {
   //         result += hardShadow(_sampler, _shadowCoord + vec4(vec2(x, y) * offset, 0.0, 0.0), _bias);
   //     }    
   // }
	//return result / 9.0;


	//for(float x = -0.5; x <= 0.5; ++x)
  //  {
    //    for(float y = -0.5; y <= 0.5; ++y)
   //     {
   //         result += hardShadow(_sampler, _shadowCoord + vec4(vec2(x, y) * offset, 0.0, 0.0), _bias);
   //     }    
   // }
	//return result / 4.0;
}


void main()
{	
//tolinear color 
	vec4 texcolor = texture2D(s_texColor, vec2(v_texcoord0.x,  -v_texcoord0.y));
//	vec3 albedo = texcolor.rgb;
	vec3 albedo = toLinear(texcolor.rgb);

	vec3 ao_rou_meta = texture2D(s_ao_rou_meta, vec2(v_texcoord0.x,  -v_texcoord0.y)).xyz;
	//ao_rou_meta = toLinear(ao_rou_meta);

	float ao = ao_rou_meta.x;
	float roughness = ao_rou_meta.y;
	float metallic = ao_rou_meta.z;

	float shadowMapBias = 0.02;
	vec2 texelSize = vec2_splat(1.0/512.0);
	float visibility =1.0;

	vec3 normal = normalize(v_normal);

	if(normalMap == 1.0){

    //vec3 normal;
	normal = texture2D(s_texnormal, v_texcoord0).xyz * 2.0 - 1.0;
	mat3 TBN = mat3(v_tangent ,v_bitangent, v_normal);

	#if BGFX_SHADER_LANGUAGE_HLSL > 0
		TBN = transpose(TBN);
	#endif

	normal = normalize(mul(TBN , normal));
	//normal = normalize(normal);

	}
	


	vec3 lightcolor = vec3(1.0, 1.0, 1.0);
	vec3 lightDir = vec3(1.0, 1.0, 1.0);
	lightDir = normalize(-lightPos.xyz);
	vec3 li_radiance = vec3(1.0, 1.0, 1.0);

	//vec3 normal = normalize(v_normal);
	vec3 viewdir = normalize(u_camPos - v_pos);

	float ndotv = max(dot(normal, viewdir),0.0);
	float ndotl = max(dot(normal, lightDir),0.0);
	vec3 halfwayDir = normalize(lightDir + viewdir);  
	float ndoth = max(dot(normal, halfwayDir),0.0);
	float vdoth = max(dot(viewdir, halfwayDir),0.0);

	//
	// blinn phong  + lambert
	//
	if(blinn_phong == 1.0){
	// blinn phong
	float diff = max(0.0, ndotl);
	float spec = max(0.0, ndoth);
	gl_FragColor.xyz = (Blinn_Phong.x + Blinn_Phong.y*lightcolor*diff*visibility + Blinn_Phong.z*lightcolor*pow(spec, Blinn_Phong.w)*visibility) *texcolor.xyz;
	gl_FragColor.w = 1.0;
	//gl_FragColor.xy = LUT;
	//gl_FragColor.zw = vec2(0.0,1.0);
	}


	//
	// pbr + simple env lighting 
	//
	if(pbr == 1.0){
	//pbr
    vec3 F0 = vec3_splat(0.04); 
    F0 = mix(F0, albedo, metallic);

    //brdf
    float NDF = DistributionGGX(ndoth, roughness);   
    float G   = GeometrySmith(ndotv, ndotl , roughness);   
    vec3 F = fresnelSchlick(vdoth, F0); 
    //pbr spec
    vec3 DGF = NDF * G * F; 
    float denominator = 4.0 * ndotv * ndotl + 0.0001; 
    vec3 specular = DGF / denominator;
 
    vec3 kS = F;
    vec3 kD = ( vec3_splat(1.0) - kS )*( 1.0 - metallic );
    vec3 direct_Diffuse = (kD * albedo / 3.1415926 ) * li_radiance * ndotl*u_doDiffuse *1.8; 
	vec3 direct_doSpecular = specular* li_radiance * ndotl*u_doSpecular*1.8 ;

    vec3 direct = direct_Diffuse + direct_doSpecular;
	vec3 ambient = vec3_splat(0.2) * albedo * ao;

	// Color.
	vec3 color = ambient + direct*visibility ;
	// HDR tonemapping
    color = color / (color + vec3_splat(1.0));
    // gamma correct
    color = pow(color, vec3_splat(1.0/2.2)); 
	gl_FragColor.xyz = color;
	gl_FragColor.w = 1.0;
	}
	
	//
	// pbr + ibl
	//
	if(pbr_ibl == 1.0){
	//pbr
    vec3 F0 = vec3_splat(0.04); 
    F0 = mix(F0, albedo, metallic);

    //brdf
    float NDF = DistributionGGX(ndoth, roughness);   
    float G   = GeometrySmith(ndotv, ndotl , roughness);   
    vec3 F = fresnelSchlick(vdoth, F0); 
    //pbr spec
    vec3 DGF = NDF * G * F; 
    float denominator = 4.0 * ndotv * ndotl + 0.0001; 
    vec3 specular = DGF / denominator;
 
    vec3 kS = F;
    vec3 kD = ( vec3_splat(1.0) - kS )*( 1.0 - metallic );
    vec3 direct_Diffuse = (kD * albedo / 3.1415926) * li_radiance * ndotl*u_doDiffuse*1.8 ; 
	vec3 direct_doSpecular = specular* li_radiance * ndotl*u_doSpecular*1.8 ;

    vec3 direct = direct_Diffuse + direct_doSpecular;

	//ibl
    float mip = 1.0 + 5.0*roughness; // Use mip levels [1..6] for radiance
	vec3 envFresnel = fresnelSchlickRoughness(ndotv,  F0, roughness);

	mat4 mtx;
	mtx[0] = u_mtx0;
	mtx[1] = u_mtx1;
	mtx[2] = u_mtx2;
	mtx[3] = u_mtx3;
	vec3 vr = 2.0*ndotv*normal - viewdir; // Same as: -reflect(vv, nn);
	vec3 cubeR = normalize(instMul(mtx, vec4(vr, 0.0)).xyz);
	vec3 cubeN = normalize(instMul(mtx, vec4(normal, 0.0)).xyz);
	cubeR = fixCubeLookup(cubeR, mip, 256.0);

	
	vec3 irradiance  = toLinear(textureCube(s_texCubeIrr, cubeN).xyz);
	vec3 anbient_Diffuse  = albedo     * irradiance * u_doDiffuseIbl;
    //vec3 envSpecular = envFresnel * radiance   * u_doSpecularIbl;
	//vec3 indirect    = envDiffuse + envSpecular;
	//  LUT calcu

	vec3 prefilterMap    = toLinear(textureCubeLod(s_texCube, cubeR, mip).xyz)  ;
	vec2 BRDF_LUT = texture2D(s_texLUT,vec2(ndotv,  1-roughness)).xy  ;

	vec3 anbient_Specular ;

	//if(doPrefilterMap == 1.0 && doBrdfLut == 1.0){
	//anbient_Specular = prefilterMap* (envFresnel * BRDF_LUT.x + BRDF_LUT.y) ;
	//}
	//if(doPrefilterMap == 1.0 && doBrdfLut == 0.0){
	//anbient_Specular =  prefilterMap*envFresnel ;
	//}
	//if(doPrefilterMap == 0.0 && doBrdfLut == 1.0){
	//anbient_Specular =  (envFresnel * BRDF_LUT.x + BRDF_LUT.y) ;
	//}
	 #if BGFX_SHADER_LANGUAGE_GLSL > 0
	anbient_Specular = prefilterMap* (envFresnel * BRDF_LUT.x + BRDF_LUT.y) ;
	#endif
	#if BGFX_SHADER_LANGUAGE_HLSL > 0
	anbient_Specular = prefilterMap* envFresnel  ;
	#endif
	vec3 anbient = (kD * anbient_Diffuse + anbient_Specular* u_doSpecularIbl) * ao;
	// anbient + direct
	vec3 color = anbient + direct *visibility;
	//  tonemapping
    color = color / (color + vec3_splat(1.0));
    // gamma correct
    color = pow(color, vec3_splat(1.0/2.2)); 

	gl_FragColor.xyz = color;
	gl_FragColor.w = 1.0;
	}
}
