$input v_dir


#include "../common/common.sh"
#include "uniforms.sh"

SAMPLERCUBE(s_texCube, 0);
SAMPLERCUBE(s_texCubeIrr, 1);

void main()
{
	vec3 dir = normalize(v_dir);
	vec4 color;
	float lod = u_bgType;
	dir = fixCubeLookup(dir, lod, 256.0);
	color = toLinear(textureCubeLod(s_texCube, dir, lod));

	gl_FragColor = toFilmic(color);
}
