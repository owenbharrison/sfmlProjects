#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

float gaussian[25]=float[25](
	1., 4., 7., 4., 1.,
	4., 16., 26., 16., 4.,
	7., 26., 41., 26., 7.,
	4., 16., 26., 16., 4.,
	1., 4., 7., 4., 1.
);

void main() {
	vec3 sum=vec3(0.);
	for(int i=0;i<5;i++){
		for(int j=0;j<5;j++){
			vec2 uv=(gl_FragCoord.xy+vec2(i-2, j-2))/resolution.xy;
			sum+=texture(mainTex, uv).rgb*gaussian[i+j*5];
		}
	}
	sum/=273.;
	
	gl_FragColor=vec4(sum, 1.);
}