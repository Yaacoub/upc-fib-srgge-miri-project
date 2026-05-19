#ifndef _VECTOR_CAMERA_INCLUDE
#define _VECTOR_CAMERA_INCLUDE


#include <glm/glm.hpp>


#define PI 3.14159f


// VectorCamera contains the properies of the camera the scene is using
// It is responsible for computing the associated GL matrices


class VectorCamera
{

public:
	VectorCamera();
	~VectorCamera();
	
	void init(const glm::vec3 &initPosition);
	
	void resizeCameraViewport(int width, int height);
	void rotateCamera(float rotation);
	void changePitch(float rotation);
	void moveForward(float distance);
	void strafe(float distance);

	glm::mat4 &getProjectionMatrix();
	glm::mat4 &getViewMatrix();
	const glm::vec3 &getPosition() const;
	
	void render();

private:
	void computeViewMatrix();

private:
	glm::vec3 position;					// Camera parameters
	float angleDirection, anglePitch;
	float rangeDistanceCamera[2];
	glm::mat4 projection, view;			// OpenGL matrices

};


#endif // _VECTOR_CAMERA_INCLUDE
