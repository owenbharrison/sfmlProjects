#include <fstream>
#include <regex>
#include <sstream>

#include <list>

#include <SFML/Graphics.hpp>
using namespace sf;

#define PI 3.1415926f
#define EPSILON 0.0001f

#define NEAR_PLANE 0.001f
#define FAR_PLANE 100.f

#include "math/matrix.h"
#include "math/vec3d.h"
using namespace displib;
typedef matrix<4, 4> mat4;

struct triIndex {
	int a, b, c;
};
struct triangle {
	vec3d p[3];
	Color col=Color::White;
	float z[3]{0.f};
	float shade=0.f;
};

float MIN(float a, float b) {
	return a<b?a:b;
}
float MAX(float a, float b) {
	return a>b?a:b;
}
float CLAMP(float t, float a, float b) {
	return MIN(MAX(t, a), b);
}
float LERP(float a, float b, float t) {
	return a+t*(b-a);
}
float INVLERP(float a, float b, float t) {
	return (t-a)/(b-a);
}

struct mesh {
	std::vector<triangle> tris;

	//load obj file
	bool loadFromFile(std::string filename) {
		std::ifstream file(filename);
		std::regex indexRegex("([0-9]+)[/0-9]*");
		if (!file.is_open()) return false;

		std::vector<vec3d> vtxs;
		std::vector<triIndex> tIxs;

		//get extreme points
		float nx=INFINITY, ny=INFINITY, nz=INFINITY;
		float mx=-INFINITY, my=-INFINITY, mz=-INFINITY;
		//go through each line of the file
		while (!file.eof()) {
			std::string line;
			getline(file, line);
			std::stringstream stream;
			stream<<line;
			char junk;

			//if the line is in vertex format
			if (line.find("v ")!=std::string::npos) {
				float x, y, z;
				stream>>junk>>x>>y>>z;
				nx=MIN(nx, x), ny=MIN(ny, y), nz=MIN(nz, z);
				mx=MAX(mx, x), my=MAX(my, y), mz=MAX(mz, z);
				vtxs.push_back({x, y, z});
			}

			//if the line is in face format
			if (line.find("f ")!=std::string::npos) {
				//find all indexes
				std::vector<int> indexes;
				for (auto iter=std::sregex_iterator(line.begin(), line.end(), indexRegex);
					iter!=std::sregex_iterator(); iter++) {
					std::smatch match;
					match=*iter;
					indexes.push_back(stoi(match.str(1))-1);
				}
				//simple triangulation of face
				for (int i=1; i<indexes.size()-1; i++) {
					tIxs.push_back({indexes[0], indexes[i], indexes[i+1]});
				}
			}
		}

		//center vtxs about (0, 0, 0)
		vec3d modelMid=vec3d(nx+mx, ny+my, nz+mz)/2;
		for (auto& v:vtxs) v-=modelMid;

		//get biggest dimension, in one dir (so on 2)
		float maxDim=MAX(mx-nx, MAX(my-ny, mz-nz))/2;
		//divide by that, "normalize"?
		for (auto& v:vtxs) v/=maxDim;

		//use real tri ix info
		for (auto& tIx:tIxs) tris.push_back({vtxs[tIx.a], vtxs[tIx.b], vtxs[tIx.c]});

		return true;
	}
};

vec3d operator*(vec3d v, mat4 m) {
	auto prod=matrix<1, 4>({v.x, v.y, v.z, v.w})*m;
	return {prod.v[0][0], prod.v[0][1], prod.v[0][2], prod.v[0][3]};
}

vec3d vecIntersectPlane(vec3d planeP, vec3d planeN, vec3d l0, vec3d l1) {
	planeN=normalize(planeN);
	float planeD=-dot(planeN, planeP);
	float ad=dot(l0, planeN);
	float bd=dot(l1, planeN);
	float t=(-planeD-ad)/(bd-ad);
	return l0+t*(l1-l0);
}

