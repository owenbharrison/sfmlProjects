#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

#define CA_AMT 1.02

void main() {
    vec2 uv=gl_FragCoord.xy/resolution;
    gl_FragColor=vec4(
        texture(mainTex, (uv-.5)*CA_AMT+.5).r,
        texture(mainTex, uv).g,
        texture(mainTex, (uv-.5)/CA_AMT+.5).b,
        1.
    );
}