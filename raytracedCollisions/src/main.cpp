#include <list>

#include "physics/constraint.h"

#include <SFML/Graphics.hpp>

#define PI 3.1415926
#define FOV PI*.5
#define LOOK_SPEED PI*.5
#define MOVE_SPEED 12.

#define RAD .09
#define COEFF_DRAG .2f
#define SUB_STEPS 3

int main() {
	srand(time(NULL));
	//sfml setup
	unsigned int width = 1000;
	unsigned int height = 800;
	sf::RenderWindow window(sf::VideoMode(sf::Vector2u(width, height)), "Particle Collisions", sf::Style::Titlebar | sf::Style::Close);
	window.setFramerateLimit(165);
	sf::Clock deltaClock;

	//prog setup
	float camPitch = PI * .5, camYaw = PI * .5;
	float3 sunPos(5, 5, -5), camPos(0, 0, -5), camDir(0, 0, 1);

	float3 grav(0, -1.3, 0);
	aabb bounds(-1.5, -2, -1.5, 1.5, 2, 1.5);

	std::list<particle> particles;
	sf::Glsl::Vec4* particleInfo= new sf::Glsl::Vec4[512];
	std::list<constraint> constraints;

	auto addSolid = [&particles, &constraints](int w, int h, int d, float3 center) {
		auto grid = new std::list<particle>::iterator[w * h];
		auto ix = [w, d](int i, int j, int k) { return i + k * w + j * w * d; };
		for (int i = 0; i < w; i++) {
			for (int j = 0; j < h; j++) {
				for (int k = 0; k < d; k++) {
					float3 pos = center - float3(w - 1, h - 1, d - 1) * RAD + float3(i, j, k) * RAD * 2;
					particles.push_back(particle(pos, RAD));
					grid[ix(i, j, k)] = --particles.end();
				}
			}
		}

		for (int x = 0; x < w; x++) {
			for (int y = 0; y < h; y++) {
				for (int z = 0; z < d; z++) {
					if (x < w - 1) constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x + 1, y, z)]));
					if (y < h - 1) constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x, y + 1, z)]));
					if (z < d - 1) constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x, y, z + 1)]));
				}
			}
		}
		//for x, set yz diags
		for (int x = 0; x < w; x++) {
			for (int y = 0; y < h - 1; y++) {
				for (int z = 0; z < d - 1; z++) {
					constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x, y + 1, z + 1)]));
					constraints.push_back(constraint(*grid[ix(x, y, z + 1)], *grid[ix(x, y + 1, z)]));
				}
			}
		}
		//for y, set zx diags
		for (int y = 0; y < h; y++) {
			for (int z = 0; z < d - 1; z++) {
				for (int x = 0; x < w - 1; x++) {
					constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x + 1, y, z + 1)]));
					constraints.push_back(constraint(*grid[ix(x + 1, y, z)], *grid[ix(x, y, z + 1)]));
				}
			}
		}
		//for z, set xy diags
		for (int z = 0; z < d; z++) {
			for (int x = 0; x < w - 1; x++) {
				for (int y = 0; y < h - 1; y++) {
					constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x + 1, y + 1, z)]));
					constraints.push_back(constraint(*grid[ix(x + 1, y, z)], *grid[ix(x, y + 1, z)]));
				}
			}
		}
		//total diags
		for (int x = 0; x < w - 1; x++) {
			for (int y = 0; y < h - 1; y++) {
				for (int z = 0; z < d - 1; z++) {
					constraints.push_back(constraint(*grid[ix(x, y, z)], *grid[ix(x + 1, y + 1, z + 1)]));
					constraints.push_back(constraint(*grid[ix(x, y, z + 1)], *grid[ix(x + 1, y + 1, z)]));
					constraints.push_back(constraint(*grid[ix(x, y + 1, z)], *grid[ix(x + 1, y, z + 1)]));
					constraints.push_back(constraint(*grid[ix(x, y + 1, z + 1)], *grid[ix(x + 1, y, z)]));
				}
			}
		}
		delete[] grid;
	};
	addSolid(4, 4, 4, float3(0, -.5, 0));

	auto addRope = [&particles, &constraints](float3 a, float3 b) {
		float3 sub = b - a;
		float dist = length(sub);
		float3 n = sub / dist;
		int w = dist / (RAD * 2) + 1;
		auto arr = new std::list<particle>::iterator[w];
		for (int i = 0; i < w; i++) {
			float3 pt = a + i * n * RAD * 2;
			particles.push_back(particle(pt, RAD));
			arr[i] = --particles.end();
		}
		arr[0]->locked = true;
		arr[w - 1]->locked = true;

		for (int i = 0; i < w - 1; i++) {
			constraints.push_back(constraint(*arr[i], *arr[i + 1]));
		}
		delete[] arr;
	};
	addRope(float3(-1.3, -1.6, 0), float3(1.3, -1.6, 0));

	//shader setup
	sf::Texture skyTexture;
	if (!skyTexture.loadFromFile("skybox.png")) {
		printf("failed to create texture.");
		exit(1);
	}
	sf::Shader shader;
	if (!shader.loadFromFile("raytracing.glsl", sf::Shader::Fragment)) {
		printf("failed to load shader.");
		exit(1);
	}
	shader.setUniform("skybox", skyTexture);
	sf::Texture shaderTexture;
	if (!shaderTexture.create(sf::Vector2u(width, height))) {
		printf("failed to create texture.");
		exit(1);
	}
	sf::Sprite shaderSprite(shaderTexture);

	//loop
	while (window.isOpen()) {
		//mouse position
		sf::Vector2i mp = sf::Mouse::getPosition(window);

		//polling
		for (sf::Event event; window.pollEvent(event);) {
			if (event.type == sf::Event::Closed) window.close();
		}

		//timing
		float actualDeltaTime = deltaClock.restart().asSeconds();
		float deltaTime = std::min(actualDeltaTime, 1 / 60.f);
		std::string fpsStr = std::to_string((int)(1 / actualDeltaTime));
		window.setTitle("Raytraced Rendering @ " + fpsStr + "fps");

		//user input
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Enter)) {
			sunPos = camPos;
		}

		//looking
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) camPitch -= LOOK_SPEED * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) camPitch += LOOK_SPEED * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) camYaw += LOOK_SPEED * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) camYaw -= LOOK_SPEED * deltaTime;
		if (camPitch < .01) camPitch = .01;
		if (camPitch > PI) camPitch = PI;

		//movement
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) camPos.y += MOVE_SPEED * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) camPos.y -= MOVE_SPEED * deltaTime;
		float3 fbDir(cosf(camYaw), 0, sinf(camYaw));
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) camPos += fbDir * MOVE_SPEED * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) camPos -= fbDir * MOVE_SPEED * deltaTime;
		float3 lrDir(fbDir.z, 0, -fbDir.x);
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) camPos -= lrDir * MOVE_SPEED * deltaTime;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) camPos += lrDir * MOVE_SPEED * deltaTime;

		//sphereical coordinates
		camDir.x = cosf(camYaw) * sinf(camPitch);
		camDir.y = cosf(camPitch);
		camDir.z = sinf(camYaw) * sinf(camPitch);

		//update
		//verlet integrate everything
		float subDeltaTime = deltaTime / SUB_STEPS;
		for (int k = 0; k < SUB_STEPS; k++) {
			for (auto& c : constraints) c.update();

			//grav and constraint
			for (auto& p : particles) {
				p.accelerate(grav);

				//drag
				p.accelerate((p.oldpos - p.pos) / deltaTime * COEFF_DRAG);

				p.checkAABB(bounds);
			}

			//particle-particle collisions
			for (auto ait = particles.begin(); ait != particles.end(); ait++) {
				auto& a = *ait;
				for (auto bit = std::next(ait); bit != particles.end(); bit++) {
					auto& b = *bit;
					if (a.getAABB().overlapAABB(b.getAABB())) {
						float totalRad = a.rad + b.rad;
						constraint c(a, b);
						//if intrinsic dist is overlapping
						if (c.restLen < totalRad) {
							//resolve it
							c.restLen = totalRad;
							c.update();
						}
					}
				}
			}

			//integrate
			for (auto& p : particles) p.update(subDeltaTime);
		}

		//render
		window.clear();

		int numSpheres = particles.size();
		if (numSpheres > 0) {
			shader.setUniform("numSpheres", numSpheres);
			int i = 0;
			for (const auto& p : particles) {
				particleInfo[i] = sf::Glsl::Vec4(p.pos.x, p.pos.y, p.pos.z, p.rad);
				i++;
			}
			shader.setUniformArray("spheres", particleInfo, numSpheres);

			//calc viewport
			float3 vUp(0, 1, 0);
			float3 b = cross(vUp, camDir);
			float3 tn = normalize(camDir);
			float3 bn = normalize(b);
			float3 vn = cross(tn, bn);

			//calc viewport sizes
			float gx = tanf(FOV * .5);
			float gy = gx * ((height - 1.) / (width - 1.));

			//stepping vectors
			float3 qx = bn * (2. * gx / (width - 1.));
			float3 qy = vn * (2. * gy / (height - 1.));
			float3 p1m = tn - bn * gx - vn * gy;

			shader.setUniform("sunPos", sf::Glsl::Vec3(sunPos.x, sunPos.y, sunPos.z));
			shader.setUniform("camPos", sf::Glsl::Vec3(camPos.x, camPos.y, camPos.z));
			shader.setUniform("qx", sf::Glsl::Vec3(qx.x, qx.y, qx.z));
			shader.setUniform("qy", sf::Glsl::Vec3(qy.x, qy.y, qy.z));
			shader.setUniform("p1m", sf::Glsl::Vec3(p1m.x, p1m.y, p1m.z));

			window.draw(shaderSprite, &shader);
		}

		window.display();
	}
	delete[] particleInfo;

	return 0;
}