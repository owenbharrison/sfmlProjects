#version 400

#define PI 3.1415926

#define EPSILON .0001

#define INFINITY 1./0.

uniform int numSpheres;
uniform vec4 spheres[256];

uniform vec3 camPos, sunPos, qx, qy, p1m;

uniform sampler2D skybox;

vec2 dirToUV(vec3 dir){
	return vec2(.5+atan(dir.z, dir.x)/(2.*PI), .5-asin(dir.y)/PI);
}

float sphereIntersectRay(vec3 pos, float rad, vec3 origin, vec3 dir) {
	vec3 oc=origin-pos;
	float a=dot(dir, dir);
	float b=2.*dot(dir, oc);
	float c=dot(oc, oc)-rad*rad;
	float disc=b*b-4.*a*c;
	if (disc<EPSILON) return -1.;
	//solve quadratic, + & -
	float sq=sqrt(disc);
	float num=-b-sq;
	if (num>EPSILON) return num/(2.*a);
	num=-b+sq;
	if (num>EPSILON) return num/(2.*a);
	return -1.;
}

vec3 traceRay(vec3 origin, vec3 dir){
	float record=INFINITY;
	int index=-1;
	for(int i=0;i<numSpheres;i++){
		vec4 s=spheres[i];
		float dist=sphereIntersectRay(s.xyz, s.w, origin, dir);
		if(dist!=-1.){
			if(dist<record){
				record=dist;
				index=i;
			}
		}
	}
	if(index!=-1) {
		//info abt hit.
		vec4 s=spheres[index];
		vec3 hitPos=origin+dir*record;
		vec3 sunDir=normalize(sunPos-hitPos);

		//shading
		vec3 surfaceNormal=normalize(hitPos-s.xyz);
		float diffuseShade=clamp(dot(sunDir, surfaceNormal), 0., 1.);
		return vec3(diffuseShade*.9+.1);
	}
	return texture(skybox, dirToUV(dir)).xyz;
}

void main(){
	vec3 pij=p1m+qx*gl_FragCoord.x+qy*gl_FragCoord.y;
	vec3 col=traceRay(camPos, normalize(pij));
	gl_FragColor=vec4(col, 1.);
}