#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

#define MASK 1.1

int stipple[16]=int[16](
    1, 9, 3, 11,
    13, 5, 15, 7,
    4, 12, 2, 10,
    16, 8, 14, 6
);

void main() {
    vec2 uv=gl_FragCoord.xy/resolution;
    vec4 col=texture(mainTex, uv);
    
    int i=int(mod(gl_FragCoord.x, 4.));
    int j=int(mod(gl_FragCoord.y, 4.));
    vec3 thresh=vec3(
        stipple[(i+0)%4+((j+0)%4)*4],
        stipple[(i+1)%4+((j+1)%4)*4],
        stipple[(i+2)%4+((j+2)%4)*4]
    )/17.;
    gl_FragColor=vec4(step(thresh*MASK, col.rgb), 1.);
}