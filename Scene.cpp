#include <iostream>
#include <cmath>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Scene.h"
#include "PLYReader.h"
#include "Application.h"


#define LOD_LEVELS 4


Scene::Scene()
{
}

Scene::~Scene()
{
	for (auto mesh: meshCube) delete mesh;
	for (auto mesh: meshWall) delete mesh;
	for (auto mesh: meshBase) delete mesh;
	for (auto& figurineGroup : meshFigurines) for (auto mesh : figurineGroup) delete mesh;
	for (auto obj : objects)  delete obj;
}


// Initialize the scene. This includes the cube we will use to render
// the floor and ceiling, as well as the camera.

void Scene::init()
{
	TriangleMesh* cube = new TriangleMesh();
	cube->buildCube();
	cube->sendToOpenGL();
	meshCube.push_back(cube);
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

	meshWall = loadMesh("models/room/Wall.ply", false);
	meshBase = loadMesh("models/room/Base.ply", false);

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
		vector<TriangleMesh*> figurineLODs = loadMesh(model_filename, true);
		meshFigurines.push_back(figurineLODs);
	}
	if (models_i != models_count)
		return false;

	buildRoom(fin, map_grid, map_w, map_h);
	return true;
}

// Loads the mesh into CPU memory and sends it to GPU memory (using GL)

vector<TriangleMesh*> Scene::loadMesh(const string &filename, bool isLODEnabled) const
{
	vector<TriangleMesh*> meshes;
#pragma warning( push )
#pragma warning( disable : 4101)
	PLYReader reader;
#pragma warning( pop ) 

	// Load the original mesh
	if (!isLODEnabled) {
		TriangleMesh* mesh = new TriangleMesh();
		if (reader.readMesh(filename, *mesh)) {
			mesh->sendToOpenGL();
			meshes.push_back(mesh);
		} else {
			delete mesh;
			mesh = nullptr;
		}
	}
	
	// Handle LOD generation and loading
	else {
		float resolutions[LOD_LEVELS] = {1024.0f, 512.0f, 256.0f, 128.0f};
		for (int lod_level = 0; lod_level < LOD_LEVELS; ++lod_level) {
			TriangleMesh* mesh = new TriangleMesh();
			string base_filename = filename.substr(0, filename.find_last_of("."));
			string lod_filename = base_filename + "_LOD" + to_string(lod_level) + ".ply";
			ifstream file_check(lod_filename);

			if (!file_check.good()) {
				file_check.close();
				TriangleMesh original_mesh;
				reader.readMesh(filename, original_mesh);

				float target_resolution = resolutions[lod_level];
				TriangleMesh* simplified_mesh = simplifyMeshQEMClustering(&original_mesh, 1.0f / target_resolution);
				savePLYBinary(lod_filename, simplified_mesh);

				delete mesh;
				mesh = simplified_mesh; 
			} else {
				file_check.close();
				reader.readMesh(lod_filename, *mesh);
			}

			mesh->sendToOpenGL();
			meshes.push_back(mesh);
		}
	}

	return meshes;
}

TriangleMesh *Scene::simplifyMeshAVG(const TriangleMesh* mesh, float cell_size) const
{
	unordered_map<tuple<int, int, int, int>, Scene::Cell, TupleHash> grid;
	TriangleMesh* mesh_simplified = new TriangleMesh();

	const vector<glm::vec3> &input_colors = mesh->get_colors();
	const vector<glm::vec3> &input_vertices = mesh->get_vertices();
	const vector<int> &input_triangles = mesh->get_triangles();

	for (size_t v = 0; v < input_vertices.size(); ++v) {
		// Determine node that contain the original vertex
		glm::vec3 vertex = input_vertices[v];
		int normal_id = 0;
		int grid_x = static_cast<int>(floor(vertex.x / cell_size));
		int grid_y = static_cast<int>(floor(vertex.y / cell_size));
		int grid_z = static_cast<int>(floor(vertex.z / cell_size));
		tuple<int, int, int, int> cell_key = make_tuple(grid_x, grid_y, grid_z, normal_id);
		
		grid[cell_key].sum += vertex;
		grid[cell_key].color_sum += input_colors[v];
		grid[cell_key].count += 1;
	}

	for (auto& pair : grid) {
		Scene::Cell &cell = pair.second;
		// Add representative to output mesh
		glm::vec3 average_color = cell.color_sum / (float)cell.count;
		glm::vec3 average_vertex = cell.sum / (float)cell.count;
		mesh_simplified->addVertexAndColor(average_vertex, average_color);
		// Store the id of that output vertex in the node
		cell.id = mesh_simplified->get_vertices().size() - 1; 
	}

	for (size_t t = 0; t < input_triangles.size(); t += 3) {
		int vertices_i[3] = { input_triangles[t], input_triangles[t+1], input_triangles[t+2] };
		int vertices_new_i[3];

		for (int i = 0; i < 3; ++i) {
			// Determine nodes that contain the original vertices
			glm::vec3 vertex = input_vertices[vertices_i[i]];
			int normal_id = 0;
			int grid_x = static_cast<int>(floor(vertex.x / cell_size));
			int grid_y = static_cast<int>(floor(vertex.y / cell_size));
			int grid_z = static_cast<int>(floor(vertex.z / cell_size));
			tuple<int, int, int, int> cell_key = make_tuple(grid_x, grid_y, grid_z, normal_id);
			// Update id using the one stored in the grid
			vertices_new_i[i] = grid[cell_key].id;
		}

		// If triangle has unique ids output it
		if (vertices_new_i[0] != vertices_new_i[1] && vertices_new_i[1] != vertices_new_i[2] && vertices_new_i[2] != vertices_new_i[0]) {
			mesh_simplified->addTriangle(vertices_new_i[0], vertices_new_i[1], vertices_new_i[2]);
		}
	}

	return mesh_simplified;
}

