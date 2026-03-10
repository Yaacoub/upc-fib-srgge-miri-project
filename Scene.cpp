#include <iostream>
#include <cmath>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Scene.h"
#include "PLYReader.h"
#include "Application.h"


Scene::Scene()
{
	meshCube = NULL;
	meshFigurine = NULL;
	meshWall = NULL;
}

Scene::~Scene()
{
	if(meshCube != NULL)
		delete meshCube;
	if(meshFigurine != NULL)
		delete meshFigurine;
	if(meshWall != NULL)
		delete meshWall;
	if(meshBase != NULL)
		delete meshBase;
	for(vector<TriangleMeshInstance *>::iterator it=objects.begin(); it!=objects.end(); it++)
		delete *it;
}


// Initialize the scene. This includes the cube we will use to render
// the floor and ceiling, as well as the camera.

void Scene::init()
{
	meshCube = new TriangleMesh();
	meshCube->buildCube();
	meshCube->sendToOpenGL();
	currentTime = 0.0f;
	
	camera.init(glm::vec3(0.f, 1.0f, 2.f));
}

// Load the map & all its associated models

bool Scene::loadMap(const string &filename)
{
	ifstream fin;
	string model_filename;
	
	fin.open(filename);
	if(!fin.is_open())
		return false;
	fin >> model_filename;
	if((meshFigurine = loadMesh(model_filename)) == NULL)
		return false;
	fin >> model_filename;
	if((meshWall = loadMesh(model_filename)) == NULL)
		return false;
	fin >> model_filename;
	if((meshBase = loadMesh(model_filename)) == NULL)
		return false;
	buildRoom();
	
	return true;
}

// Loads the mesh into CPU memory and sends it to GPU memory (using GL)

TriangleMesh *Scene::loadMesh(const string &filename) const
{
	TriangleMesh *mesh;
#pragma warning( push )
#pragma warning( disable : 4101)
	PLYReader reader;
#pragma warning( pop ) 

	mesh = new TriangleMesh();
	bool bSuccess = reader.readMesh(filename, *mesh);
	if(bSuccess)
		mesh->sendToOpenGL();
	else
	{
		delete mesh;
		mesh = NULL;
	}
	
	return mesh;
}

void Scene::update(int deltaTime)
{
	currentTime += deltaTime;
}

// Render the scene. First the room, then the mesh it there is one loaded.

void Scene::render()
{
	Application::instance().getShader()->use();
	camera.render();
	for(vector<TriangleMeshInstance *>::iterator it=objects.begin(); it!=objects.end(); it++)
		(*it)->render();
}

VectorCamera &Scene::getCamera()
{
	return camera;
}

// Init & render the room. Both the floor and the walls are instances of the
// same initial cube scaled and translated to build the room.

void Scene::buildRoom()
{
	glm::mat4 transform;
	TriangleMeshInstance *instance;

	//object = new TriangleMeshInstance();
	//object->init(mesh, glm::vec4(1.0f), glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), 0.2f, 0.3f);

	// Create instance for the floor
	// Build model transform matrix, create instance from cube, and add it to walls vector	
	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.f, -0.05f, 0.f));
	transform = glm::scale(transform, glm::vec3(20.f, 0.1f, 20.f));
	instance = new TriangleMeshInstance();
	instance->init(meshCube, glm::vec4(0.137f, 0.094f, 0.074f, 1.0f), transform, 0.1f, 0.85f);
	objects.push_back(instance);

	// Ceiling
	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.f, 2.05f, 0.f));
	transform = glm::scale(transform, glm::vec3(20.f, 0.1f, 20.f));
	instance = new TriangleMeshInstance();
	instance->init(meshCube, glm::vec4(0.525f, 0.517f, 0.478f, 1.0f), transform, 0.1f, 0.65f);
	objects.push_back(instance);

	// Walls
	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.f, 0.f, -9.96875));
	transform = glm::scale(transform, glm::vec3(20.f, 2.f, 1.f));
	transform = glm::rotate(transform, PI/2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	instance = new TriangleMeshInstance();
	instance->init(meshWall, glm::vec4(1.0f), transform, 0.1f, 0.85f);
	objects.push_back(instance);

	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.f, 0.f, 9.96875f));
	transform = glm::scale(transform, glm::vec3(20.f, 2.f, 1.f));
	transform = glm::rotate(transform, PI/2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	instance = new TriangleMeshInstance();
	instance->init(meshWall, glm::vec4(1.0f), transform, 0.0f, 0.9f);
	objects.push_back(instance);

	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(-9.96875f, 0.f, 0.f));
	transform = glm::scale(transform, glm::vec3(1.f, 2.f, 20.f));
	instance = new TriangleMeshInstance();
	instance->init(meshWall, glm::vec4(1.0f), transform, 0.0f, 0.9f);
	objects.push_back(instance);

	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(9.96875f, 0.f, 0.f));
	transform = glm::scale(transform, glm::vec3(1.f, 2.f, 20.f));
	instance = new TriangleMeshInstance();
	instance->init(meshWall, glm::vec4(1.0f), transform, 0.0f, 0.9f);
	objects.push_back(instance);

	// Base
	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.0, 0.0f, -1.0f));
	transform = glm::scale(transform, glm::vec3(0.5f, 0.75f, 0.5f));
	instance = new TriangleMeshInstance();
	instance->init(meshBase, glm::vec4(1.0f), transform, 0.15f, 0.75f);
	objects.push_back(instance);

	// Figurine
	transform = glm::mat4(1.0f);
	transform = glm::translate(transform, glm::vec3(0.0, 0.75f, -1.0f));
	transform = glm::scale(transform, glm::vec3(0.5f, 0.5f, 0.5f));
	transform = glm::rotate(transform, PI, glm::vec3(0.0f, 1.0f, 0.0f));
	instance = new TriangleMeshInstance();
	instance->init(meshFigurine, glm::vec4(1.0f), transform, 0.15f, 0.4f);
	objects.push_back(instance);
}




