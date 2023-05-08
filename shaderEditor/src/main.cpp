#include <SFML/Graphics.hpp>
using namespace sf;

struct buffer {
	Shader shader;
	Texture texture;
	int mode=0;

	bool create(std::string& shaderStr){
		bool success=shader.loadFromMemory(shaderStr, Shader::Fragment);
		if(success) mode=1;
		return success;
	}

	bool create(std::string& imagePath){
		bool success=texture.loadFromFile(imagePath);
		if (success) mode=2;
		return success;
	}

	Texture render(){
		switch(mode){
			case 0:
				texture.update();
		}
	}
};

int main() {
	//sfml setup
	const unsigned int uWidth=1000;
	const unsigned int uHeight=800;
	RenderWindow rWindow(
		VideoMode(Vector2u(uWidth, uHeight)), "3D Engine", Style::Titlebar|Style::Close);
	Clock cDeltaClock;

	RenderTexture rtRender;
	if (!rtRender.create(Vector2u(uWidth, uHeight))) {
		printf("Error: couldn't create texture");
		exit(1);
	}
	Shader sShader1;
	if (!sShader1.loadFromFile("shader/aberration.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	sShader1.setUniform("resolution", Glsl::Vec2(uWidth, uHeight));
	Shader sShader2;
	if (!sShader2.loadFromFile("shader/crt.glsl", Shader::Fragment)) {
		printf("Error: couldn't load shader");
		exit(1);
	}
	sShader2.setUniform("resolution", Glsl::Vec2(uWidth, uHeight));

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

	//loop
	while (rWindow.isOpen()) {
		//polling
		for (Event event; rWindow.pollEvent(event);) {
			if (event.type==Event::Closed) rWindow.close();
		}

		//update
		float deltaTime=cDeltaClock.restart().asSeconds();
		//rWindow.setTitle("3D Engine @ "+std::to_string(int(1.f/deltaTime))+"fps");

		//clear "buffers"
		/*
		for (int i=0; i<uWidth; i++) {
			for (int j=0; j<uHeight; j++) {
				iPixels.setPixel(Vector2u(i, j), Color(51, 51, 51));
			}
		}
		tPixels.update(iPixels);
		*/
		sShader1.setUniform("mainTex", tPixels);

		rtRender.clear();
		rtRender.draw(sShaderSprite, &sShader1);
		rtRender.display();

		rWindow.clear();
		sShader2.setUniform("mainTex", rtRender.getTexture());
		rWindow.draw(sShaderSprite, &sShader2);
		rWindow.display();
	}

	return 0;
}