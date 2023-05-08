#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

#define CURVATURE 3.9

#define BLUR .021

void main() {
    vec2 uv=gl_FragCoord.xy/resolution;
    
    //curving
    vec2 crtUV=uv*2.-1.;
    vec2 offset=crtUV.yx/CURVATURE;
    crtUV+=crtUV*offset*offset;
    crtUV=crtUV*.5+.5;
    
    vec2 edge=smoothstep(0., BLUR, crtUV)*(1.-smoothstep(1.-BLUR, 1., crtUV));
    
    gl_FragColor=texture(mainTex, crtUV)*edge.x*edge.y;
    
    //lines
    if(mod(gl_FragCoord.y, 2.)<1.) gl_FragColor.rgb*=.7;
    else if(mod(gl_FragCoord.x, 3.)<1.) gl_FragColor.rgb*=.7;
    else gl_FragColor*=1.2;
}