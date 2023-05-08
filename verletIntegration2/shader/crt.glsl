#version 330

uniform vec2 Resolution;
uniform sampler2D MainTex;

#define CURVATURE 4.7

#define BLUR .021

#define CA_AMT 1.0075

void main(){
    vec2 uv=gl_FragCoord.xy/Resolution;
    
    //curving
    vec2 crtUV=uv*2.-1.;
    vec2 offset=crtUV.yx/CURVATURE;
    crtUV+=crtUV*offset*offset;
    crtUV=crtUV*.5+.5;
    
    vec2 edge=smoothstep(0., BLUR, crtUV)*(1.-smoothstep(1.-BLUR, 1., crtUV));
    
    //chromatic abberation
    gl_FragColor=vec4(
        texture(MainTex, (crtUV-.5)*CA_AMT+.5).r,
        texture(MainTex, crtUV).g,
        texture(MainTex, (crtUV-.5)/CA_AMT+.5).b,
				1.
    )*edge.x*edge.y;
    
    //lines
    if(mod(gl_FragCoord.y, 2.)<1.) gl_FragColor.rgb*=.7;
    else if(mod(gl_FragCoord.x, 3.)<1.) gl_FragColor.rgb*=.7;
    else gl_FragColor*=1.2;
}