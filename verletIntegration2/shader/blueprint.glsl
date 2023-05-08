#version 330

#define SIZE 20.

#define ANIM_SPEED .15

uniform float Time;
uniform vec2 Resolution;
uniform sampler2D MainTex;

//https://www.shadertoy.com/view/4djSRW
vec3 hash33(vec3 p3){
	p3=fract(p3*vec3(.1031, .1030, .0973));
	p3+=dot(p3, p3.yxz+33.33);
	return fract((p3.xxy+p3.yxx)*p3.zyx);
}

void main(){
		float timeOffset=round(Time/ANIM_SPEED);
		vec3 noise=hash33(vec3(gl_FragCoord.xy, timeOffset));
		
		vec2 coord=Resolution*.5-gl_FragCoord.xy;
		coord+=(noise.xy-.5)*.3;
		
		//background
		gl_FragColor=vec4(.07, .3, .7, 1.);
		//small lines
		vec2 a=smoothstep(.07, .0, abs(.5-fract(coord/SIZE)));
		gl_FragColor+=.25*(a.x+a.y);
		//big lines
		vec2 b=smoothstep(.01, .0, abs(.5-fract(coord/(SIZE*5.))));
		gl_FragColor+=.5*(b.x+b.y);
		
		//composite
		vec2 uv=gl_FragCoord.xy/Resolution;
		vec4 col=texture(MainTex, uv);
		gl_FragColor=mix(gl_FragColor, col, col.a);
		
		//more noise
		gl_FragColor+=(noise.x-.5)*.03;
		
		//add vignette https://www.shadertoy.com/view/lsKSWR
		uv*=1.-uv.yx;
		float vig=uv.x*uv.y*22.;
		gl_FragColor*=pow(vig, .05);
}