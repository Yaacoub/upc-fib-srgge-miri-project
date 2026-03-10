#include <iostream>
#include "Application.h"


#define MOVEMENT_SPEED 2.0f
#define ROTATION_DELTA_HEAD 0.05f
#define ROTATION_DELTA_PITCH 0.05f


// Initialize GL and the attributes of Application

void Application::init(GLFWwindow* window)
{
	bPlay = true;

	// Init OpenGL
	glClearColor(0.08f, 0.08f, 0.08f, 1.0f); 		// Slightly dark background to see shading
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Init GLFW
	mainWindow = window;
	monitor = glfwGetPrimaryMonitor();
	// Initialize the scene object
	initShaders();
	scene.init();
	
	// State attributes needed to track keyboard & mouse
	for(unsigned int i=0; i<4; i++)
		directions[i] = false;
	mouseButtons[0] = false;
	mouseButtons[1] = false;
	bNavigation = false;
}

// Load the map into the scene

bool Application::loadMap(const string &filename)
{
	return scene.loadMap(filename);
}

// Update any animations or state in the scene
// Also move using the keyboard if in navigation mode

bool Application::update(int deltaTime)
{
	scene.update(deltaTime);
	
	if(bNavigation)
	{
		float move = deltaTime / 1000.f * MOVEMENT_SPEED;
		if(shift)
			move *= 3.0f;
		if(directions[0])
			scene.getCamera().strafe(move);
		if(directions[1])
			scene.getCamera().strafe(-move);
		if(directions[2])
			scene.getCamera().moveForward(-move);
		if(directions[3])
			scene.getCamera().moveForward(move);
	}
	
	return bPlay;
}

// Render the scene

void Application::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	scene.render();
}

// Resizing the window should change the size GL viewport
// as well as adapt the camera to the new aspect ratio

void Application::resize(int width, int height)
{
	glViewport(0, 0, width, height);
	scene.getCamera().resizeCameraViewport(width, height);
}

// Process keyboard events. Escape exits the application

void Application::keyPressed(int key)
{
	switch(key)
	{
	case GLFW_KEY_ESCAPE:
		bPlay = false;
		break;
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_LEFT:
	case GLFW_KEY_DOWN:
	case GLFW_KEY_UP:
		directions[key - GLFW_KEY_RIGHT] = true;
		break;
	case GLFW_KEY_D:
		directions[0] = true;
		break;
	case GLFW_KEY_A:
		directions[1] = true;
		break;
	case GLFW_KEY_S:
		directions[2] = true;
		break;
	case GLFW_KEY_W:
		directions[3] = true;
		break;
	case GLFW_KEY_LEFT_SHIFT :
		shift = true;
		break;
	}
}

// F1 enters and exits the navigation mode
// F5 does the same for fullscreen mode

void Application::keyReleased(int key)
{
	switch(key)
	{
	case GLFW_KEY_RIGHT:
	case GLFW_KEY_LEFT:
	case GLFW_KEY_DOWN:
	case GLFW_KEY_UP:
		directions[key - GLFW_KEY_RIGHT] = false;
		break;
	case GLFW_KEY_D:
		directions[0] = false;
		break;
	case GLFW_KEY_A:
		directions[1] = false;
		break;
	case GLFW_KEY_S:
		directions[2] = false;
		break;
	case GLFW_KEY_W:
		directions[3] = false;
		break;
	case GLFW_KEY_LEFT_SHIFT :
		shift = false;
		break;
	case GLFW_KEY_F1:
		bNavigation = !bNavigation;
		if(bNavigation)
			glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		else
			glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		break;
	case GLFW_KEY_F5:
		bFullscreen = !bFullscreen;
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if(bFullscreen)
		{
			int xpos, ypos;
			glfwGetWindowPos(mainWindow, &xpos, &ypos);
			windowPos = glm::ivec2(xpos, ypos);

			int width, height;
			glfwGetWindowSize(mainWindow, &width, &height);
			windowSize = glm::ivec2(width, height);

			glfwSetWindowMonitor(mainWindow, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
		}
		else
			glfwSetWindowMonitor(mainWindow, NULL, windowPos.x, windowPos.y, windowSize.x, windowSize.y, GLFW_DONT_CARE);
		break;
	}
}

// Moving the mouse while in navigation mode rotates the camera

void Application::mouseMove(int x, int y)
{
	if(bNavigation)
	{
		glm::ivec2 move = glm::ivec2(previousMousePos.x - x, y - previousMousePos.y);
		if((move.x != 0) || (move.y != 0))
		{
			if(move.x != 0)
				scene.getCamera().rotateCamera(ROTATION_DELTA_HEAD * move.x);
			if(move.y != 0)
				scene.getCamera().changePitch(ROTATION_DELTA_PITCH * move.y);
		}
	}
	previousMousePos = glm::ivec2(x, y);
}

// Process mouse events

void Application::mousePress(int button)
{
	mouseButtons[button] = true;
}

void Application::mouseRelease(int button)
{
	mouseButtons[button] = false;
}

// Load, compile, and link the vertex and fragment shader

void Application::initShaders()
{
	Shader vShader, fShader;

	vShader.initFromFile(VERTEX_SHADER, "shaders/basic.vert");
	if(!vShader.isCompiled())
	{
		cout << "Vertex Shader Error" << endl;
		cout << "" << vShader.log() << endl << endl;
	}
	fShader.initFromFile(FRAGMENT_SHADER, "shaders/basic.frag");
	if(!fShader.isCompiled())
	{
		cout << "Fragment Shader Error" << endl;
		cout << "" << fShader.log() << endl << endl;
	}
	basicProgram.init();
	basicProgram.addShader(vShader);
	basicProgram.addShader(fShader);
	basicProgram.link();
	if(!basicProgram.isLinked())
	{
		cout << "Shader Linking Error" << endl;
		cout << "" << basicProgram.log() << endl << endl;
	}
	basicProgram.bindFragmentOutput("outColor");
	vShader.free();
	fShader.free();
}

ShaderProgram *Application::getShader()
{
	return &basicProgram;
}




