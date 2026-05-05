#include "Application.h"
#include "TriangleMeshInstance.h"
#include <glm/gtc/matrix_transform.hpp>


TriangleMeshInstance::TriangleMeshInstance()
{
	for (int i = 0; i < LOD_LEVELS; ++i) meshes[i] = nullptr;
	metallic = 0.0f;
	roughness = 1.0f;
	currentLOD = 0;
	isLODEnabled = false;
}

TriangleMeshInstance::~TriangleMeshInstance()
{
}


void TriangleMeshInstance::init(const vector<TriangleMesh*> &meshes, const glm::vec4 &color, const glm::mat4 &transform, float metallic, float roughness)
{
	this->color = color;
	this->transform = transform;
	this->metallic = metallic;
	this->roughness = roughness;
	this->currentLOD = 0;
    this->isLODEnabled = (meshes.size() > 1);

	for (int i = 0; i < LOD_LEVELS; ++i) {
        this->meshes[i] = (i < meshes.size()) ? meshes[i] : meshes[0];
    }
}

void TriangleMeshInstance::render(bool showLODColors)
{
	TriangleMesh* mesh = meshes[currentLOD];
	if(mesh != NULL)
	{
		Application::instance().getShader()->use();
		Application::instance().getShader()->setUniform1f("metallic", metallic);
		Application::instance().getShader()->setUniform1f("roughness", roughness);
		Application::instance().getShader()->setUniformMatrix4f("model", transform);
		
		glm::vec4 renderColor = color;
		if (showLODColors && isLODEnabled) {
			switch (currentLOD) {
				case 0: renderColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); break; // White  (LOD 1 - Highest)
				case 1: renderColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f); break; // Blue   (LOD 2)
				case 2: renderColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); break; // Yellow (LOD 3)
				case 3: renderColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); break; // Red    (LOD 4 - Lowest)
			}
		}
		Application::instance().getShader()->setUniform4f("color", renderColor.r, renderColor.g, renderColor.b, renderColor.a);
		Application::instance().getShader()->setUniform1i("showLODColors", showLODColors && isLODEnabled);

		mesh->render();
	}
}

TriangleMesh *TriangleMeshInstance::getMesh()
{
	TriangleMesh* mesh = meshes[currentLOD];
	return meshes[currentLOD];
}

void TriangleMeshInstance::setTransform(const glm::mat4 &transform)
{
	this->transform = transform;
}

const glm::mat4 &TriangleMeshInstance::getTransform() const
{
	return transform;
}

void TriangleMeshInstance::addTransform(const glm::mat4 &addedTransform)
{
	transform = addedTransform * transform;
}

void TriangleMeshInstance::resetTransform()
{
	transform = glm::mat4(1.0f);
}

void TriangleMeshInstance::translate(const glm::vec3 &move)
{
	transform = glm::translate(glm::mat4(1.0f), move) * transform;
}

void TriangleMeshInstance::rotate(const glm::vec3 &axis, float angleDegrees)
{
	transform = glm::rotate(glm::mat4(1.0f), angleDegrees * 3.14159f / 180.f, axis) * transform;
}

void TriangleMeshInstance::scale(const glm::vec3 &factor)
{
	transform = glm::scale(glm::mat4(1.0f), factor) * transform;
}

void TriangleMeshInstance::setColor(const glm::vec4 &color)
{
	this->color = color;
}

const glm::vec4 &TriangleMeshInstance::getColor() const
{
	return color;
}

void TriangleMeshInstance::setMaterial(float metallic, float roughness)
{
	this->metallic = metallic;
	this->roughness = roughness;
}

void TriangleMeshInstance::setCurrentLOD(int LOD)
{
	this->currentLOD = LOD;
}

float TriangleMeshInstance::getMetallic() const
{
	return metallic;
}

float TriangleMeshInstance::getRoughness() const
{
	return roughness;
}

int TriangleMeshInstance::getCurrentLOD() const
{
	return currentLOD;
}

bool TriangleMeshInstance::getIsLODEnabled() const
{
	return isLODEnabled;
}

float TriangleMeshInstance::getBoundingBoxDiagonal() const {
    if (!meshes[0]) return 0.0f; 

    float localDiagonal = meshes[0]->getBoundingBoxDiagonal();
    glm::vec3 scale(
        glm::length(glm::vec3(transform[0])),
        glm::length(glm::vec3(transform[1])),
        glm::length(glm::vec3(transform[2]))
    );
    float maxScale = std::max({scale.x, scale.y, scale.z});

    return localDiagonal * maxScale;
}