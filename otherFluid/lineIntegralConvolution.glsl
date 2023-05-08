#version 330

uniform vec2 Resolution;
uniform sampler2D MainTex;

#define K 5//[3, 10]

#define GET_VECTOR(uv) (texture(MainTex, uv).xy*2.-1.)
#define GET_NOISE(uv) texture(MainTex, uv).z
#define GET_SMOKE(uv) texture(MainTex, uv).w

void main(){
		vec2 coord=vec2(gl_FragCoord.x, Resolution.y-gl_FragCoord.y);

    vec2 uv=coord/Resolution;
    //base color
		float smoke=GET_SMOKE(uv);
    gl_FragColor=vec4(1.-smoke, 0., smoke, 1.);
    
    //trace pt along line
		vec2 pt0=coord+.5, pt=pt0;
		vec2 dir0=GET_VECTOR(uv), dir=dir0;
		float sum=0.;
		for(int i=-K;i<=K;i++){
        //which way are we going?
        switch(sign(i)){
            case -1://behind: step back
                pt-=dir;
                break;
            case 0://middle: init for ahead
                pt=pt0;
                dir=dir0;
                break;
            case 1://ahead: step forward
                pt+=dir;
                break;
        }
        //sampling
        vec2 ptUV=pt/Resolution.xy;
        sum+=GET_NOISE(ptUV);
        dir=GET_VECTOR(ptUV);
    }
    
    //combine
    gl_FragColor.rgb*=sum/float(K);
}