TriangleMesh *Scene::simplifyMeshQEM(const TriangleMesh* mesh, float cell_size) const
{
	unordered_map<tuple<int, int, int, int>, Scene::Cell, TupleHash> grid;
	TriangleMesh* mesh_simplified = new TriangleMesh();

	const vector<glm::vec3> &input_colors = mesh->get_colors();
	const vector<glm::vec3> &input_vertices = mesh->get_vertices();
	const vector<int> &input_triangles = mesh->get_triangles();

	for (size_t t = 0; t < input_triangles.size(); t += 3) {
		int vertices_i[3] = { input_triangles[t], input_triangles[t+1], input_triangles[t+2] };

		for (int i = 0; i < 3; ++i) {
			// Determine node that contain the original vertex
			glm::vec3 vertex = input_vertices[vertices_i[i]];
			int normal_id = 0;
			int grid_x = static_cast<int>(floor(vertex.x / cell_size));
			int grid_y = static_cast<int>(floor(vertex.y / cell_size));
			int grid_z = static_cast<int>(floor(vertex.z / cell_size));
			tuple<int, int, int, int> cell_key = make_tuple(grid_x, grid_y, grid_z, normal_id);

			grid[cell_key].sum += vertex;
        	grid[cell_key].color_sum += input_colors[vertices_i[i]];
        	grid[cell_key].count += 1;

			// Compute quadric from plane
			// Compute plane of T in coordinates relative to the node’s center
			glm::vec3 node_center((grid_x + 0.5f) * cell_size, (grid_y + 0.5f) * cell_size,(grid_z + 0.5f) * cell_size);
			grid[cell_key].center = node_center;

			glm::vec3 e1 = input_vertices[vertices_i[1]] - input_vertices[vertices_i[0]];
			glm::vec3 e2 = input_vertices[vertices_i[2]] - input_vertices[vertices_i[0]];
			glm::vec3 normal = glm::normalize(glm::cross(e1, e2));
			glm::vec4 q = glm::vec4(normal, -glm::dot(normal, input_vertices[vertices_i[0]] - node_center));
			Eigen::Vector4d qq(q[0], q[1], q[2], q[3]);
			Eigen::Matrix4d Qq = qq * qq.transpose();

			// Add quadric to matrix inside node
			grid[cell_key].quadric_sum += Qq;
		}
	}

	// Compute representatives using quadrics and update triangles
	for (auto& pair : grid) {
		Scene::Cell &cell = pair.second;

		Eigen::Matrix4d A = cell.quadric_sum;
		Eigen::Vector4d B(0, 0, 0, 1);
		A.row(3) << 0, 0, 0, 1;
		
		Eigen::JacobiSVD<Eigen::Matrix4d> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
		Eigen::Matrix<double, 4, 1> sigma = svd.singularValues();
		Eigen::Matrix4d sigma_plus = Eigen::Matrix4d::Zero();
		for (int i = 0; i < 4; ++i) {
			if (sigma(i) > 1e-6) sigma_plus(i, i) = 1.0 / sigma(i);
		}
		
		Eigen::Matrix4d A_plus = svd.matrixV() * sigma_plus * svd.matrixU().transpose();
		Eigen::Vector4d p_star = A_plus * B;

		// Add representative to output mesh
		glm::vec3 final_vertex = glm::vec3(p_star[0], p_star[1], p_star[2]) + cell.center;
		glm::vec3 average_color = cell.color_sum / (float)cell.count; // Weighted average
		mesh_simplified->addVertexAndColor(final_vertex, average_color);

		// Store the id of that output vertex in the node
		cell.id = mesh_simplified->get_vertices().size() - 1; 
	}

	for (size_t t = 0; t < input_triangles.size(); t += 3) {
		int vertices_i[3];
		int vertices_new_i[3];
		vertices_i[0] = input_triangles[t];
		vertices_i[1] = input_triangles[t+1];
		vertices_i[2] = input_triangles[t+2];

		for (int i = 0; i < 3; ++i) {
			// Determine nodes that contain the original vertices
			glm::vec3 vertex = input_vertices[vertices_i[i]];
			int normal_id = 0;
			int grid_x = static_cast<int>(floor(vertex.x / cell_size));
			int grid_y = static_cast<int>(floor(vertex.y / cell_size));
			int grid_z = static_cast<int>(floor(vertex.z / cell_size));
			tuple<int, int, int, int> cell_key = make_tuple(grid_x, grid_y, grid_z, normal_id);
			// Update id using the one stored in the grid
			vertices_new_i[i] = grid[cell_key].id;
		}

		// If triangle has unique ids output it
		if (vertices_new_i[0] != vertices_new_i[1] && vertices_new_i[1] != vertices_new_i[2] && vertices_new_i[2] != vertices_new_i[0]) {
			mesh_simplified->addTriangle(vertices_new_i[0], vertices_new_i[1], vertices_new_i[2]);
		}
	}

	return mesh_simplified;
}

