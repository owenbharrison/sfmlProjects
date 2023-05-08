#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

#define PI 3.1415926

#define MASK .7

#define HT_SIZE 9.7

#define HT_R_ANGLE PI*.45
#define HT_G_ANGLE PI*1.1
#define HT_B_ANGLE PI*1.91

#define HT_MIN .12
#define HT_MAX .8

vec2 rotVec(vec2 v, float f) {
	vec2 a=vec2(cos(f), sin(f));
	vec2 b=vec2(-a.y, a.x);
	return a*v.x+b*v.y;
}

void main() {
	//get cam col
	vec2 uv=gl_FragCoord.xy/resolution;
	vec4 col=texture(mainTex, uv);
	vec3 thresh=clamp(col.rgb*.5, .05, .45);
	
	//halftone stuff
	vec2 shrunk=gl_FragCoord.xy/HT_SIZE;
	vec3 colToUse=vec3(HT_MIN);
	
	//get r values
	vec2 rotatedR=rotVec(shrunk, HT_R_ANGLE);
	vec2 repeatR=fract(rotatedR);
	vec2 normalR=.5-repeatR;
	float distR=length(normalR)*MASK;
	if(distR<thresh.r) colToUse.r=HT_MAX;
	
	//get g values
	vec2 rotatedG=rotVec(shrunk, HT_G_ANGLE);
	vec2 repeatG=fract(rotatedG);
	vec2 normalG=.5-repeatG;
	float distG=length(normalG)*MASK;
	if(distG<thresh.g) colToUse.g=HT_MAX;
	
	//get b values
	vec2 rotatedB=rotVec(shrunk, HT_B_ANGLE);
	vec2 repeatB=fract(rotatedB);
	vec2 normalB=.5-repeatB;
	float distB=length(normalB)*MASK;
	if(distB<thresh.b) colToUse.b=HT_MAX;
	
	//combine
	gl_FragColor=vec4(colToUse, 1.);
}