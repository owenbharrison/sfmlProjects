#include <SFML/Graphics.hpp>
using namespace sf;

#include "fluid.h"

#define DENSITY 1000

#define ITERATIONS 40

#define OBSTACLE_X 25

#define OBSTACLE_Y height/2+1

#define OBSTACLE_RADIUS 13

#define IN_VEL 2

float invLerp(float v, float a, float b) {
	return (v-a)/(b-a);
}

int main() {
	srand(time(NULL));

	//sfml setup
	unsigned int width=600;
	unsigned int height=400;
	RenderWindow window(VideoMode(Vector2u(width, height)), "", Style::Titlebar|Style::Close);
	window.setFramerateLimit(165);
	Clock deltaClock;

	//basic prog setup
	fluid mainFluid(DENSITY, width, height, 1.f/width);

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
		int  mouseX=mp.x, mouseY=mp.y;
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
			for (int i=-1; i<=1; i++) {
				for (int j=-1; j<=1; j++) {
					int x=mouseX+i, y=mouseY+j;
					if (x>0&&y>0&&x<mainFluid.w-1&&y<mainFluid.h-1) {
						bool& current=mainFluid.fluidity[mainFluid.ix(x, y)];
						if (add) current=false;
						if (del) current=true;
					}
				}
			}

			//reset
			for (int i=0; i<mainFluid.sz; i++) {
				mainFluid.smoke[i]=1;
				mainFluid.u[i]=0;
				mainFluid.v[i]=0;
				mainFluid.newU[i]=0;
				mainFluid.newV[i]=0;
			}
		}

		std::string fpsStr=std::to_string((int)(1/deltaTime));
		window.setTitle("Fluid Sim @ "+fpsStr+"fps");

		//render
		window.clear();
		window.display();
	}

	return 0;
}