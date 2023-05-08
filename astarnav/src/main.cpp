#include <fstream>
#include <sstream>
#include <iostream>

#include <list>
#include <algorithm>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Mouse.hpp>
using namespace sf;

#include "vector/float2.h"

#define NODE_SIZE 6
#define ROAD_SIZE 4

#define PI 3.1415926f
#define RADTODEG (180.f/PI)

struct node {
	int id = -1;
	float2 pos;
	float gCost = 0, hCost = 0, fCost = 0;
	node* parent = nullptr;
	std::list<node*> neighbors;

	node() {}

	node(float2 pos_) : pos(pos_) {}
};

int main() {
	//setup
	unsigned int width = 1000;
	unsigned int height = 750;
	RenderWindow window(VideoMode(Vector2u(width, height)), "A* Navigation");
	Font sfFont;
	if (!sfFont.loadFromFile("C:\\Windows\\Fonts\\arial.ttf")) {
		printf("Arial font not found.");
		exit(0);
	}
	Clock deltaClock;
	float totalDeltaTime = 0;

	std::list<node> nodes;
	node* startNode = nullptr, * endNode = nullptr;

	bool running = false;
	bool pausePlay = false, wasPausePlay = false;
	bool addNode = false, wasAddNode = false;
	bool connect = false, wasConnect = false;
	node* connectStart = nullptr;
	bool hold = false, wasHold = false;
	node* heldNode = nullptr;

	bool outputFile = false, wasOutputFile = false;
	bool inputFile = false, wasInputFile = false;

	bool changeBackground = false, wasChangeBackground = false;

	Texture backgroundImg;
	if (!backgroundImg.loadFromFile("asphalt.png")) {
		printf("unable to load image");
		exit(0);
	}
	Sprite background(backgroundImg);
	IntRect size = background.getTextureRect();
	background.setScale(Vector2f(width / (float)size.width, height / (float)size.height));

	//loop
	auto drawLine = [&window](float2 a, float2 b, Color col = Color::White) {
		float2 ba = b - a;
		RectangleShape line(Vector2f(length(ba), 3));
		line.setOrigin(Vector2f(0, 1.5));
		line.setRotation(radians(atan2(ba.y, ba.x)));
		line.setPosition(Vector2f(a.x, a.y));

		line.setFillColor(col);
		window.draw(line);
	};
	auto drawArrowAlongLine = [&window, drawLine](float2 a, float2 b, float t, Color col = Color::White) {
		drawLine(a, b, col);

		float2 ba = b - a;
		float2 tang = normalize(ba) * NODE_SIZE;
		float2 norm(-tang.y, tang.x);
		float2 pt = a + ba * t;

		drawLine(pt, pt - tang + norm, col);
		drawLine(pt, pt - tang - norm, col);
	};
	auto drawCircle = [&window](float2 p, float r, Color col = Color::White) {
		CircleShape circ(r);
		circ.setOutlineThickness(-2);
		circ.setOutlineColor(col);
		circ.setFillColor(Color::Transparent);

		circ.setOrigin(Vector2f(r, r));
		circ.setPosition(Vector2f(p.x, p.y));
		window.draw(circ);
	};
	auto fillCircle = [&window](float2 p, float r, Color col = Color::White) {
		CircleShape circ(r);
		circ.setFillColor(col);

		circ.setOrigin(Vector2f(r, r));
		circ.setPosition(Vector2f(p.x, p.y));
		window.draw(circ);
	};
	auto drawText = [&sfFont, &window](float2 p, std::string str, Color col = Color::White) {
		Text text;
		text.setFont(sfFont);
		text.setStyle(Text::Bold);
		text.setString(str);
		text.setCharacterSize(18);
		text.setFillColor(col);
		text.setOrigin(text.findCharacterPos(str.length() / 2));

		text.setPosition(Vector2f(p.x, p.y - 6));
		window.draw(text);
	};
	while (window.isOpen()) {
		//polling
		Event event;
		while (window.pollEvent(event)) {
			if (event.type == Event::Closed) window.close();
		}

		//timing
		float deltaTime = deltaClock.restart().asSeconds();
		totalDeltaTime += deltaTime;
		std::string fpsStr = std::to_string((int)(1 / deltaTime));
		std::string runStr = running ? "running" : "not running";
		window.setTitle("A* Navigation [" + runStr + "] @ " + fpsStr + "fps");

		//update
		Vector2i mp = Mouse::getPosition(window);
		float2 mousePos(mp.x, mp.y);

		//for modifying
		connect = Keyboard::isKeyPressed(Keyboard::C);
		bool connectChanged = connect != wasConnect;
		if (connectChanged) {
			if (connect) {//on key down
				//clear connection.
				connectStart = nullptr;
				for (node& n : nodes) {
					float dist = length(mousePos - n.pos);
					if (dist < NODE_SIZE) {
						connectStart = &n;
					}
				}
			}
			else {//on key up
				node* connectEnd = nullptr;
				for (auto& n : nodes) {
					float dist = length(mousePos - n.pos);
					if (dist < NODE_SIZE) {
						connectEnd = &n;
					}
				}
				//dont connect to nothing from nothing or self.
				if (connectStart != nullptr && connectEnd != nullptr && connectEnd != connectStart) {
					connectStart->neighbors.push_back(connectEnd);
				}
				//clear connection.
				connectStart = nullptr;
			}
		}
		wasConnect = connect;

		addNode = Keyboard::isKeyPressed(Keyboard::A);
		bool toAdd = addNode && !wasAddNode;//on key down
		wasAddNode = addNode;

		bool toDelete = Keyboard::isKeyPressed(Keyboard::D);
		bool toSetStart = Keyboard::isKeyPressed(Keyboard::S);
		bool toSetEnd = Keyboard::isKeyPressed(Keyboard::E);

		outputFile = Keyboard::isKeyPressed(Keyboard::O);
		bool toOutputFile = outputFile && !wasOutputFile;
		wasOutputFile = outputFile;
		inputFile = Keyboard::isKeyPressed(Keyboard::I);
		bool toInputFile = inputFile && !wasInputFile;
		wasInputFile = inputFile;

		changeBackground = Keyboard::isKeyPressed(Keyboard::B);
		bool toChangeBackground = changeBackground && !wasChangeBackground;
		wasChangeBackground = changeBackground;

		if (toAdd || toDelete || toSetStart || toSetEnd || connectChanged || toInputFile || toOutputFile || toChangeBackground) {
			//stop if modify
			running = false;

			//add at mouse if far from others
			if (toAdd) {
				startNode = nullptr;
				endNode = nullptr;

				bool farAway = true;
				for (const auto& n : nodes) {
					float dist = length(mousePos - n.pos);
					if (dist < NODE_SIZE) {
						farAway = false;
						break;
					}
				}
				if (farAway) {
					nodes.push_back(node(mousePos));
				}
			}
			//removes at mouse
			if (toDelete) {
				auto it = nodes.begin();
				while (it != nodes.end()) {
					auto& n = *it;
					float dist = length(mousePos - n.pos);
					if (dist < NODE_SIZE) {
						for (auto oit = nodes.begin(); oit != nodes.end(); oit++) {
							auto& o = *oit;
							o.neighbors.remove(&n);
						}
						//remove node
						it = nodes.erase(it);
					}
					else it++;
				}
			}
			//sets random in range.
			if (toSetStart) {
				for (auto& n : nodes) {
					float dist = length(mousePos - n.pos);
					if (dist < NODE_SIZE) {
						startNode = &n;
					}
				}
			}
			//sets random in range.
			if (toSetEnd) {
				for (auto& n : nodes) {
					float dist = length(mousePos - n.pos);
					if (dist < NODE_SIZE) {
						endNode = &n;
					}
				}
			}
		}

		//save
		if (toOutputFile) {
			//prompt for filename
			std::cout << "input nav filename to save\n";
			std::string filename;
			std::cin >> filename;

			//make new file
			std::ofstream file;
			file.open(filename);
			int id = 0;
			for (auto& n : nodes) {
				n.id = id++;
				file << "n " << n.id << " " << n.pos.x << " " << n.pos.y << "\n";
			}

			for (const auto& n : nodes) {
				for (const auto o : n.neighbors) {
					file << "c " << n.id << " " << o->id << "\n";
				}
			}
			file.close();
			std::cout << "nav mesh saved to " << filename << "\n";
		}
		//load
		if (toInputFile) {
			//prompt for filename
			std::cout << "input nav filename to load\n";
			std::string filename;
			std::cin >> filename;

			//does file exist?
			std::ifstream file(filename);
			if (file.good()) {
				nodes.clear();
				for (std::string line; getline(file, line);) {
					std::stringstream lineStream;
					lineStream << line;
					char junk;
					if (line[0] == 'n') {
						node n;
						lineStream >> junk >> n.id >> n.pos.x >> n.pos.y;
						nodes.push_back(n);
					}
					if (line[0] == 'c') {
						//read connection
						int fromId, toId;
						lineStream >> junk >> fromId >> toId;
						//find connect pts
						auto fromIt = find_if(nodes.begin(), nodes.end(), [fromId](const node& n) {return n.id == fromId; });
						auto toIt = find_if(nodes.begin(), nodes.end(), [toId](const node& n) {return n.id == toId; });
						if (fromIt != nodes.end() && toIt != nodes.end()) {
							//do they exist?
							auto& from = *fromIt;
							auto& to = *toIt;
							//connect them!
							from.neighbors.push_back(&to);
						}
					}
				}
				std::cout << "successfully nav mesh loaded file\n";
			}
			else std::cout << "couldn't find " << filename << "\n";
			file.close();
		}
		//change background
		if (toChangeBackground) {
			//prompt for filename
			std::cout << "input background filename\n";
			std::string filename;
			std::cin >> filename;

			//does file exist?
			if (backgroundImg.loadFromFile(filename)) {
				background=Sprite(backgroundImg);
				IntRect newSize = background.getTextureRect();
				background.setScale(Vector2f(width / (float)newSize.width, height / (float)newSize.height));

				std::cout << "loaded background image\n";
			}
			else std::cout << "couldn't find " << filename << "\n";
		}

		//for pause/play
		pausePlay = Keyboard::isKeyPressed(Keyboard::Space);
		if (pausePlay && !wasPausePlay) {
			connectStart = nullptr;
			heldNode = nullptr;
			if (running) running = false;
			else {
				bool startValid = startNode != nullptr;
				bool endValid = endNode != nullptr;
				if (!startValid) std::cerr << "couldn't complete navigation. choose start\n";
				if (!endValid) std::cerr << "couldn't complete navigation. choose start\n";
				if (startValid && endValid) running = true;
			}
		}
		wasPausePlay = pausePlay;

		//mouse interact
		hold = Mouse::isButtonPressed(Mouse::Left);
		if (hold != wasHold) {//on key change
			//reset
			heldNode = nullptr;
			if (hold) {//on key down
				//find point to "hold"
				for (node& p : nodes) {
					float dist = length(mousePos - p.pos);
					if (dist < NODE_SIZE) {
						heldNode = &p;
					}
				}
			}
		}
		wasHold = hold;
		if (heldNode != nullptr) {
			heldNode->pos = mousePos;
		}

		//render
		window.clear();
		window.draw(background);

		//show connections...
		for (node& n : nodes) {
			for (node* o : n.neighbors) {
				float2 tang = normalize(o->pos - n.pos);
				float2 norm(-tang.y, tang.x);
				//...on one side?
				drawLine(n.pos + norm * ROAD_SIZE, o->pos + norm * ROAD_SIZE, Color(255, 255, 255));
			}
		}

		//show nodes
		float maxLen = length(float2(width, height));
		for (node& n : nodes) {
			float dist = length(mousePos - n.pos);
			float pct = pow(1 - dist / maxLen, 5);
			fillCircle(n.pos, NODE_SIZE, Color(150, 150, 150, pct * 127 + 127));
		}

		//show start, end
		if (startNode != nullptr) {
			drawText(startNode->pos, "Start Node", Color::Magenta);
		}
		if (endNode != nullptr) {
			drawText(endNode->pos, "End Node", Color::Magenta);
		}

		//show hover point
		node* possible = nullptr;
		for (node& p : nodes) {
			float dist = length(mousePos - p.pos);
			if (dist < NODE_SIZE) {
				possible = &p;
			}
		}
		if (possible != nullptr) {
			drawCircle(possible->pos, NODE_SIZE, Color::Green);
		}

		//show held point
		if (heldNode != nullptr) {
			drawCircle(heldNode->pos, NODE_SIZE, Color::Cyan);
		}

		//calculate and show path
		if (running) {
			std::list<node*> openSet{ startNode }, closedSet;
			while (openSet.size() > 0) {
				//curr = node in OPEN with lowest f
				auto it = openSet.begin();
				auto currNode = *it;
				for (it++; it != openSet.end(); it++) {
					auto check = *it;
					bool fLess = check->fCost < currNode->fCost;
					bool fEqual = check->fCost == currNode->fCost;
					bool hLess = check->hCost < currNode->hCost;
					if (fLess || fEqual && hLess) {
						currNode = check;
					}
				}

				//remove curr from OPEN
				openSet.remove(currNode);

				//add curr to CLOSED
				closedSet.push_back(currNode);

				if (currNode == endNode) {
					//running=false;

					//path found
					openSet.clear();
					closedSet.clear();

					//trace back using parents
					node* c = endNode;
					while (c != startNode) {
						float2 tang = normalize(c->parent->pos - c->pos);
						float2 norm(-tang.y, tang.x);
						float t = totalDeltaTime - (int)totalDeltaTime;
						drawArrowAlongLine(c->parent->pos - norm * ROAD_SIZE, c->pos - norm * ROAD_SIZE, t, Color::Blue);

						c = c->parent;
					}
					break;
				}

				//foreach neighbor of the curr node
				for (node* n : currNode->neighbors) {
					//if neighbor is in CLOSED
					auto nInClosed = find(closedSet.begin(), closedSet.end(), n);
					if (nInClosed != closedSet.end()) {
						//skip to next neighbor
						continue;
					}

					//if new path to neighbor is shorter OR neighbor is NOT in OPEN
					float currNDist = length(currNode->pos - n->pos);
					float newGCost = currNode->gCost + currNDist;
					auto nInOpen = find(openSet.begin(), openSet.end(), n);
					if (newGCost < n->gCost || nInOpen == openSet.end()) {
						//set f of neighbor
						n->gCost = newGCost;
						n->hCost = length(endNode->pos - n->pos);
						n->fCost = n->gCost + n->hCost;

						//set parent of neighbor to curr
						n->parent = currNode;

						//if neighbor is NOT in OPEN...
						if (nInOpen == openSet.end()) {
							//...add neighbor to OPEN
							openSet.push_back(n);
						}
					}
				}
			}
		}

		window.display();
	}

	return 0;
}