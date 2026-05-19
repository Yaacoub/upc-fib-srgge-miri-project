#ifndef _TRIANGLE_MESH_INSTANCE_INCLUDE
#define _TRIANGLE_MESH_INSTANCE_INCLUDE


#include <glm/glm.hpp>
#include "TriangleMesh.h"


#define LOD_LEVELS 4


class TriangleMeshInstance
{

public:
	TriangleMeshInstance();
	~TriangleMeshInstance();
	
	void init(const vector<TriangleMesh*> &meshes, const glm::vec4 &color = glm::vec4(1.0f), const glm::mat4 &transform = glm::mat4(1.0f), float metallic = 0.0f, float roughness = 1.0f);
	void render(bool showLODColors);
	
	TriangleMesh *getMesh();
	
	void setTransform(const glm::mat4 &transform);
	const glm::mat4 &getTransform() const;
	void addTransform(const glm::mat4 &addedTransform);
	
	void resetTransform();
	void translate(const glm::vec3 &move);
	void rotate(const glm::vec3 &axis, float angleDegrees);
	void scale(const glm::vec3 &factor);
	
	void setColor(const glm::vec4 &color);
	const glm::vec4 &getColor() const;
	void setMaterial(float metallic, float roughness);
	void setCurrentLOD(int LOD);
	void setWaitFrames(int frames);
	void setCell(int x, int y);

	void updateWaitFrames();

	float getMetallic() const;
	float getRoughness() const;
	int getCurrentLOD() const;
	bool getIsLODEnabled() const;
	int getWaitFrames() const;
	int getCellX() const;
	int getCellY() const;

	float getBoundingBoxDiagonal() const;
	int getTriangleCount(int LOD) const { return meshes[LOD]->getTriangleCount(); }

	
private:
	TriangleMesh* meshes[LOD_LEVELS];
	glm::vec4 color;
	glm::mat4 transform;
	float metallic;
	float roughness;
	int currentLOD;
	bool isLODEnabled;
	int waitFrames;
	int cellX = 0;
	int cellY = 0;

};


#endif // _TRIANGLE_MESH_INSTANCE_INCLUDE