#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

#define PI 3.1415926

#define GRAY(a) dot(a, vec3(.299, .587, .114))

mat3 sobelX=mat3(vec3(-1., 0., 1.), vec3(-2., 0., 2.), vec3(-1., 0., 1.));
mat3 sobelY=mat3(vec3(-1., -2., -1.), vec3(0., 0., 0.), vec3(1., 2., 1.));

//red, orange, yellow, green, cyan, blue, violet, magneta
vec3 colorWheel(float angle){
    float r=cos(angle);
    float g=cos(angle-PI*.667);
    float b=cos(angle-PI*1.33);
    
    return vec3(r, g, b)*.5+.5;
}

void main() {
    //go thru each kernel
    float xDiff=0.;
    float yDiff=0.;
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            vec2 kernUV=(gl_FragCoord.xy+vec2(i-1, j-1))/resolution;
            vec3 kernCol=texture(mainTex, kernUV).rgb;
						float lum=GRAY(kernCol);
            xDiff+=sobelX[i][j]*lum;
            yDiff+=sobelY[i][j]*lum;
        }
    }
    
    //"total" diff
    float diff=length(vec2(xDiff, yDiff));
    
    float angle=atan(yDiff, xDiff);
    
    gl_FragColor=vec4(colorWheel(angle)*diff, 1.);
}