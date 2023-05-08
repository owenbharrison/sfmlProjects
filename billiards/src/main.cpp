#include <SFML/Graphics.hpp>
using namespace sf;

#include "vector/float2.h"

#define PI 3.14159267f

#define RANDOM (rand()/32767.f)

#define SCALE 8

#define FIELD_WIDTH (100*SCALE)
#define FIELD_HEIGHT (50*SCALE)

#define BALL_RADIUS (1.125f*SCALE)
#define POCKET_RADIUS (2.25f*SCALE)

// sz/sqrt(8)
#define CORNER_BUFFER (POCKET_RADIUS/2.8284271f)

#define GRAVITY 9.81f

#define RAIL_COR .75f

#define U_STATIC 0.01f

struct aabb {
	float2 min, max;

	bool containsPt(float2 pt) {
		bool xOverlap=pt.x>=min.x&&pt.x<=max.x;
		bool yOverlap=pt.y>=min.y&&pt.y<=max.y;
		return xOverlap&&yOverlap;
	}

	bool overlapAABB(aabb a) {
		bool xOverlap=min.x<=a.max.x&&max.x>=a.min.x;
		bool yOverlap=min.y<=a.max.y&&max.y>=a.min.y;
		return xOverlap&&yOverlap;
	}
};

struct ball {
	float2 pos, vel, acc;
	Color col=Color::White;

	void update(float dt) {
		vel+=acc*dt;
		pos+=vel*dt;

		acc*=0;
	}

	void applyForce(float2 f) {
		acc+=f;
	}

	void checkBounds(aabb a) {
		bool flag=false;
		if (pos.x<a.min.x+BALL_RADIUS) {
			pos.x=a.min.x+BALL_RADIUS;
			vel.x*=-1;
			flag=true;
		}
		if (pos.y<a.min.y+BALL_RADIUS) {
			pos.y=a.min.y+BALL_RADIUS;
			vel.y*=-1;
			flag=true;
		}
		if (pos.x>a.max.x-BALL_RADIUS) {
			pos.x=a.max.x-BALL_RADIUS;
			vel.x*=-1;
			flag=true;
		}
		if (pos.y>a.max.y-BALL_RADIUS) {
			pos.y=a.max.y-BALL_RADIUS;
			vel.y*=-1;
			flag=true;
		}
		if (flag) vel*=RAIL_COR*RAIL_COR;
	}

	aabb getAABB() {
		return {pos-BALL_RADIUS, pos+BALL_RADIUS};
	}
};

int main() {
	srand(time(NULL));

	//sfml setup
	unsigned int width=FIELD_WIDTH;
	unsigned int height=FIELD_HEIGHT;
	RenderWindow window(VideoMode(Vector2u(width, height)), "", Style::Titlebar|Style::Close);
	window.setFramerateLimit(165);
	Clock deltaClock;

	aabb bounds{float2(0), float2(width, height)};

	//basic prog setup
	ball balls[16];
	balls[0].pos=float2(FIELD_WIDTH*.25f, FIELD_HEIGHT*.5f);
	balls[0].vel.x=RANDOM*250.f+150.f;
	balls[0].vel.y=RANDOM*6.f-3.f;
	for (int c=0, i=1; c<5; c++) {
		float x=FIELD_WIDTH*.75f+c*BALL_RADIUS*1.7320508f;
		for (int r=0; r<=c; r++, i++) {
			float y=FIELD_HEIGHT*.5f+(2.f*r-c)*BALL_RADIUS;
			balls[i].pos=float2(x, y);
		}
	}

	//loop
	auto drawLine=[&window](float2 a, float2 b, Color col=Color::White) {
		float2 ba=b-a;
		RectangleShape line(Vector2f(length(ba), 2));
		line.setOrigin(Vector2f(0, 1));
		line.setRotation(radians(atan2(ba.y, ba.x)));
		line.setPosition(Vector2f(a.x, a.y));

		line.setFillColor(col);
		window.draw(line);
	};
	auto drawCircle=[&window](float2 p, float r, Color col=Color::White) {
		CircleShape circ(r);
		circ.setOutlineThickness(-2);
		circ.setOutlineColor(col);
		circ.setFillColor(Color::Transparent);

		circ.setOrigin(Vector2f(r, r));
		circ.setPosition(Vector2f(p.x, p.y));
		window.draw(circ);
	};
	auto fillCircle=[&window](float2 p, float r, Color col=Color::White) {
		CircleShape circ(r);
		circ.setFillColor(col);

		circ.setOrigin(Vector2f(r, r));
		circ.setPosition(Vector2f(p.x, p.y));
		window.draw(circ);
	};
	auto drawRect=[&window](float2 p, float2 s, int w=1, Color col=Color::White) {
		RectangleShape rect(Vector2f(s.x, s.y));
		rect.setPosition(Vector2f(p.x, p.y));

		rect.setFillColor(Color::Transparent);
		rect.setOutlineColor(col);
		rect.setOutlineThickness(w);
		window.draw(rect);
	};
	auto fillRect=[&window](float2 p, float2 s, Color col=Color::White) {
		RectangleShape rect(Vector2f(s.x, s.y));
		rect.setPosition(Vector2f(p.x, p.y));

		rect.setFillColor(col);
		window.draw(rect);
	};
	while (window.isOpen()) {
		//polling
		for (Event event; window.pollEvent(event);) {
			if (event.type==Event::Closed) window.close();
		}

		//update
		float deltaTime=deltaClock.restart().asSeconds();

		//for each ball
		for (int i=0; i<16; i++) {
			auto& a=balls[i];
			//for each next ball
			for (int j=i+1; j<16; j++) {
				auto& b=balls[j];
				//broad phase
				if (a.getAABB().overlapAABB(b.getAABB())) {
					float2 pSub=a.pos-b.pos;
					float dist=length(pSub);
					//narrow phase
					float distDelta=BALL_RADIUS*2.f-dist;
					if (distDelta>0.f) {
						//collision resolve(push apart)
						float2 dir=pSub/dist;
						a.pos+=dir*distDelta*.5;
						b.pos-=dir*distDelta*.5;

						//collision response(elastic)
						float2 vSub=a.vel-b.vel;
						float left=dot(vSub, pSub)/(dist*dist);
						a.vel-=left*pSub;
						b.vel+=left*pSub;
					}
				}
			}
		}

		float kineticEnergy=0.f;
		for (int i=0; i<16; i++) {
			auto& b=balls[i];

			float2 dir=normalize(b.vel);
			b.applyForce(dir*-U_STATIC*GRAVITY);

			b.update(deltaTime);

			b.checkBounds(bounds);

			kineticEnergy+=.5f*dot(b.vel, b.vel);
		}

		std::string fpsStr=std::to_string((int)(1/deltaTime));
		//window.setTitle("Verlet Integration @ "+fpsStr+"fps");
		window.setTitle(std::to_string(kineticEnergy)+'J');

		//render
		window.clear();
		for (int i=0; i<16; i++) {
			const auto& b=balls[i];
			drawCircle(b.pos, BALL_RADIUS);
		}

		drawLine(float2(FIELD_WIDTH*.75f, 0), float2(FIELD_WIDTH*.75f, FIELD_HEIGHT));
		drawLine(float2(0, FIELD_HEIGHT*.5f), float2(FIELD_WIDTH, FIELD_HEIGHT*.5f));
		window.display();
	}

	return 0;
}