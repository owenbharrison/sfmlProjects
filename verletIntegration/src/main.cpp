#include <fstream>
#include <iostream>
#include <sstream>

#include <list>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
using namespace sf;

#include "io/stopwatch.h"
#include "physics/constraint.h"
#include "physics/fx.h"

#include "geom/quadtree.h"

#define PI 3.1415926f

#define RANDOM (rand() / 32767.f)

#define COEFF_DRAG .2f
#define SUB_STEPS 3

#define MS_STIFF 395.237f
#define MS_DAMP 6.3f

#define RAD 12.10f
#define DEL_PTC_RAD RAD * 2
#define DEL_CON_RAD DEL_PTC_RAD * .8f

float lerp(float a, float b, float t) {
	return a+(b-a)*t;
}

float clamp(float t, float a, float b) {
	if (t<a) return a;
	if (t>b) return b;
	return t;
}

float invVecLerp(float2 pt, float2 a, float2 b) {
	if (a==b) return 0;
	float2 pa=pt-a, ba=b-a;
	return clamp(dot(pa, ba)/dot(ba, ba), 0, 1);
}

//function to return closest point on segment
float2 getClosePt(float2 pt, float2 a, float2 b) {
	float2 ba=b-a;
	return a+ba*invVecLerp(pt, a, b);
}

//reciprocating color for [stress?]
Color colorWheel(float a01) {
	a01*=.833f;
	float angle=a01*2*PI;
	float r=(cos(angle)+1)*127.5f;
	float g=(cos(angle-PI*.667f)+1)*127.5f;
	float b=(cos(angle-PI*1.33f)+1)*127.5f;

	return Color(r, g, b);
}

//properly place min and max for aabb.
aabb boundsBetween(float2 a, float2 b) {
	float nx, ny, mx, my;
	if (a.x<b.x) {
		nx=a.x, mx=b.x;
	} else {
		nx=b.x, mx=a.x;
	}
	if (a.y<b.y) {
		ny=a.y, my=b.y;
	} else {
		ny=b.y, my=a.y;
	}
	return aabb(nx, ny, mx, my);
}