TriangleMesh *Scene::simplifyMeshQEMClustering(const TriangleMesh* mesh, float cell_size) const
{
	unordered_map<tuple<int, int, int, int>, Scene::Cell, TupleHash> grid;
	TriangleMesh* mesh_simplified = new TriangleMesh();

	const vector<glm::vec3> &input_colors = mesh->get_colors();
	const vector<glm::vec3> &input_vertices = mesh->get_vertices();
	const vector<int> &input_triangles = mesh->get_triangles();

	// Normal clustering
	vector<glm::vec3> vertex_normals(input_vertices.size(), glm::vec3(0.0f));
	for (size_t t = 0; t < input_triangles.size(); t += 3) {
		int v0 = input_triangles[t];
		int v1 = input_triangles[t+1];
		int v2 = input_triangles[t+2];
		
		glm::vec3 e1 = input_vertices[v1] - input_vertices[v0];
		glm::vec3 e2 = input_vertices[v2] - input_vertices[v0];
		glm::vec3 normal = glm::cross(e1, e2);
		
		vertex_normals[v0] += normal;
		vertex_normals[v1] += normal;
		vertex_normals[v2] += normal;
	}

	for (size_t t = 0; t < input_triangles.size(); t += 3) {
		int vertices_i[3] = { input_triangles[t], input_triangles[t+1], input_triangles[t+2] };

		glm::vec3 e1 = input_vertices[vertices_i[1]] - input_vertices[vertices_i[0]];
		glm::vec3 e2 = input_vertices[vertices_i[2]] - input_vertices[vertices_i[0]];
		glm::vec3 face_normal = glm::normalize(glm::cross(e1, e2));

		for (int i = 0; i < 3; ++i) {
			glm::vec3 vertex = input_vertices[vertices_i[i]];
			
			glm::vec3 vertex_normal = vertex_normals[vertices_i[i]];
			int normal_id = 0;
			if (vertex_normal.x >= 0.0f) normal_id |= 1; 
			if (vertex_normal.y >= 0.0f) normal_id |= 2; 
			if (vertex_normal.z >= 0.0f) normal_id |= 4;

			int grid_x = static_cast<int>(floor(vertex.x / cell_size));
			int grid_y = static_cast<int>(floor(vertex.y / cell_size));
			int grid_z = static_cast<int>(floor(vertex.z / cell_size));
			tuple<int, int, int, int> cell_key = make_tuple(grid_x, grid_y, grid_z, normal_id);

			grid[cell_key].sum += vertex;
			grid[cell_key].color_sum += input_colors[vertices_i[i]];
			grid[cell_key].count += 1;

			// Compute quadric from plane
			// Compute plane of T in coordinates relative to the node’s center
			glm::vec3 node_center((grid_x + 0.5f) * cell_size, (grid_y + 0.5f) * cell_size,(grid_z + 0.5f) * cell_size);
			grid[cell_key].center = node_center;

			glm::vec4 q = glm::vec4(face_normal, -glm::dot(face_normal, input_vertices[vertices_i[0]] - node_center));
			Eigen::Vector4d qq(q[0], q[1], q[2], q[3]);
			Eigen::Matrix4d Qq = qq * qq.transpose();

			// Add quadric to matrix inside node
			grid[cell_key].quadric_sum += Qq;
		}
	}

	// Compute representatives using quadrics and update triangles
	for (auto& pair : grid) {
		Scene::Cell &cell = pair.second;

		Eigen::Matrix4d A = cell.quadric_sum;
		Eigen::Vector4d B(0, 0, 0, 1);
		A.row(3) << 0, 0, 0, 1;
		
		Eigen::JacobiSVD<Eigen::Matrix4d> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
		Eigen::Matrix<double, 4, 1> sigma = svd.singularValues();
		Eigen::Matrix4d sigma_plus = Eigen::Matrix4d::Zero();
		for (int i = 0; i < 4; ++i) {
			if (sigma(i) > 1e-6) sigma_plus(i, i) = 1.0 / sigma(i);
		}
		
		Eigen::Matrix4d A_plus = svd.matrixV() * sigma_plus * svd.matrixU().transpose();
		Eigen::Vector4d p_star = A_plus * B;

		// Add representative to output mesh
		glm::vec3 final_vertex = glm::vec3(p_star[0], p_star[1], p_star[2]) + cell.center;
		glm::vec3 average_color = cell.color_sum / (float)cell.count; // Weighted average
		mesh_simplified->addVertexAndColor(final_vertex, average_color);

		// Store the id of that output vertex in the node
		cell.id = mesh_simplified->get_vertices().size() - 1; 
	}

	for (size_t t = 0; t < input_triangles.size(); t += 3) {
		int vertices_i[3] = { input_triangles[t], input_triangles[t+1], input_triangles[t+2] };
		int vertices_new_i[3];

		for (int i = 0; i < 3; ++i) {
			glm::vec3 vertex = input_vertices[vertices_i[i]];
			
			glm::vec3 vertex_normal = vertex_normals[vertices_i[i]];
			int normal_id = 0;
			if (vertex_normal.x >= 0.0f) normal_id |= 1; 
			if (vertex_normal.y >= 0.0f) normal_id |= 2; 
			if (vertex_normal.z >= 0.0f) normal_id |= 4;

			int grid_x = static_cast<int>(floor(vertex.x / cell_size));
			int grid_y = static_cast<int>(floor(vertex.y / cell_size));
			int grid_z = static_cast<int>(floor(vertex.z / cell_size));
			tuple<int, int, int, int> cell_key = make_tuple(grid_x, grid_y, grid_z, normal_id);
			
			// Update id using the one stored in the grid
			vertices_new_i[i] = grid[cell_key].id;
		}

		// If triangle has unique ids output it
		if (vertices_new_i[0] != vertices_new_i[1] && vertices_new_i[1] != vertices_new_i[2] && vertices_new_i[2] != vertices_new_i[0]) {
			mesh_simplified->addTriangle(vertices_new_i[0], vertices_new_i[1], vertices_new_i[2]);
		}
	}

	return mesh_simplified;
}