int triangleClipAgainstPlane(vec3d planeP, vec3d planeN, triangle& in, triangle& out1, triangle& out2) {
	planeN=normalize(planeN);

	auto dist=[&](vec3d& p) {
		return dot(planeN, p)-dot(planeN, planeP);
	};

	vec3d* insidePts[3]; int insidePtCt=0;
	vec3d* outsidePts[3]; int outsidePtCt=0;

	for (int i=0; i<3; i++) {
		vec3d &p=in.p[i];
		if (dist(p)>=0.f) insidePts[insidePtCt++]=&p;
		else outsidePts[outsidePtCt++]=&p;
	}

	switch (insidePtCt) {
		case 0: return 0;
		case 1:
			out1.col=in.col;
			out1.shade=in.shade;

			out1.p[0]=*insidePts[0];
			out1.p[1]=vecIntersectPlane(planeP, planeN, *insidePts[0], *outsidePts[0]);
			out1.p[2]=vecIntersectPlane(planeP, planeN, *insidePts[0], *outsidePts[1]);
			return 1;
		case 2:
			out1.col=in.col;
			out1.shade=in.shade;

			out1.p[0]=*insidePts[0];
			out1.p[1]=*insidePts[1];
			out1.p[2]=vecIntersectPlane(planeP, planeN, *insidePts[0], *outsidePts[0]);

			out2.col=in.col;
			out2.shade=in.shade;

			out2.p[0]=*insidePts[1];
			out2.p[1]=out1.p[2];
			out2.p[2]=vecIntersectPlane(planeP, planeN, *insidePts[1], *outsidePts[0]);
			return 2;
		case 3: out1=in; return 1;
	}
}