int main() {
	srand(time(NULL));
	//sfml setup
	unsigned int width=1000;
	unsigned int height=800;
	RenderWindow window(VideoMode(Vector2u(width, height)), "Particle Collisions", Style::Titlebar|Style::Close);
	//window.setFramerateLimit(165);
	Clock deltaClock;
	stopwatch updateWatch, renderWatch;
	float totalDeltaTime=0;

	//basic prog setup
	float2 grav(0, 400);
	aabb bounds(0, 0, width, height);
	quadTree* mainTree=nullptr;
	std::list<particle> particles;
	std::list<constraint> constraints;
	std::list<fx> effects;

	//add single particle
	bool addKey=false, addKeyWas=false;

	//setup and func for solid adding
	bool solidKey=false, solidKeyWas=false;
	bool addingSolid=false;
	float2 solidStart;
	auto addSolid=[&particles, &constraints](int w, int h, float2 center) {
		auto grid=new std::list<particle>::iterator[w*h];
		auto ix=[w](int i, int j) { return i+j*w; };
		for (int i=0; i<w; i++) {
			for (int j=0; j<h; j++) {
				float2 pos=center-float2(w-1, h-1)*RAD+float2(i, j)*RAD*2;
				particles.push_back(particle(pos, RAD));
				grid[ix(i, j)]=--particles.end();
			}
		}

		for (int i=0; i<w; i++) {
			for (int j=0; j<h; j++) {
				if (i<w-1) constraints.push_back(constraint(*grid[ix(i, j)], *grid[ix(i+1, j)]));
				if (j<h-1) constraints.push_back(constraint(*grid[ix(i, j)], *grid[ix(i, j+1)]));
			}
		}

		for (int i=0; i<w-1; i++) {
			for (int j=0; j<h-1; j++) {
				constraints.push_back(constraint(*grid[ix(i, j)], *grid[ix(i+1, j+1)]));
				constraints.push_back(constraint(*grid[ix(i+1, j)], *grid[ix(i, j+1)]));
			}
		}
		delete[] grid;
	};
	addSolid(9, 6, float2(width/2, height/2));

	//setup and func for rope adding
	bool ropeKey=false, ropeKeyWas=false;
	bool addingRope=false;
	float2 ropeStart;
	auto addRope=[&particles, &constraints](float2 a, float2 b) {
		float2 sub=b-a;
		float dist=length(sub);
		float2 n=sub/dist;
		int w=dist/(RAD*2)+1;
		auto arr=new std::list<particle>::iterator[w];
		for (int i=0; i<w; i++) {
			float2 pt=a+i*n*RAD*2;
			particles.push_back(particle(pt, RAD));
			arr[i]=--particles.end();
		}
		arr[0]->locked=true;
		arr[w-1]->locked=true;

		for (int i=0; i<w-1; i++) {
			constraints.push_back(constraint(*arr[i], *arr[i+1]));
		}
		delete[] arr;
	};
	addRope(float2(width*.25f, height*.8f), float2(width*.75f, height*.8f));

	//connection setup
	bool connectKey=false, connectKeyWas=false;
	particle* connectStart=nullptr;

	//mouse constraint setup
	bool held=false, heldWas=false;
	particle* heldParticle=nullptr;
	bool lock=false, lockWas=false;

	//file setup
	bool outputFile=false, outputFileWas=false;
	bool inputFile=false, inputFileWas=false;

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
	auto showAABB=[&window, &drawRect](aabb a) {
		drawRect(a.min, a.max-a.min, 1);
	};
	while (window.isOpen()) {
		//mouse position
		Vector2i mp=Mouse::getPosition(window);
		float2 mousePos(mp.x+RANDOM-.5, mp.y+RANDOM-.5);

		//polling
		for (Event event; window.pollEvent(event);) {
			if (event.type==Event::Closed) window.close();
		}

		//timing
		float actualDeltaTime=deltaClock.restart().asSeconds();
		float deltaTime=std::min(actualDeltaTime, 1/60.f);
		totalDeltaTime+=actualDeltaTime;
		std::string fpsStr=std::to_string((int)(1/actualDeltaTime));
		window.setTitle("Verlet Integration @ "+fpsStr+"fps");

		//update
		bool info=Keyboard::isKeyPressed(Keyboard::F12);
		if (info) updateWatch.start();

		//single adding
		addKey=Keyboard::isKeyPressed(Keyboard::A);
		if (addKey&&!addKeyWas) {
			bool toPut=true;
			for (const auto& p:particles) {
				if (length(mousePos-p.pos)<p.rad+RAD) {
					toPut=false;
					break;
				}
			}
			if (toPut) {
				particles.push_back(particle(mousePos, RAD));
			}
		}
		addKeyWas=addKey;

		//solid adding
		float2 solidEnd=mousePos;
		aabb solidBounds=boundsBetween(solidStart, mousePos);
		bool solidValid=false;
		{
			bool toPut=bounds.containsPt(solidStart)&&bounds.containsPt(solidEnd);
			for (const auto& p:particles) {
				if (p.getAABB().overlapAABB(solidBounds)) {
					toPut=false;
					break;
				}
			}
			if (toPut) solidValid=true;
		}
		solidKey=Keyboard::isKeyPressed(Keyboard::S);
		if (solidKey!=solidKeyWas) {
			if (solidKey) {
				addingSolid=true;

				solidStart=mousePos;
			} else {
				if (addingSolid) {
					if (solidValid) {
						float2 sz=(solidBounds.max-solidBounds.min)/(RAD*2);
						float2 ctr=(solidBounds.min+solidBounds.max)/2;
						int w=sz.x, h=sz.y;
						addSolid(w, h, ctr);
					}
				}

				addingSolid=false;
			}
		}
		solidKeyWas=solidKey;

		//rope adding
		bool ropeValid=false;
		float2 ropeEnd=mousePos;
		{
			bool toPut=true;
			for (const auto& p:particles) {
				float2 pt=getClosePt(p.pos, ropeStart, ropeEnd);
				if (length(p.pos-pt)<p.rad+RAD) {
					toPut=false;
					break;
				}
			}
			if (toPut) ropeValid=true;
		}
		ropeKey=Keyboard::isKeyPressed(Keyboard::R);
		if (ropeKey!=ropeKeyWas) {
			if (ropeKey) {
				addingRope=true;

				ropeStart=mousePos;
			} else {
				if (addingRope) {
					if (ropeValid) {
						addRope(ropeStart, ropeEnd);
					}
				}

				addingRope=false;
			}
		}
		ropeKeyWas=ropeKey;

		//connecting
		connectKey=Keyboard::isKeyPressed(Keyboard::C);
		if (connectKey!=connectKeyWas) {
			if (connectKey) {
				connectStart=nullptr;
				for (auto& p:particles) {
					if (length(mousePos-p.pos)<p.rad) connectStart=&p;
				}
			} else {
				if (connectStart!=nullptr) {
					particle* connectEnd=nullptr;
					for (auto& p:particles) {
						if (length(mousePos-p.pos)<p.rad) connectEnd=&p;
					}
					if (connectEnd!=nullptr&&connectEnd!=connectStart) {
						constraints.push_back(constraint(*connectStart, *connectEnd));
					}
					connectStart=nullptr;
				}
			}
		}
		connectKeyWas=connectKey;

		//holding
		held=Mouse::isButtonPressed(Mouse::Left);
		if (held!=heldWas) {
			heldParticle=nullptr;

			if (held) {
				for (auto& p:particles) {
					if (length(mousePos-p.pos)<p.rad) heldParticle=&p;
				}
			}
		}
		heldWas=held;

		//update held, locking
		lock=Keyboard::isKeyPressed(Keyboard::L);
		bool toLock=lock&&!lockWas;
		if (heldParticle!=nullptr) {
			float2 sub=mousePos-heldParticle->pos;
			heldParticle->accelerate(sub*MS_STIFF);
			if (toLock) heldParticle->locked=!heldParticle->locked;
		}
		lockWas=lock;

		//verlet integrate everything
		float subDeltaTime=deltaTime/SUB_STEPS;
		for (int k=0; k<SUB_STEPS; k++) {
			for (auto& c:constraints) c.update();

			//grav and constraint
			for (auto& p:particles) {
				p.accelerate(grav);

				//drag
				p.accelerate((p.oldpos-p.pos)/deltaTime*COEFF_DRAG);

				p.checkAABB(bounds);
			}

			//particle-particle collisions
			//for every particle
			for (auto ait=particles.begin(); ait!=particles.end(); ait++) {
				auto& a=*ait;
				//check all particles after it
				for (auto bit=std::next(ait); bit!=particles.end(); bit++) {
					auto& b=*bit;
					//broad phase
					if (a.getAABB().overlapAABB(b.getAABB())) {
						float totalRad=a.rad+b.rad;
						constraint tempC(a, b);
						//if intrinsic dist is overlapping (narrow phase)
						if (tempC.restLen<totalRad) {
							//resolve
							tempC.restLen=totalRad;
							tempC.update();
						}
					}
				}
			}

			//integrate
			for (auto& p:particles) p.update(subDeltaTime);
		}

		//update fx
		for (auto it=effects.begin(); it!=effects.end();) {
			auto& f=*it;
			f.accelerate(grav);

			f.update(deltaTime);

			if (f.isDead()||!bounds.containsPt(f.pos)) {
				it=effects.erase(it);
			} else it++;
		}

		//remove particles near mouse
		bool deleteParticles=Mouse::isButtonPressed(Mouse::Right);
		if (deleteParticles) {
			heldParticle=nullptr;
			connectStart=nullptr;
			for (auto it=particles.begin(); it!=particles.end();) {
				auto& p=*it;
				if (length(mousePos-p.pos)<DEL_PTC_RAD) {
					//add fx
					int num=8+RANDOM*8;
					float fxRad=(p.rad/num)*3;
					for (int i=0; i<num; i++) {
						float angle=RANDOM*2*PI;
						float rad=RANDOM*p.rad;
						float2 dir(cosf(angle), sinf(angle));
						float2 pos=p.pos+dir*rad;
						float speed=30+RANDOM*20;
						float lifeSpan=.5f+RANDOM;
						effects.push_back(fx(pos, dir*speed, fxRad, lifeSpan));
					}

					//remove constraints
					constraints.remove_if([&p](const constraint& c) { return c.a==&p||c.b==&p; });

					//remove particle
					it=particles.erase(it);
				} else it++;
			}
		}

		//remove constraints near mouse
		bool deleteConstraints=Mouse::isButtonPressed(Mouse::Middle);
		if (deleteConstraints) {
			constraints.remove_if([&mousePos](const constraint& c) {
				return length(mousePos-getClosePt(mousePos, c.a->pos, c.b->pos))<DEL_CON_RAD;
			});
		}

		//clearing of constraints and particles
		bool clearAll=Keyboard::isKeyPressed(Keyboard::Delete);
		if (Keyboard::isKeyPressed(Keyboard::End)||clearAll) constraints.clear();
		if (clearAll) {
			heldParticle=nullptr;
			connectStart=nullptr;
			particles.clear();
		}
		if (info) updateWatch.stop();

		//file input/output
		//save
		outputFile=Keyboard::isKeyPressed(Keyboard::O);
		if (outputFile&&!outputFileWas) {
			//prompt for filename
			std::cout<<"input filename to save\n";
			std::string filename;
			std::cin>>filename;

			//make new file
			std::ofstream file;
			file.open(filename);
			int id=0;
			for (auto& p:particles) {
				p.id=id++;
				file<<"p "<<p.id<<' '<<p.pos.x<<' '<<p.pos.y<<' '<<p.rad<<' '<<p.locked<<'\n';
			}

			for (auto& c:constraints) {
				file<<"c "<<c.a->id<<' '<<c.b->id<<' '<<c.restLen<<'\n';
			}
			file.close();
			if (file.good()) std::cout<<"physics configuration saved to "<<filename<<'\n';
			else std::cout<<"\""<<filename<<"\" is an invalid filename"<<'\n';
		}
		outputFileWas=outputFile;
		//load
		inputFile=Keyboard::isKeyPressed(Keyboard::I);
		if (inputFile) {
			//prompt for filename
			std::cout<<"input filename to load\n";
			std::string filename;
			std::cin>>filename;

			//does file exist?
			std::ifstream file(filename);
			if (file.good()) {
				constraints.clear();
				particles.clear();
				for (std::string line; getline(file, line);) {
					std::stringstream lineStream;
					lineStream<<line;
					char junk;
					if (line[0]=='p') {
						particle p;
						lineStream>>junk>>p.id>>p.pos.x>>p.pos.y>>p.rad>>p.locked;
						p.oldpos=p.pos;
						particles.push_back(p);
					}
					if (line[0]=='c') {
						//read connection
						int aId, bId;
						float restLen;
						lineStream>>junk>>aId>>bId>>restLen;
						//find connect pts
						auto aIt=std::find_if(particles.begin(), particles.end(), [&aId](const particle& p) { return p.id==aId; });
						auto bIt=std::find_if(particles.begin(), particles.end(), [&bId](const particle& p) { return p.id==bId; });
						//do they exist?
						if (aIt!=particles.end()&&bIt!=particles.end()) {
							//connect them!
							constraint c(*aIt, *bIt);
							c.restLen=restLen;
							constraints.push_back(c);
						}
					}
				}
				std::cout<<"loaded physics configuration file\n";
			} else std::cout<<"couldn't find "<<filename<<"\n";
			file.close();
		}
		inputFileWas=inputFile;

		//render
		if (info) renderWatch.start();
		window.clear();
		fillRect(float2(0), float2(width, height), Color(173, 173, 173));

		//show quadTree
		if (Keyboard::isKeyPressed(Keyboard::Q)) {
			mainTree=new quadTree(bounds);
			for (const auto& p:particles) {
				mainTree->insert(p.pos);
			}
			std::list<quadTree*> toShow{mainTree};
			while (toShow.size()>0) {
				for (auto it=toShow.begin(); it!=toShow.end();) {
					quadTree* q=*it;
					if (q!=nullptr) {
						showAABB(q->bounds);

						if (q->northWest!=nullptr) {
							toShow.push_front(q->northWest);
							toShow.push_front(q->northEast);
							toShow.push_front(q->southWest);
							toShow.push_front(q->southEast);
						}
						it=toShow.erase(it);
					} else it++;
				}
			}
			delete mainTree;
		}

		//show constraints
		for (const auto& c:constraints) {
			drawLine(c.a->pos, c.b->pos, Color::Black);
		}

		//show particles
		bool showVel=Keyboard::isKeyPressed(Keyboard::V);
		for (const auto& p:particles) {
			float2 vel=(p.pos-p.oldpos)/deltaTime;
			float pct=clamp(length(vel)*0.01f, 0, 1);
			Color col=colorWheel(pct);

			//show where they are going
			if (showVel) {
				float2 nextPos=p.pos+vel/deltaTime;
				drawCircle(nextPos, p.rad, col);
				drawLine(p.pos, nextPos, col);
			}
			fillCircle(p.pos, p.rad, col);
			if (p.locked) drawCircle(p.pos, p.rad, Color::Black);
		}

		//show effects
		for (const auto& f:effects) {
			float pct=f.age/f.lifeSpan;
			int shade=(1-pct)*127;
			fillCircle(f.pos, f.rad, Color(shade, shade, shade));
		}

		//show solid addition
		if (addingSolid) {
			Color col=solidValid?Color::Green:Color::Red;
			drawRect(solidBounds.min, solidBounds.max-solidBounds.min, 4, col);
		}

		//show rope addition
		if (addingRope) {
			Color col=ropeValid?Color::Green:Color::Red;
			drawLine(ropeStart, ropeEnd, col);
		}

		//show current connection
		if (connectStart!=nullptr) {
			float fract=totalDeltaTime-(int)(totalDeltaTime);
			Color col=fract>.5f?Color::Yellow:Color::Cyan;
			drawLine(connectStart->pos, mousePos, col);
		}

		//show mouse constraint
		if (heldParticle!=nullptr) {
			drawLine(heldParticle->pos, mousePos, Color::Black);
		}

		//show particle deletion
		if (deleteParticles) {
			drawCircle(mousePos, DEL_PTC_RAD, Color::Blue);
		}

		//show constraint deletion
		if (deleteConstraints) {
			drawCircle(mousePos, DEL_CON_RAD, Color::Magenta);
		}

		if (Keyboard::isKeyPressed(Keyboard::LShift)) {
			//select continuously connected objects
			if (particles.size()>0&&constraints.size()>0) {
				//get first particle
				particle* closeParticle=nullptr;
				for (auto& p:particles) {
					if (length(mousePos-p.pos)<p.rad) closeParticle=&p;
				}
				if (closeParticle!=nullptr) {
					std::list<particle*> pStack{closeParticle};
					std::list<constraint*> cStack;
					for (auto& c:constraints) cStack.push_back(&c);

					//floodfill
					for (const auto& p:pStack) {
						for (auto cit=cStack.begin(); cit!=cStack.end();) {
							const auto& c=*cit;
							bool fromA=c->a==p;
							bool fromB=c->b==p;
							//xor, both shouldnt happen.
							if (fromA!=fromB) {
								particle* toAdd=nullptr;
								if (fromA) toAdd=c->b;
								if (fromB) toAdd=c->a;
								if (toAdd!=nullptr) {
									bool canAdd=true;
									for (const auto& o:pStack) {
										if (o==toAdd) {
											canAdd=false;
											break;
										}
									}
									if (canAdd) {
										pStack.push_back(toAdd);
									}
								}

								cit=cStack.erase(cit);
							} else cit++;
						}
					}

					//find & draw bounding box
					aabb ccoBnd(float2(INFINITY), float2(-INFINITY));
					for (const auto& p:pStack) {
						float2 pos=p->pos;
						float rad=p->rad;
						ccoBnd.min.x=std::min(ccoBnd.min.x, pos.x-rad), ccoBnd.min.y=std::min(ccoBnd.min.y, pos.y-rad);
						ccoBnd.max.x=std::max(ccoBnd.max.x, pos.x+rad), ccoBnd.max.y=std::max(ccoBnd.max.y, pos.y+rad);
					}
					drawRect(ccoBnd.min, ccoBnd.max-ccoBnd.min, 4, Color::Green);

					//remove if prompted
					if (Keyboard::isKeyPressed(Keyboard::X)) {
						for (const auto& p:pStack) {
							particles.remove_if([&p](const particle& o) { return &o==p; });
							constraints.remove_if([&p](const constraint& c) { return c.a==p||c.b==p; });
						}
					}
				}
			}
		}

		window.display();
		if (info) renderWatch.stop();

		if (info) printf("up: %ius re: %ius\n", updateWatch.getMicroseconds(), renderWatch.getMicroseconds());
	}

	return 0;
}