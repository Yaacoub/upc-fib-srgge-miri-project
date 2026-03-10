#version 150

uniform mat4 projection, model, view;

in vec3 position;
in vec3 normal;
in vec3 colorAttr;
out vec3 normalFrag;
out vec3 fragPos;
out vec3 vertexColor;

void main()
{
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	normalFrag = normalize(normalMatrix * normal);
	fragPos = vec3(model * vec4(position, 1.0));
	vertexColor = colorAttr;
	gl_Position = projection * view * vec4(fragPos, 1.0);
}
