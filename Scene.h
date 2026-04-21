#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE


#include <eigen3/Eigen/Dense>
#include <glm/glm.hpp>
#include <unordered_map>
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
	TriangleMesh *loadMesh(const string &filename, int lod_level) const;
	TriangleMesh *simplifyMeshAVG(const TriangleMesh* input_mesh, float cell_size) const;
	TriangleMesh *simplifyMeshQEM(const TriangleMesh* input_mesh, float cell_size) const;
	TriangleMesh *simplifyMeshQEMClustering(const TriangleMesh* input_mesh, float cell_size) const;
	void savePLYBinary(const string& filename, const TriangleMesh* mesh) const;
	void update(int deltaTime);
	void render();

	VectorCamera &getCamera();

	struct Cell {
		int count = 0;
		int id = -1; // ID of the representative vertex
		glm::vec3 center = glm::vec3(0.0f);
		glm::vec3 sum = glm::vec3(0.0f);
    	glm::vec3 color_sum = glm::vec3(0.0f);
		Eigen::Matrix4d quadric_sum = Eigen::Matrix4d::Zero();
	};

	struct TupleHash {
		size_t operator()(const tuple<int, int, int, int>& t) const {
			size_t h1 = hash<int>{}(get<0>(t));
			size_t h2 = hash<int>{}(get<1>(t));
			size_t h3 = hash<int>{}(get<2>(t));
			size_t h4 = hash<int>{}(get<3>(t));
			// Combine the hashes (simple combination)
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3); 
		}
	};

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

