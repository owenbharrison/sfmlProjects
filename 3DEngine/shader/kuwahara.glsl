#version 330

uniform vec2 resolution;
uniform sampler2D mainTex;

#define K 4

#define KERN_TEX(a, b) texture(mainTex, (gl_FragCoord.xy+vec2(i-a, j-b))/resolution).rgb

#define GRAY(a) dot(a, vec3(.299, .587, .114))

void main() {
    float sectorSz=float(K*K+2*K+1);
    
    vec3 u0=vec3(0.), u1=u0, u2=u0, u3=u0;
    for(int i=0;i<=K;i++){
        for(int j=0;j<=K;j++){
            u0+=KERN_TEX(K, K);
            u1+=KERN_TEX(0, K);
            u2+=KERN_TEX(K, 0);
            u3+=KERN_TEX(0, 0);
        }
    }
    u0/=sectorSz, u1/=sectorSz, u2/=sectorSz, u3/=sectorSz;
    vec4 u=vec4(GRAY(u0), GRAY(u1), GRAY(u2), GRAY(u3));
    
    vec4 std=vec4(0.);
    for(int i=0;i<=K;i++){
        for(int j=0;j<=K;j++){
            vec4 v=vec4(
                GRAY(KERN_TEX(K, K)),
                GRAY(KERN_TEX(0, K)),
                GRAY(KERN_TEX(K, 0)),
                GRAY(KERN_TEX(0, 0))
            )-u;
            std+=v*v;
        }
    }
    
    float m=min(std.x, min(std.y, min(std.z, std.w)));
    gl_FragColor=vec4(
        m==std.x ? u0 :
        m==std.y ? u1 : 
        m==std.z ? u2 :
        m==std.w ? u3 :
        (u0+u1+u2+u3)*.25,
        1.
    );
}