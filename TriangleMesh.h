#ifndef _TRIANGLE_MESH_INCLUDE
#define _TRIANGLE_MESH_INCLUDE


#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "ShaderProgram.h"


using namespace std;


// Class TriangleMesh contains the geometry of a mesh built out of triangles.
// Both the vertices and the triangles are stored in vectors.
// TriangleMesh also manages the ids of the copy in the GPU, so as to 
// be able to render it using OpenGL.


class TriangleMesh
{

public:
	TriangleMesh();
	~TriangleMesh();

	void addVertex(const glm::vec3 &position);
	void addTriangle(int v0, int v1, int v2);

	void initVertices(const vector<float> &newVertices, const vector<float> &newColors);
	void initTriangles(const vector<int> &newTriangles);

	void buildCube();
	
	void sendToOpenGL();
	void render() const;
	void free();

private:
	vector<glm::vec3> vertices;
	vector<glm::vec3> colors;
	vector<int> triangles;

	GLuint vao;
	GLuint vbo;
	GLint posLocation, normalLocation, colorLocation;
	
};


#endif // _TRIANGLE_MESH_INCLUDE

