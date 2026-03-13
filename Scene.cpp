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
	meshFigurines = {};
	meshWall = NULL;
}

Scene::~Scene()
{
	if(meshCube != NULL)
		delete meshCube;
	if(meshWall != NULL)
		delete meshWall;
	if(meshBase != NULL)
		delete meshBase;
	for(vector<TriangleMesh *>::iterator it=meshFigurines.begin(); it!=meshFigurines.end(); it++)
		delete *it;
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
	int models_count, models_i;
	int map_w, map_h;

	if((meshWall = loadMesh("models/room/Wall.ply")) == NULL)
		return false;
	if((meshBase = loadMesh("models/room/Base.ply")) == NULL)
		return false;

	fin.open(filename);
	if(!fin.is_open())
		return false;

    fin >> map_w >> map_h;
    vector<string> map_grid;
    for (int y = 0; y < map_h; ++y) {
        string row;
        fin >> row;
        map_grid.push_back(row);
    }

	fin >> models_count;
	for (models_i = 0; models_i < models_count; ++models_i) {
		fin >> model_filename;
		TriangleMesh *meshFigurine;
		if((meshFigurine = loadMesh(model_filename)) == NULL)
			break;
		meshFigurines.push_back(meshFigurine);
	}
	if (models_i != models_count)
		return false;

	buildRoom(fin, map_grid, map_w, map_h);
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

void Scene::buildRoom(ifstream &fin, const vector<string> &mapGrid, int mapWidth, int mapHeight)
{
	glm::mat4 transform;
	TriangleMeshInstance *instance;
	int figurines_count, figurines_i;

	// Size of each grid cell
	float tileSize = 1.0f;
	float halfTileSize = tileSize / 2.0f;

	// Offsets to center the map at (0,0) in world space
	float offsetX = -(mapWidth * tileSize) / 2.0f + (tileSize / 2.0f);
    float offsetZ = -(mapHeight * tileSize) / 2.0f + (tileSize / 2.0f);

	for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            char cell = mapGrid[y][x];
            
            // World X and Z coordinates for this tile
            float posX = offsetX + (x * tileSize);
            float posZ = offsetZ + (y * tileSize);

			if (cell == '0') {
				// Floor
				transform = glm::mat4(1.0f);
				transform = glm::translate(transform, glm::vec3(posX, -0.05f, posZ));
				transform = glm::scale(transform, glm::vec3(tileSize, 0.1f, tileSize));
				instance = new TriangleMeshInstance();
				instance->init(meshCube, glm::vec4(0.137f, 0.094f, 0.074f, 1.0f), transform, 0.1f, 0.85f);
				objects.push_back(instance);

				// Ceiling
				transform = glm::mat4(1.0f);
				transform = glm::translate(transform, glm::vec3(posX, 2.05f, posZ));
				transform = glm::scale(transform, glm::vec3(tileSize, 0.1f, tileSize));
				instance = new TriangleMeshInstance();
				instance->init(meshCube, glm::vec4(0.525f, 0.517f, 0.478f, 1.0f), transform, 0.1f, 0.65f);
				objects.push_back(instance);
			}

            // Walls
            if (cell == '1') {
				// Add wall width to its length
				float wallLength = tileSize + 0.0625f;

				// Check where the open spaces or map edges are
                bool openLeft  = (x > 0 && mapGrid[y][x-1] == '0');
                bool openRight = (x < mapWidth - 1 && mapGrid[y][x+1] == '0');
                bool openUp    = (y > 0 && mapGrid[y-1][x] == '0');
                bool openDown  = (y < mapHeight - 1 && mapGrid[y+1][x] == '0');

				// Left boundary (shifted -X)
                if (openLeft) {
                    transform = glm::mat4(1.0f);
                    transform = glm::translate(transform, glm::vec3(posX - halfTileSize, 0.0f, posZ));
                    transform = glm::scale(transform, glm::vec3(tileSize, 2.0f, wallLength));
                    instance = new TriangleMeshInstance();
                    instance->init(meshWall, glm::vec4(1.0f), transform, 0.1f, 0.85f);
                    objects.push_back(instance);
                }

                // Right boundary (shifted +X)
                if (openRight) {
                    transform = glm::mat4(1.0f);
                    transform = glm::translate(transform, glm::vec3(posX + halfTileSize, 0.0f, posZ));
                    transform = glm::scale(transform, glm::vec3(tileSize, 2.0f, wallLength));
                    instance = new TriangleMeshInstance();
                    instance->init(meshWall, glm::vec4(1.0f), transform, 0.1f, 0.85f);
                    objects.push_back(instance);
                }

                // Forward boundary (shifted -Z, rotated 90 deg)
                if (openUp) {
                    transform = glm::mat4(1.0f);
                    transform = glm::translate(transform, glm::vec3(posX, 0.0f, posZ - halfTileSize));
                    transform = glm::rotate(transform, 3.14159f / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    transform = glm::scale(transform, glm::vec3(tileSize, 2.0f, wallLength));
                    instance = new TriangleMeshInstance();
                    instance->init(meshWall, glm::vec4(1.0f), transform, 0.1f, 0.85f);
                    objects.push_back(instance);
                }

                // Backward boundary (shifted +Z, rotated 90 deg)
                if (openDown) {
                    transform = glm::mat4(1.0f);
                    transform = glm::translate(transform, glm::vec3(posX, 0.0f, posZ + halfTileSize));
                    transform = glm::rotate(transform, 3.14159f / 2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                    transform = glm::scale(transform, glm::vec3(tileSize, 2.0f, wallLength));
                    instance = new TriangleMeshInstance();
                    instance->init(meshWall, glm::vec4(1.0f), transform, 0.1f, 0.85f);
                    objects.push_back(instance);
                }
            }
        }
    }

	// Figurines
	fin >> figurines_count;

	for (figurines_i = 0; figurines_i < figurines_count; ++figurines_i) {
		int figurine_type;
		float tx, ty, tz, sx, sy, sz;
		
		fin >> figurine_type;
        fin >> tx >> ty >> tz;
        fin >> sx >> sy >> sz;

		// Convert the coordinates into world coordinates
        float worldX = offsetX + (tx * tileSize);
        float worldZ = offsetZ + (tz * tileSize);

		transform = glm::mat4(1.0f);
		transform = glm::translate(transform, glm::vec3(worldX, ty, worldZ));
        transform = glm::scale(transform, glm::vec3(sx, sy, sz));
		transform = glm::rotate(transform, PI, glm::vec3(0.0f, 1.0f, 0.0f));
		instance = new TriangleMeshInstance();
		instance->init(meshFigurines[figurine_type], glm::vec4(1.0f), transform, 0.15f, 0.4f);
		objects.push_back(instance);

		if (ty > 0.0) {
			// Base
			transform = glm::mat4(1.0f);
			transform = glm::translate(transform, glm::vec3(worldX, 0.0, worldZ));
			transform = glm::scale(transform, glm::vec3(0.5f, 0.75f, 0.5f));
			instance = new TriangleMeshInstance();
			instance->init(meshBase, glm::vec4(1.0f), transform, 0.15f, 0.75f);
			objects.push_back(instance);
		}
	}
}