$input v_position


#include "../common/common.sh"

uniform vec4 u_depthScaleOffset;  
#define Scale u_depthScaleOffset.x
#define Offset u_depthScaleOffset.y

void main()
{
	float depth = v_position.z/v_position.w * Scale + Offset;
	gl_FragColor = packFloatToRgba(depth);
}