void Scene::optimizeLODs(const glm::vec3& cameraPos) {
    float targetFPS = 60.0f;
    float machineTPS = 150000000.0f; // Empirical value
    int maxCost = static_cast<int>(machineTPS / targetFPS);
    int currentCost = 0;

    // Calculate static cost
    int staticCost = 0;
    for (auto obj : objects) {
        if (!obj->getIsLODEnabled()) {
            staticCost += obj->getMesh()->getTriangleCount();
        }
    }
    maxCost -= staticCost;
	
	std::vector<LODUpgrade> possibleUpgrades;
    possibleUpgrades.reserve(objects.size() * (LOD_LEVELS - 1)); // Memory optimization

    // Start with lowest LOD for all objects
	// Determine total cost of those objects
    for (auto obj : objects) {
        if (!obj->getIsLODEnabled()) continue;
        obj->setCurrentLOD(LOD_LEVELS - 1);
        currentCost += obj->getMesh()->getTriangleCount();
		
		float D = glm::max(glm::distance(glm::vec3(obj->getTransform()[3]), cameraPos), 0.001f);
        float d = obj->getBoundingBoxDiagonal();

		for (int index_curr = LOD_LEVELS - 1; index_curr > 0; --index_curr) {
			int index_next = index_curr - 1;
            
			int LOD_curr = LOD_LEVELS - index_curr;
    		int LOD_next = LOD_LEVELS - index_next;
			
			float benefit_curr = 1.0f - (d / (static_cast<float>(1 << LOD_curr) * D));
            float benefit_next = 1.0f - (d / (static_cast<float>(1 << LOD_next) * D));

            float deltaBenefit = benefit_next - benefit_curr;
            int deltaCost = obj->getTriangleCount(index_next) - obj->getTriangleCount(index_curr);

            if (deltaCost > 0) {
                float ratio = deltaBenefit / static_cast<float>(deltaCost);
                possibleUpgrades.push_back({obj, index_next, deltaCost, ratio});
            }
        }
    }

	std::sort(possibleUpgrades.begin(), possibleUpgrades.end());

	// Find object with largest ∆benefit / ∆cost
    for (const auto& upgrade : possibleUpgrades) {
		if (currentCost + upgrade.cost <= maxCost) {
			// Increase LOD for that object
			// Update total cost
            if (upgrade.obj->getCurrentLOD() == upgrade.targetLOD + 1) {
                upgrade.obj->setCurrentLOD(upgrade.targetLOD);
                currentCost += upgrade.cost;
            }
        } else {
            break; 
        }
    }
}

