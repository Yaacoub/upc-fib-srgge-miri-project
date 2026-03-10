#ifndef _APPLICATION_INCLUDE
#define _APPLICATION_INCLUDE


#include "Scene.h"
#include "ShaderProgram.h"
#include <GLFW/glfw3.h>


// Application is a singleton (a class with a single instance) that represents our whole app


class Application
{

public:
	Application() {}
	
	
	static Application &instance()
	{
		static Application G;
		
		return G;
	}
	
	void init(GLFWwindow* window);
	bool loadMap(const string &filename);
	bool update(int deltaTime);
	void render();
	
	void resize(int width, int height);
	
	// Input callback methods
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMove(int x, int y);
	void mousePress(int button);
	void mouseRelease(int button);
	
	ShaderProgram *getShader();
	
private:
	void initShaders();


private:
	bool bPlay;						// Continue?
	Scene scene;					// Scene to render
	
	bool directions[4];				// State of directional input
	bool shift; 					// State of shift key (running)

	glm::ivec2 previousMousePos; 	// Previous mouse position
	bool mouseButtons[3];			// State of mouse buttons

	GLFWwindow* mainWindow;			// Main GLFW window
	bool bNavigation;				// Navigation enabled?
	bool bFullscreen; 				// Fullscreen enabled?
	glm::ivec2 windowPos;			// Window position before going fullscreen
	glm::ivec2 windowSize; 			// Window size before going fullscreen
	GLFWmonitor* monitor;			// Monitor used for fullscreen
	
	ShaderProgram basicProgram;		// Shader program used for all GL geometry
	
};


#endif // _APPLICATION_INCLUDE


