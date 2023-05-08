#include <list>

#include <SFML/Graphics.hpp>
using namespace sf;

#include "physics/constraint.h"
#include "physics/spring.h"

#define SUB_STEPS 4

#define COEFF_DRAG 15.43f

#define PARTICLE_RAD 9.5f
#define CONSTRAINT_RAD 6.2f
#define SPRING_STIFFNESS 24335.23f
#define SPRING_DAMPING 361.7f

#define RANDOM (rand()/32767.f)

float lerp(float a, float b, float t) {
	return a+(b-a)*t;
}

float clamp(float t, float a, float b) {
	if (t<a) return a;
	if (t>b) return b;
	return t;
}

int main() {
	srand(time(NULL));

	//sfml setup
	unsigned int width=720;
	unsigned int height=480;
	RenderWindow window(VideoMode(Vector2u(width, height)), "", Style::Titlebar|Style::Close);
	window.setFramerateLimit(165);
	Clock deltaClock;
	float totalDeltaTime=0;

	//basic prog setup
	float2 grav(0, 320);
	aabb bounds(0, 0, width, height);
	std::list<particle> particles;
	std::list<constraint> constraints;
	std::list<spring> springs;

	bool running=true;
	bool pausePlay=false;

	bool addParticle=false;
	bool addConstraint=false;
	particle* constraintStart=nullptr;
	bool addSpring=false;
	particle* springStart=nullptr;

	bool lockParticle=false;

	bool mouseDown=false;
	particle* mouseParticle=nullptr;

	RenderTexture renderTex;
	if (!renderTex.create(Vector2u(width, height))) {
		printf("Error: unable to create render texture.");
		return 1;
	}
	Shader crtShader;
	if (!crtShader.loadFromFile("shader/crt.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	crtShader.setUniform("Resolution", Glsl::Vec2(width, height));
	Shader blueprintShader;
	if (!blueprintShader.loadFromFile("shader/blueprint.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	blueprintShader.setUniform("Resolution", Glsl::Vec2(width, height));
	Texture shaderTex;
	if (!shaderTex.create(Vector2u(width, height))) {
		printf("Error: couldn't create texture");
		exit(1);
	}
	Sprite shaderSprite(shaderTex);
	shaderSprite.setOrigin(Vector2f(0, height/2));
	shaderSprite.setPosition(Vector2f(0, height/2));
	shaderSprite.setScale(Vector2f(1, -1));

	//loop
	auto drawLine=[&renderTex](float2 a, float2 b, Color col=Color::White) {
		float2 ba=b-a;
		RectangleShape line(Vector2f(length(ba), 2));
		line.setOrigin(Vector2f(0, 1));
		line.setRotation(radians(atan2(ba.y, ba.x)));
		line.setPosition(Vector2f(a.x, a.y));

		line.setFillColor(col);
		renderTex.draw(line);
	};
	auto drawThickLine=[&renderTex](float2 a, float2 b, float w, Color col=Color::White) {
		float2 ba=b-a;
		RectangleShape line(Vector2f(length(ba), 2*w));
		line.setOrigin(Vector2f(0, w));
		line.setRotation(radians(atan2(ba.y, ba.x)));
		line.setPosition(Vector2f(a.x, a.y));

		line.setFillColor(col);
		renderTex.draw(line);
	};
	auto drawCircle=[&renderTex](float2 p, float r, Color col=Color::White) {
		CircleShape circ(r);
		circ.setOutlineThickness(-2);
		circ.setOutlineColor(col);
		circ.setFillColor(Color::Transparent);

		circ.setOrigin(Vector2f(r, r));
		circ.setPosition(Vector2f(p.x, p.y));
		renderTex.draw(circ);
	};
	auto fillCircle=[&renderTex](float2 p, float r, Color col=Color::White) {
		CircleShape circ(r);
		circ.setFillColor(col);

		circ.setOrigin(Vector2f(r, r));
		circ.setPosition(Vector2f(p.x, p.y));
		renderTex.draw(circ);
	};
	auto drawArrow=[&drawLine](float2 a, float2 b, float t, Color col=Color::White) {
		float2 sub=b-a, sz=sub*t, tang{-sz.y, sz.x};
		float2 aSt=b-sz, lPt=aSt-tang*.5f, rPt=aSt+tang*.5f;
		drawLine(a, aSt, col), drawLine(lPt, rPt, col);
		drawLine(rPt, b, col), drawLine(lPt, b, col);
	};
	while (window.isOpen()) {
		//mouse position
		Vector2i mp=Mouse::getPosition(window);
		float2 mousePos(mp.x, mp.y);

		//polling
		for (Event event; window.pollEvent(event);) {
			if (event.type==Event::Closed) window.close();
		}

		//timing
		float actualDeltaTime=deltaClock.restart().asSeconds();
		float deltaTime=MIN(actualDeltaTime, 1./30.);
		totalDeltaTime+=deltaTime;
		blueprintShader.setUniform("Time", totalDeltaTime);

		//user input
		bool pausePlayTemp=Keyboard::isKeyPressed(Keyboard::Space);
		if (pausePlayTemp&&!pausePlay) {
			constraintStart=nullptr;
			springStart=nullptr;
			mouseParticle=nullptr;
			running^=true;
		} else {
			bool addParticleTemp=Keyboard::isKeyPressed(Keyboard::A);
			if (addParticleTemp&&!addParticle) {//on keydown
				particles.push_back(particle(mousePos, PARTICLE_RAD));
			}
			addParticle=addParticleTemp;

			bool addConstraintTemp=Keyboard::isKeyPressed(Keyboard::C);
			if (addConstraintTemp!=addConstraint) {
				if (addConstraintTemp) {//on keydown
					for (auto& p:particles) {
						if (length(p.pos-mousePos)<p.rad) {
							constraintStart=&p;
						}
					}
				} else {
					if (constraintStart!=nullptr) {
						particle* constraintEnd=nullptr;
						for (auto& p:particles) {
							if (length(p.pos-mousePos)<p.rad) constraintEnd=&p;
						}
						if (constraintEnd!=nullptr&&constraintEnd!=constraintStart) {
							constraints.push_back(constraint(*constraintStart, *constraintEnd, CONSTRAINT_RAD));
						}
					}
					//reset
					constraintStart=nullptr;
				}
			}
			addConstraint=addConstraintTemp;

			//add new springs
			bool addSpringTemp=Keyboard::isKeyPressed(Keyboard::S);
			if (addSpringTemp!=addSpring) {
				if (addSpringTemp) {//on keydown
					for (auto& p:particles) {
						if (length(p.pos-mousePos)<p.rad) {
							springStart=&p;
						}
					}
				} else {//on key up
					if (springStart!=nullptr) {
						particle* springEnd=nullptr;
						for (auto& p:particles) {
							if (length(p.pos-mousePos)<p.rad) springEnd=&p;
						}
						if (springEnd!=nullptr&&springEnd!=springStart) {
							springs.push_back(spring(*springStart, *springEnd, SPRING_STIFFNESS, SPRING_DAMPING));
						}
					}
					//reset
					springStart=nullptr;
				}
			}
			addSpring=addSpringTemp;
		}
		pausePlay=pausePlayTemp;

		//deletion!
		bool deleting=Keyboard::isKeyPressed(Keyboard::X);
		if (deleting) {
			//for every particle
			for (auto pit=particles.begin(); pit!=particles.end();) {
				auto& p=*pit;
				//if touching mouse
				if (length(mousePos-p.pos)<p.rad) {
					//remove all connecting constraints
					for (auto cit=constraints.begin(); cit!=constraints.end();) {
						auto& c=*cit;
						if (&p==c.a||&p==c.b) cit=constraints.erase(cit);
						else cit++;
					}
					//and springs
					for (auto sit=springs.begin(); sit!=springs.end();) {
						auto& s=*sit;
						if (&p==s.a||&p==s.b) sit=springs.erase(sit);
						else sit++;
					}
					//remove particle
					pit=particles.erase(pit);
				} else pit++;
			}
			//for every constraint
			for (auto cit=constraints.begin(); cit!=constraints.end();) {
				auto& c=*cit;
				float2 ba=c.b->pos-c.a->pos;
				float t=clamp(dot(mousePos-c.a->pos, ba)/dot(ba, ba), 0, 1);
				if (length(c.a->pos+ba*t-mousePos)<c.rad) {
					cit=constraints.erase(cit);
				} else cit++;
			}
			//for every spring
			for (auto sit=springs.begin(); sit!=springs.end();) {
				auto& s=*sit;
				float2 ba=s.b->pos-s.a->pos;
				float t=clamp(dot(mousePos-s.a->pos, ba)/dot(ba, ba), 0, 1);
				float rad=(s.a->rad+s.b->rad)/2;
				if (length(s.a->pos+ba*t-mousePos)<rad) {
					sit=springs.erase(sit);
				} else sit++;
			}
		}

		//set mouse particle
		bool mouseDownTemp=Mouse::isButtonPressed(Mouse::Left);
		if (mouseDownTemp!=mouseDown) {
			if (mouseDownTemp) {//on mouse down
				for (auto& p:particles) {
					if (length(p.pos-mousePos)<p.rad) {
						mouseParticle=&p;
					}
				}
			} else mouseParticle=nullptr;
		}
		mouseDown=mouseDownTemp;
		if (mouseParticle!=nullptr) {
			float2 sub=mousePos-mouseParticle->pos;
			if (running) {
				//more of a dragging motion.
				if (!mouseParticle->locked) mouseParticle->pos+=sub*deltaTime;
			} else {
				//set mouse particle to mouse pos.
				mouseParticle->pos=mouseParticle->oldpos=mousePos;
				for (auto& c:constraints) {
					if (mouseParticle==c.a||mouseParticle==c.b) {
						c.restLen=length(c.a->pos-c.b->pos);
					}
				}
				for (auto& s:springs) {
					if (mouseParticle==s.a||mouseParticle==s.b) {
						s.restLen=length(s.a->pos-s.b->pos);
					}
				}
			}
		}

		//lock and unlock particles.
		bool lockParticleTemp=Mouse::isButtonPressed(Mouse::Right);
		if (lockParticleTemp&&!lockParticle) {
			if (mouseParticle!=nullptr) {
				mouseParticle->locked^=true;
			} else {
				//just dont lock more than one at a time?
				particle* toLock=nullptr;
				for (auto& p:particles) {
					if (length(mousePos-p.pos)<p.rad) toLock=&p;
				}
				if (toLock!=nullptr) toLock->locked^=true;
			}
		}
		lockParticle=lockParticleTemp;

		//update
		if (running) {
			float subDeltaTime=deltaTime/SUB_STEPS;
			for (int k=0; k<SUB_STEPS; k++) {
				for (auto& c:constraints) c.update();

				for (auto& s:springs) s.update(subDeltaTime);

				//grav and constraint
				for (auto& p:particles) {
					//gravity: f=mg
					p.applyForce(grav*p.mass);

					//drag: f=-cv
					float2 vel=(p.oldpos-p.pos)/subDeltaTime;
					p.applyForce(COEFF_DRAG*vel);

					//bounds detection: "keep in window"
					p.checkAABB(bounds);
				}

				//particle-particle collisions
				for (auto ait=particles.begin(); ait!=particles.end(); ait++) {
					auto& a=*ait;
					for (auto bit=std::next(ait); bit!=particles.end(); bit++) {
						auto& b=*bit;
						//broad phase
						if (!a.getAABB().checkAABB(b.getAABB())) continue;

						float totalRad=a.rad+b.rad;
						constraint tempC(a, b, 0);
						//if intrinsic dist is overlapping (narrow phase)
						if (tempC.restLen>totalRad) continue;

						//resolve
						tempC.restLen=totalRad;
						tempC.update();
					}
				}

				//particle-constraint collisions
				for (auto& p:particles) {
					for (auto& c:constraints) {
						//dont check "self"
						if (&p==c.a||&p==c.b) continue;
						//broad phase
						if (!p.getAABB().checkAABB(c.getAABB())) continue;

						float2 pa=p.pos-c.a->pos, ba=c.b->pos-c.a->pos;
						float t=clamp(dot(pa, ba)/dot(ba, ba), 0, 1);
						particle tempP(c.a->pos+ba*t, 0);
						float totalRad=p.rad+c.rad;
						constraint tempC(p, tempP, 0);
						//if intrinsic dist is overlapping (narrow phase)
						if (tempC.restLen>totalRad) continue;

						//resolve particle
						tempC.restLen=totalRad;
						float2 tempForce=tempC.getForce();
						float totalMass=p.mass+c.a->mass+c.b->mass;
						if (!p.locked) p.pos+=(c.a->mass+c.b->mass)/totalMass*2*tempForce;

						//resolve constraint proportionally(torque-ish?)
						float2 abForce=p.mass/totalMass*2*tempForce;
						if (!c.a->locked) c.a->pos-=abForce*(1-t);
						if (!c.b->locked) c.b->pos-=abForce*t;
					}
				}

				//"integrate"
				for (auto& p:particles) p.update(subDeltaTime);
			}
		}

		//render
		std::string fpsStr=std::to_string((int)(1/deltaTime));
		std::string title="Verlet Integration 2 @ "+fpsStr+"[ ";
		renderTex.clear(running?Color(51, 51, 51):Color(0, 0, 0, 0));

		if (running&&mouseParticle!=nullptr) {
			title+="mov_ptc ";
			float factor=1.5f+(sinf(totalDeltaTime*3.)+1)/4;
			fillCircle(mouseParticle->pos, mouseParticle->rad*factor, Color(0x666666FF));
		}

		if (constraintStart!=nullptr) {
			title+="add_cst ";
			Color redYellow(240, 120+119*sinf(totalDeltaTime*2.3), 40);
			fillCircle(constraintStart->pos, constraintStart->rad, redYellow);
			drawLine(mousePos, constraintStart->pos, redYellow);
		}

		if (springStart!=nullptr) {
			title+="add_spr ";
			int mgVal=119*cosf(totalDeltaTime*2.3);
			Color magentaCyan(120+mgVal, 120-mgVal, 240);
			fillCircle(springStart->pos, springStart->rad, magentaCyan);
			drawLine(mousePos, springStart->pos, magentaCyan);
		}

		if (deleting) title+="delete ";
		title+="]";
		window.setTitle(title);

		//show particle velocities
		if (Keyboard::isKeyPressed(Keyboard::V)) {
			for (auto& p:particles) {
				float2 vel=(p.pos-p.oldpos)/deltaTime;
				drawArrow(p.pos, p.pos+vel, .2f, Color::Blue);
			}
		}

		//draw constraints
		if (running) for (const auto& c:constraints) {
			drawThickLine(c.a->pos, c.b->pos, c.rad, Color::Black);
			drawThickLine(c.a->pos, c.b->pos, c.rad-2);
		} else for (const auto& c:constraints) {//pipes
			float2 rAB=c.b->pos-c.a->pos;
			float tTheta=atan2f(rAB.y, rAB.x);
			float aTheta=asinf(c.rad/c.a->rad);
			float2 blPt=c.a->pos+float2(cosf(tTheta+aTheta), sinf(tTheta+aTheta))*c.a->rad;
			float2 brPt=c.a->pos+float2(cosf(tTheta-aTheta), sinf(tTheta-aTheta))*c.a->rad;

			float bTheta=asinf(c.rad/c.b->rad);
			float2 tlPt=c.b->pos+float2(cosf(tTheta-bTheta+PI), sinf(tTheta-bTheta+PI))*c.b->rad;
			float2 trPt=c.b->pos+float2(cosf(tTheta+bTheta+PI), sinf(tTheta+bTheta+PI))*c.b->rad;

			drawLine(blPt, tlPt);
			drawLine(brPt, trPt);
		}

		//draw springs as dotted lines
		for (const auto& s:springs) {
			float2 sub=s.b->pos-s.a->pos;
			float rad=.25f*(s.a->rad+s.b->rad);
			int num=int(s.restLen/(2*rad));
			for (int i=0; i<num; i++) {
				float2 a=s.a->pos+float(i)/num*sub;
				float2 b=s.a->pos+float(i+1)/num*sub;
				int d=i-num/2;
				if (d%2) {
					if (running) {
						drawThickLine(a, b, .5f*rad);
					}
					else drawLine(a, b);
				}
			}
		}

		//draw particles
		for (const auto& p:particles) {
			if (running) {
				fillCircle(p.pos, p.rad);
				drawCircle(p.pos, p.rad, Color::Black);
			} else drawCircle(p.pos, p.rad);
			//draw x if locked
			if (p.locked) {
				float v=p.rad/2.13;
				drawLine(p.pos-v, p.pos+v, Color::Red);
				drawLine(p.pos+float2(-v, v), p.pos+float2(+v, -v), Color::Red);
				if (running) drawCircle(p.pos, p.rad, Color::Red);
			}
		}
		Texture renderedTex=renderTex.getTexture();
		crtShader.setUniform("MainTex", renderedTex);
		blueprintShader.setUniform("MainTex", renderedTex);

		window.clear();
		if (running) window.draw(shaderSprite, &crtShader);
		else window.draw(shaderSprite, &blueprintShader);
		window.display();
	}

	return 0;
}