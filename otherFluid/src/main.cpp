#include <SFML/Graphics.hpp>
using namespace sf;

#include "fluid.h"

#define DENSITY 1000

#define ITERATIONS 40

#define OBSTACLE_X 25

#define OBSTACLE_Y height/2+1

#define OBSTACLE_RADIUS 13

#define IN_VEL 2

#define RANDOM (rand()/32767.f)

float clamp(float x, float a, float b) {
	return (x<a)?a:((x>b)?b:x);
}

float invLerp(float v, float a, float b) {
	return (v-a)/(b-a);
}

struct vec3 {
	float x=0.f, y=0.f, z=0.f;

	vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	vec3 operator*(float f) {
		return {x*f, y*f, z*f};
	}

	vec3 operator+(float f){
		return {x+f, y+f, z+f};
	}
};

float dot(vec3 a, vec3 b){
	return a.x*b.x+a.y*b.y+a.z*b.z;
}

float fract(float f) {
	return f-(int)f;
}

vec3 fract(vec3 v) {
	return {fract(v.x), fract(v.y), fract(v.z)};
}

float hash12(float x, float y) {
	vec3 p3=fract(vec3(x, y, x)*.1031f);
	p3=p3+dot(p3, vec3(p3.y, p3.z, p3.x)+33.33f);
	return fract((p3.x+p3.y)*p3.z);
}

int main() {
	srand(time(NULL));

	//sfml setup
	unsigned int width=400;
	unsigned int height=200;
	Vector2u size(width, height);
	RenderWindow window(VideoMode(size), "", Style::Titlebar|Style::Close);
	window.setFramerateLimit(165);
	Clock deltaClock;

	//shaders
	RenderTexture rtRender;
	if (!rtRender.create(size)) {
		printf("Error: couldn't create texture");
		exit(1);
	}
	Shader sShader1;
	if (!sShader1.loadFromFile("lineIntegralConvolution.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	sShader1.setUniform("Resolution", Glsl::Vec2(width, height));
	Texture tShaderTexture;
	if (!tShaderTexture.create(size)) {
		printf("Error: couldn't create texture");
		exit(1);
	}
	Sprite sShaderSprite(tShaderTexture);
	Image iPixels;
	iPixels.create(size);
	Texture tPixels;
	if (!tPixels.create(size)) {
		printf("Error: couldn't create texture");
		exit(1);
	}

	//basic prog setup
	fluid mainFluid(DENSITY, width, height, 1.f/width);

	auto resetFluid=[&](){
		for (int i=0; i<mainFluid.sz; i++) {
			mainFluid.smoke[i]=1;
			mainFluid.u[i]=0;
			mainFluid.v[i]=0;
			mainFluid.newU[i]=0;
			mainFluid.newV[i]=0;
		}
	};
	resetFluid();

	for (int i=0; i<mainFluid.w; i++) {
		for (int j=0; j<mainFluid.h; j++) {
			mainFluid.fluidity[mainFluid.ix(i, j)]=!(i==0||j==0||j==mainFluid.h-1);

			if (i==1) {
				mainFluid.u[mainFluid.ix(i, j)]=IN_VEL;
			}
		}
	}

	for (int i=height/3; i<height*2/3; i++) {
		mainFluid.smoke[mainFluid.ix(1, i)]=0;
	}

	//setup circle
	for (int i=0; i<mainFluid.w; i++) {
		for (int j=0; j<mainFluid.h; j++) {
			bool& current=mainFluid.fluidity[mainFluid.ix(i, j)];
			if (current) {
				int dx=i-OBSTACLE_X;
				int dy=j-OBSTACLE_Y;
				current=dx*dx+dy*dy>OBSTACLE_RADIUS*OBSTACLE_RADIUS;
			}
		}
	}

	//loop
	while (window.isOpen()) {
		Vector2i mp=Mouse::getPosition(window);
		int mouseX=mp.x, mouseY=mp.y;
		//polling
		for (Event event; window.pollEvent(event);) {
			if (event.type==Event::Closed) window.close();
		}

		//update
		float deltaTime=deltaClock.restart().asSeconds();

		//wind tunnel
		for (int i=0; i<mainFluid.w; i++) {
			mainFluid.u[mainFluid.ix(1, i)]=IN_VEL;

			if (i>height/3&&i<height*2/3) {
				mainFluid.smoke[mainFluid.ix(1, i)]=0;
			}
		}

		mainFluid.simulate(deltaTime, ITERATIONS);

		bool add=Keyboard::isKeyPressed(Keyboard::A);
		bool del=Keyboard::isKeyPressed(Keyboard::D);
		bool clear=Keyboard::isKeyPressed(Keyboard::C);
		if (add||del||clear) {
			if (clear) {
				for (int i=1; i<mainFluid.w-1; i++) {
					for (int j=1; j<mainFluid.h-1; j++) {
						mainFluid.fluidity[mainFluid.ix(i, j)]=true;
					}
				}
			}
			for (int i=-4; i<=4; i++) {
				for (int j=-4; j<=4; j++) {
					int x=mouseX+i, y=mouseY+j;
					if (x>0&&y>0&&x<mainFluid.w-1&&y<mainFluid.h-1) {
						bool& current=mainFluid.fluidity[mainFluid.ix(x, y)];
						if (add) current=false;
						if (del) current=true;
					}
				}
			}

			resetFluid();
		}

		std::string fpsStr=std::to_string((int)(1/deltaTime));
		window.setTitle("Fluid Sim @ "+fpsStr+"fps");

		//render
		float minP=INFINITY, maxP=-INFINITY;
		for (int i=0; i<mainFluid.sz; i++) {
			float p=mainFluid.pressure[i];
			minP=MIN(minP, p); maxP=MAX(maxP, p);
		}
		for (int i=0; i<width; i++) {
			for (int j=0; j<height; j++) {
				Vector2u coords(i, j);
				int index=mainFluid.ix(i, j);
				bool wall=mainFluid.fluidity[index];
				if (wall) {
					float vx=mainFluid.u[index];
					float vy=mainFluid.v[index];
					float velMag=sqrtf(vx*vx+vy*vy);
					vx/=velMag, vy/=velMag;
					int vxVal=clamp(vx*127+127, 0, 255), vyVal=clamp(vy*127+127, 0, 255);
					int smokeVal=clamp(mainFluid.smoke[index]*255, 0, 255);
					iPixels.setPixel(coords, Color(vxVal, vyVal, hash12(i, j)*255, smokeVal));
				}
				else iPixels.setPixel(coords, Color::Black);
			}
		}
		tPixels.update(iPixels);
		sShader1.setUniform("MainTex", tPixels);

		window.clear();
		window.draw(sShaderSprite, &sShader1);
		window.display();
	}

	return 0;
}