mat4 createIdentityMatrix() {
	return {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

mat4 createXRotMatrix(float theta) {
	return {
		1.f, 0.f, 0.f, 0.f,
		0.f, cosf(theta), sinf(theta), 0.f,
		0.f, -sinf(theta), cosf(theta), 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

mat4 createYRotMatrix(float theta) {
	return {
		cosf(theta), 0.f, sinf(theta), 0.f,
		0.f, 1.f, 0.f, 0.f,
		-sinf(theta), 0.f, cosf(theta), 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

mat4 createZRotMatrix(float theta) {
	return {
		cosf(theta), sinf(theta), 0.f, 0.f,
		-sinf(theta), cosf(theta), 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
}

mat4 createTranslationMatrix(float x, float y, float z) {
	return {
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		x, y, z, 1.f
	};
}

mat4 createProjectionMatrix(float degrees, float aspect, float near, float far) {
	float fovRad=1.f/tanf(degrees*.5f/180.f*PI);
	return {
		aspect*fovRad, 0.f, 0.f, 0.f,
		0.f, fovRad, 0.f, 0.f,
		0.f, 0.f, far/(far-near), 1.f,
		0.f, 0.f, (-far*near)/(far-near), 0.f
	};
}

mat4 createPointAtMatrix(vec3d pos, vec3d target, vec3d up) {
	vec3d newForward=normalize(target-pos);
	vec3d newUp=normalize(up-newForward*dot(up, newForward));
	vec3d newRight=cross(newUp, newForward);

	return {
		newRight.x, newRight.y, newRight.z, 0.f,
		newUp.x, newUp.y, newUp.z, 0.f,
		newForward.x, newForward.y, newForward.z, 0.f,
		pos.x, pos.y, pos.z, 1.f
	};
}

mat4 getQuickInverse(mat4 m) {
	mat4 res;
	res.v[0][0]=m.v[0][0], res.v[0][1]=m.v[1][0], res.v[0][2]=m.v[2][0], res.v[0][3]=0.f;
	res.v[1][0]=m.v[0][1], res.v[1][1]=m.v[1][1], res.v[1][2]=m.v[2][1], res.v[1][3]=0.f;
	res.v[2][0]=m.v[0][2], res.v[2][1]=m.v[1][2], res.v[2][2]=m.v[2][2], res.v[2][3]=0.f;
	res.v[3][0]=-(m.v[3][0]*res.v[0][0]+m.v[3][1]*res.v[1][0]+m.v[3][2]*res.v[2][0]);
	res.v[3][1]=-(m.v[3][0]*res.v[0][1]+m.v[3][1]*res.v[1][1]+m.v[3][2]*res.v[2][1]);
	res.v[3][2]=-(m.v[3][0]*res.v[0][2]+m.v[3][1]*res.v[1][2]+m.v[3][2]*res.v[2][2]);
	res.v[3][3]=1.f;
	return res;
}

int main() {
	const unsigned int uTestWidth=5;
	const unsigned int uTestHeight=5;

	//sfml setup
	const unsigned int uWidth=1000;
	const unsigned int uHeight=800;
	RenderWindow rWindow(
		VideoMode(Vector2u(uWidth, uHeight)), "3D Engine", Style::Titlebar|Style::Close);
	Clock cDeltaClock;

	//prog setup
	mesh mMainMesh;
	if (mMainMesh.loadFromFile("obj/table.obj")) {
		for (auto& tTri:mMainMesh.tris) {
			vec3d tNorm=normalize(cross(tTri.p[1]-tTri.p[0], tTri.p[2]-tTri.p[0]));
			float r=tNorm.x*.5+.5;
			float g=tNorm.y*.5+.5;
			float b=tNorm.z*.5+.5;
			tTri.col=Color(r*255, g*255, b*255);
		}
	} else {
		printf("Error: couldn't load file.");
		exit(1);
	}

	mat4 m4Proj=createProjectionMatrix(90.f, (float)uHeight/(float)uWidth, NEAR_PLANE, FAR_PLANE);

	vec3d vCamera, vLookDir;
	float fYaw=0.f, fPitch=PI*.5f;

	vec3d vLightDir=normalize({0.f, 1.f, -1.f});

	RenderTexture rtRender;
	if (!rtRender.create(Vector2u(uWidth, uHeight))) {
		printf("Error: couldn't create texture");
		exit(1);
	}
	Shader sShader1;
	if (!sShader1.loadFromFile("shader/halftone.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	sShader1.setUniform("resolution", Glsl::Vec2(uWidth, uHeight));
	Shader sShader2;
	if (!sShader2.loadFromFile("shader/gaussian.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	sShader2.setUniform("resolution", Glsl::Vec2(uWidth, uHeight));

	float* fDepthBuffer=new float[uWidth*uHeight];
	Image iPixels;
	iPixels.create(Vector2u(uWidth, uHeight));
	Texture tPixels;
	if (!tPixels.create(Vector2u(uWidth, uHeight))) {
		printf("Error: couldn't create texture");
		exit(1);
	}

	Texture tShaderTexture;
	if (!tShaderTexture.create(Vector2u(uWidth, uHeight))) {
		printf("Error: couldn't create texture");
		exit(1);
	}
	Sprite sShaderSprite(tShaderTexture);

	auto fillTriangle=[&](float fx1, float fy1, float z1, float fx2, float fy2, float z2, float fx3, float fy3, float z3, Color col=Color::White) {
		//horizontal line
		auto drawline=[&](int sx, int ex, int y) {
			if (y>=0&&y<uHeight) {
				float denom=(fy2-fy3)*(fx1-fx3)+(fx3-fx2)*(fy1-fy3);
				for (int i=sx; i<=ex; i++) {
					if (i>=0&&i<uWidth) {
						float w1=((fy2-fy3)*(i-fx3)+(fx3-fx2)*(y-fy3))/denom;
						float w2=((fy3-fy1)*(i-fx3)+(fx1-fx3)*(y-fy3))/denom;
						float w3=1.f-w1-w2;
						int nIndex=i+y*uWidth;
						float fIntZ=z1*w1+z2*w2+z3*w3;
						if (fIntZ<fDepthBuffer[nIndex]) {
							fDepthBuffer[nIndex]=fIntZ;
							iPixels.setPixel(Vector2u(i, y), col);
						}
					}
				}
			}
		};
		int x1=roundf(fx1), y1=roundf(fy1), x2=roundf(fx2), y2=roundf(fy2), x3=roundf(fx3), y3=roundf(fy3);
		int t1x, t2x, y, minx, maxx, t1xp, t2xp;
		bool changed1=false;
		bool changed2=false;
		int signx1, signx2, dx1, dy1, dx2, dy2;
		int e1, e2;
		if (y1>y2) {
			std::swap(y1, y2);
			std::swap(x1, x2);
		}
		if (y1>y3) {
			std::swap(y1, y3);
			std::swap(x1, x3);
		}
		if (y2>y3) {
			std::swap(y2, y3);
			std::swap(x2, x3);
		}
		t1x=t2x=x1;
		y=y1;
		dx1=(int)(x2-x1);
		if (dx1<0) {
			dx1=-dx1;
			signx1=-1;
		} else signx1=1;
		dy1=(int)(y2-y1);
		dx2=(int)(x3-x1);
		if (dx2<0) {
			dx2=-dx2;
			signx2=-1;
		} else signx2=1;
		dy2=(int)(y3-y1);
		if (dy1>dx1) {
			std::swap(dx1, dy1);
			changed1=true;
		}
		if (dy2>dx2) {
			std::swap(dy2, dx2);
			changed2=true;
		}
		e2=(int)(dx2>>1);
		if (y1==y2) goto next;
		e1=(int)(dx1>>1);
		for (int i=0; i<dx1;) {
			t1xp=0;
			t2xp=0;
			if (t1x<t2x) {
				minx=t1x;
				maxx=t2x;
			} else {
				minx=t2x;
				maxx=t1x;
			}
			while (i<dx1) {
				i++;
				e1+=dy1;
				while (e1>=dx1) {
					e1-=dx1;
					if (changed1) t1xp=signx1;
					else goto next1;
				}
				if (changed1) break;
				else t1x+=signx1;
			}
next1:
			while (true) {
				e2+=dy2;
				while (e2>=dx2) {
					e2-=dx2;
					if (changed2) t2xp=signx2;
					else goto next2;
				}
				if (changed2) break;
				else t2x+=signx2;
			}
next2:
			if (minx>t1x) minx=t1x;
			if (minx>t2x) minx=t2x;
			if (maxx<t1x) maxx=t1x;
			if (maxx<t2x) maxx=t2x;
			drawline(minx, maxx, y);
			if (!changed1) t1x+=signx1;
			t1x+=t1xp;
			if (!changed2) t2x+=signx2;
			t2x+=t2xp;
			y++;
			if (y==y2) break;
		}
next:
		dx1=(int)(x3-x2);
		if (dx1<0) {
			dx1=-dx1;
			signx1=-1;
		} else signx1=1;
		dy1=(int)(y3-y2);
		t1x=x2;
		if (dy1>dx1) {
			std::swap(dy1, dx1);
			changed1=true;
		} else changed1=false;
		e1=(int)(dx1>>1);
		for (int i=0; i<=dx1; i++) {
			t1xp=0;
			t2xp=0;
			if (t1x<t2x) {
				minx=t1x;
				maxx=t2x;
			} else {
				minx=t2x;
				maxx=t1x;
			}
			while (i<dx1) {
				e1+=dy1;
				while (e1>=dx1) {
					e1-=dx1;
					if (changed1) {
						t1xp=signx1;
						break;
					} else goto next3;
				}
				if (changed1) break;
				else t1x+=signx1;
				if (i<dx1) i++;
			}
next3:
			while (t2x!=x3) {
				e2+=dy2;
				while (e2>=dx2) {
					e2-=dx2;
					if (changed2) t2xp=signx2;
					else goto next4;
				}
				if (changed2) break;
				else t2x+=signx2;
			}
next4:
			if (minx>t1x) minx=t1x;
			if (minx>t2x) minx=t2x;
			if (maxx<t1x) maxx=t1x;
			if (maxx<t2x) maxx=t2x;
			drawline(minx, maxx, y);
			if (!changed1) t1x+=signx1;
			t1x+=t1xp;
			if (!changed2) t2x+=signx2;
			t2x+=t2xp;
			y++;
			if (y>y3) return;
		}
	};
	//loop
	while (rWindow.isOpen()) {
		//polling
		for (Event event; rWindow.pollEvent(event);) {
			if (event.type==Event::Closed) rWindow.close();
		}

		//update
		float deltaTime=cDeltaClock.restart().asSeconds();
		//rWindow.setTitle("3D Engine @ "+std::to_string(int(1.f/deltaTime))+"fps");

		//look
		if (Keyboard::isKeyPressed(Keyboard::Enter)) {
			vLightDir=normalize(vCamera);
		}

		if (Keyboard::isKeyPressed(Keyboard::Left)) fYaw+=deltaTime;
		if (Keyboard::isKeyPressed(Keyboard::Right)) fYaw-=deltaTime;
		if (Keyboard::isKeyPressed(Keyboard::Up)) fPitch-=deltaTime;
		if (Keyboard::isKeyPressed(Keyboard::Down)) fPitch+=deltaTime;
		if (fPitch>PI-EPSILON) fPitch=PI-EPSILON;
		if (fPitch<EPSILON) fPitch=EPSILON;

		//move
		if (Keyboard::isKeyPressed(Keyboard::Space)) vCamera.y+=.8f*deltaTime;
		if (Keyboard::isKeyPressed(Keyboard::LShift)) vCamera.y-=.8f*deltaTime;
		vec3d vFb={cosf(fYaw), 0.f, sinf(fYaw)};
		if (Keyboard::isKeyPressed(Keyboard::W)) vCamera+=vFb*deltaTime;
		if (Keyboard::isKeyPressed(Keyboard::S)) vCamera-=.4f*vFb*deltaTime;
		vec3d vLr={sinf(fYaw), 0.f, -cosf(fYaw)};
		if (Keyboard::isKeyPressed(Keyboard::A)) vCamera-=.6f*vLr*deltaTime;
		if (Keyboard::isKeyPressed(Keyboard::D)) vCamera+=.6f*vLr*deltaTime;

		//crazy stuff starts
		mat4 m4World=createIdentityMatrix();

		vec3d vUp{0.f, 1.f, 0.f};
		vLookDir={sinf(fPitch)*cosf(fYaw), cosf(fPitch), sinf(fPitch)*sinf(fYaw)};
		vec3d vTarget=vCamera+vLookDir;

		mat4 m4Camera=createPointAtMatrix(vCamera, vTarget, vUp);
		mat4 m4View=getQuickInverse(m4Camera);

		//cull triangles
		std::list<triangle> lTrisToDraw;
		for (auto& tTri:mMainMesh.tris) {
			triangle tProj, tTrans, tViewed;

			for (int i=0; i<3; i++) {
				tTrans.p[i]=tTri.p[i]*m4World;
			}
			tTrans.col=tTri.col;

			//culling
			vec3d vNormal=normalize(cross(tTrans.p[1]-tTrans.p[0], tTrans.p[2]-tTrans.p[0]));
			if (dot(vNormal, tTrans.p[0]-vCamera)<0.f) {
				float diffuse=dot(vNormal, vLightDir);
				tTrans.shade=CLAMP(diffuse, .15f, 1);

				for (int i=0; i<3; i++) {
					tViewed.p[i]=tTrans.p[i]*m4View;
				}
				tViewed.col=tTrans.col;
				tViewed.shade=tTrans.shade;

				int clippedTris=0;
				triangle clipped[2];
				clippedTris=triangleClipAgainstPlane({0.f, 0.f, NEAR_PLANE}, {0.f, 0.f, 1.f}, tViewed, clipped[0], clipped[1]);

				for (int n=0; n<clippedTris; n++) {
					for (int i=0; i<3; i++) {
						tProj.p[i]=clipped[n].p[i]*m4Proj;

						tProj.p[i]/=tProj.p[i].w;

						//scale into view
						vec3d offset{1, 1, 0};
						tProj.p[i]+=offset;
						tProj.p[i].x*=.5f*uWidth;
						tProj.p[i].y*=.5f*uHeight;
					}
					tProj.col=clipped[n].col;
					tProj.shade=clipped[n].shade;

					lTrisToDraw.push_back(tProj);
				}
			}
		}

		//clear "buffers"
		for (int i=0; i<uWidth*uHeight; i++) fDepthBuffer[i]=FAR_PLANE;
		for (int i=0; i<uWidth; i++) {
			for (int j=0; j<uHeight; j++) {
				iPixels.setPixel(Vector2u(i, j), Color(51, 51, 51));
			}
		}
		//for each tri
		for (const auto& tToRaster:lTrisToDraw) {
			triangle tClipped[2];
			std::list<triangle> lTriangles{tToRaster};
			int nNewTriangles=1;
			//clip against 4 planes.
			for (int p=0; p<4; p++) {
				int nTrisToAdd=0;
				while (nNewTriangles>0) {
					//take tri from front of queue
					triangle tTest=lTriangles.front();
					lTriangles.pop_front();
					nNewTriangles--;

					//clip it against plane
					switch (p) {
						case 0:	nTrisToAdd=triangleClipAgainstPlane({0.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, tTest, tClipped[0], tClipped[1]); break;
						case 1:	nTrisToAdd=triangleClipAgainstPlane({0.f, (float)uHeight-1, 0.f}, {0.f, -1.f, 0.f}, tTest, tClipped[0], tClipped[1]); break;
						case 2:	nTrisToAdd=triangleClipAgainstPlane({0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, tTest, tClipped[0], tClipped[1]); break;
						case 3:	nTrisToAdd=triangleClipAgainstPlane({(float)uWidth-1, 0.f, 0.f}, {-1.f, 0.f, 0.f}, tTest, tClipped[0], tClipped[1]); break;
					}

					//add new tris to queue
					for (int w=0; w<nTrisToAdd; w++) lTriangles.push_back(tClipped[w]);
				}
				nNewTriangles=lTriangles.size();
			}

			for (auto& tTri:lTriangles) {
				fillTriangle(
					tTri.p[0].x, tTri.p[0].y, tTri.p[0].z,
					tTri.p[1].x, tTri.p[1].y, tTri.p[1].z,
					tTri.p[2].x, tTri.p[2].y, tTri.p[2].z,
					Color(
						tTri.col.r*tTri.shade,
						tTri.col.g*tTri.shade,
						tTri.col.b*tTri.shade
					)
				);
			}
		}
		float minDepth=0.f, maxDepth=0.f;
		for (int i=0; i<uWidth*uHeight; i++) {
			float d=fDepthBuffer[i];
			if (i==0||d<minDepth) minDepth=d;
			if (i==0||d>maxDepth) maxDepth=d;
		}
		for (int i=0; i<uWidth; i++) {
			for (int j=0; j<uHeight; j++) {
				int shade=INVLERP(minDepth, maxDepth, fDepthBuffer[i+j*uWidth])*255;
				//iPixels.setPixel(Vector2u(i, j), Color(shade, shade, shade));
			}
		}
		tPixels.update(iPixels);
		sShader1.setUniform("mainTex", tPixels);

		rtRender.clear();
		rtRender.draw(sShaderSprite, &sShader1);
		//rtRender.display();
		sShader2.setUniform("mainTex", rtRender.getTexture());

		rWindow.clear();
		rWindow.draw(sShaderSprite, &sShader2);
		rWindow.display();
	}

	return 0;
}