void Scene::savePLYBinary(const string& filename, const TriangleMesh* mesh) const
{
	ofstream fout(filename, ios::out | ios::binary);
	if (!fout.is_open()) {
		cout << "Error: Could not open file for writing: " << filename << endl;
		return;
	}

	const vector<glm::vec3> &vertices = mesh->get_vertices();
	const vector<glm::vec3> &colors = mesh->get_colors();
	const vector<int> &triangles = mesh->get_triangles();
	
	int numVertices = vertices.size();
	int numFaces = triangles.size() / 3;

	// Header
	fout << "ply\n";
	fout << "format binary_little_endian 1.0\n";
	fout << "element vertex " << numVertices << "\n";
	fout << "property float x\n";
	fout << "property float y\n";
	fout << "property float z\n";
	fout << "property uchar red\n";
	fout << "property uchar green\n";
	fout << "property uchar blue\n";
	fout << "element face " << numFaces << "\n";
	fout << "property list uchar int vertex_indices\n";
	fout << "end_header\n";

	// Vertices and Colors
	for (size_t i = 0; i < vertices.size(); ++i) {
		fout.write(reinterpret_cast<const char*>(&vertices[i].x), sizeof(float));
		fout.write(reinterpret_cast<const char*>(&vertices[i].y), sizeof(float));
		fout.write(reinterpret_cast<const char*>(&vertices[i].z), sizeof(float));

		unsigned char r = static_cast<unsigned char>(glm::clamp(colors[i].r, 0.0f, 1.0f) * 255.0f);
		unsigned char g = static_cast<unsigned char>(glm::clamp(colors[i].g, 0.0f, 1.0f) * 255.0f);
		unsigned char b = static_cast<unsigned char>(glm::clamp(colors[i].b, 0.0f, 1.0f) * 255.0f);
		
		fout.write(reinterpret_cast<const char*>(&r), sizeof(unsigned char));
		fout.write(reinterpret_cast<const char*>(&g), sizeof(unsigned char));
		fout.write(reinterpret_cast<const char*>(&b), sizeof(unsigned char));
	}

	// Triangles
	unsigned char vertsPerFace = 3;
	for (size_t i = 0; i < triangles.size(); i += 3) {
		fout.write(reinterpret_cast<const char*>(&vertsPerFace), sizeof(unsigned char));
		fout.write(reinterpret_cast<const char*>(&triangles[i]), sizeof(int));
		fout.write(reinterpret_cast<const char*>(&triangles[i + 1]), sizeof(int));
		fout.write(reinterpret_cast<const char*>(&triangles[i + 2]), sizeof(int));
	}

	fout.close();
}

void Scene::update(int deltaTime)
{
	currentTime += deltaTime;
	optimizeLODs(camera.getPosition());
}

// Render the scene. First the room, then the mesh it there is one loaded.

void Scene::render()
{
	Application::instance().getShader()->use();
	camera.render();
	for(vector<TriangleMeshInstance *>::iterator it=objects.begin(); it!=objects.end(); it++)
		(*it)->render(getShowLODColors());
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