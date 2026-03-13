#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE


#include <glm/glm.hpp>
#include <vector>
#include "VectorCamera.h"
#include "TriangleMesh.h"
#include "TriangleMeshInstance.h"


using namespace std;


// Scene contains all the entities of our game.
// It is responsible for updating and render them.


class Scene
{

public:
	Scene();
	~Scene();

	void init();
	bool loadMap(const string &filename);
	TriangleMesh *loadMesh(const string &filename) const;
	void update(int deltaTime);
	void render();

	VectorCamera &getCamera();

private:
	void computeModelViewMatrix();
	
	void buildRoom(ifstream &fin, const vector<string> &mapGrid, int mapWidth, int mapHeight);

private:
	VectorCamera camera;
	TriangleMesh *meshCube, *meshWall, *meshBase;
	vector<TriangleMesh *> meshFigurines;
	vector<TriangleMeshInstance *> objects;
	float currentTime;

};


#endif // _SCENE_INCLUDE

