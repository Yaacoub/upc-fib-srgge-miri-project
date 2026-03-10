#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include "PLYReader.h"


// Reads the mesh from the PLY file, first the header, then the vertex data, 
// and finally the face data. Then it rescales the model so that it fits a
// box of size 1x1x1 centered at the origin

bool PLYReader::readMesh(const string &filename, TriangleMesh &mesh)
{
	ifstream fin;
	int nVertices, nFaces;
	bool hasColors = false;
	bool hasAlpha = false;

	fin.open(filename.c_str(), ios_base::in | ios_base::binary);
	if(!fin.is_open())
		return false;
	if(!loadHeader(fin, nVertices, nFaces, hasColors, hasAlpha))
	{
		fin.close();
		return false;
	}

	vector<float> plyVertices;
	vector<float> plyColors;
	vector<int> plyTriangles;

	loadVertices(fin, nVertices, plyVertices, plyColors, hasColors, hasAlpha);
	loadFaces(fin, nFaces, plyTriangles);
	fin.close();

	rescaleModel(plyVertices);
	addModelToMesh(plyVertices, plyColors, plyTriangles, mesh);

	return true;
}

// Reads the header of a PLY file.
// It first checks that the file is really a PLY. 
// Then it reads lines until it finds the 'end_header'
// The 'element vertex' and 'element face' lines contain the number 
// of primitives in the file.

bool PLYReader::loadHeader(ifstream &fin, int &nVertices, int &nFaces, bool &hasColors, bool &hasAlpha)
{
	string line;
	bool parsingVertex = false;

	nVertices = 0;
	nFaces = 0;
	hasColors = false;
	hasAlpha = false;

	if(!getline(fin, line) || line.compare(0, 3, "ply") != 0)
		return false;
	while(getline(fin, line))
	{
		if(line.compare(0, 10, "end_header") == 0)
			break;
		if(line.compare(0, 14, "element vertex") == 0)
		{
			nVertices = atoi(&line[15]);
			parsingVertex = true;
			continue;
		}
		if(line.compare(0, 12, "element face") == 0)
		{
			nFaces = atoi(&line[13]);
			parsingVertex = false;
			continue;
		}
		if(parsingVertex && line.compare(0, 8, "property") == 0)
		{
			if(line.find("red") != string::npos || line.find("green") != string::npos || line.find("blue") != string::npos)
				hasColors = true;
			if(line.find("alpha") != string::npos)
				hasAlpha = true;
		}
	}
	if(nVertices <= 0)
		return false;
	cout << "Loading triangle mesh" << endl;
	cout << "\tVertices = " << nVertices << endl;
	cout << "\tFaces = " << nFaces << endl;
	cout << "\tColors = " << (hasColors ? "yes" : "no") << endl;
	cout << endl;

	return true;
}

// Loads the vertices' coordinates into a vector

void PLYReader::loadVertices(ifstream &fin, int nVertices, vector<float> &plyVertices, vector<float> &plyColors, bool hasColors, bool hasAlpha)
{
	int i;
	float value;
	unsigned char color;

	plyVertices.resize(3*nVertices);
	plyColors.resize(3*nVertices, 1.f);
	for(i=0; i<nVertices; i++)
	{
		fin.read((char *)&value, sizeof(float));
		plyVertices[3*i] = value;
		fin.read((char *)&value, sizeof(float));
		plyVertices[3*i+1] = value;
		fin.read((char *)&value, sizeof(float));
		plyVertices[3*i+2] = value;
		if(hasColors)
		{
			fin.read((char *)&color, sizeof(unsigned char));
			plyColors[3*i] = color / 255.f;
			fin.read((char *)&color, sizeof(unsigned char));
			plyColors[3*i+1] = color / 255.f;
			fin.read((char *)&color, sizeof(unsigned char));
			plyColors[3*i+2] = color / 255.f;
			if(hasAlpha)
				fin.read((char *)&color, sizeof(unsigned char)); // Discard alpha if present
		}
	}
}

// Same thing for the faces. Those with more than three sides
// are subdivided into triangles.

void PLYReader::loadFaces(ifstream &fin, int nFaces, vector<int> &plyTriangles)
{
	int i, tri[3];
	unsigned char nVrtxPerFace;

	plyTriangles.reserve(nFaces * 3);
	for(i=0; i<nFaces; i++)
	{
		fin.read((char *)&nVrtxPerFace, sizeof(unsigned char));
		fin.read((char *)&tri[0], sizeof(int));
		fin.read((char *)&tri[1], sizeof(int));
		fin.read((char *)&tri[2], sizeof(int));
		plyTriangles.push_back(tri[0]);
		plyTriangles.push_back(tri[1]);
		plyTriangles.push_back(tri[2]);
		for(; nVrtxPerFace>3; nVrtxPerFace--)
		{
			tri[1] = tri[2];
			fin.read((char *)&tri[2], sizeof(int));
			plyTriangles.push_back(tri[0]);
			plyTriangles.push_back(tri[1]);
			plyTriangles.push_back(tri[2]);
		}
	}
}

// Rescales the model to fit a box of 1x1x1 with its base centered at the origin

void PLYReader::rescaleModel(vector<float> &plyVertices)
{
	unsigned int i;
	glm::vec3 baseCenter, size[2];

	size[0] = glm::vec3(1e10, 1e10, 1e10);
	size[1] = glm::vec3(-1e10, -1e10, -1e10);
	for(i=0; i<plyVertices.size(); i+=3)
	{
		size[0][0] = glm::min(size[0][0], plyVertices[i]);
		size[0][1] = glm::min(size[0][1], plyVertices[i+1]);
		size[0][2] = glm::min(size[0][2], plyVertices[i+2]);
		size[1][0] = glm::max(size[1][0], plyVertices[i]);
		size[1][1] = glm::max(size[1][1], plyVertices[i+1]);
		size[1][2] = glm::max(size[1][2], plyVertices[i+2]);
	}
	baseCenter = glm::vec3((size[0][0] + size[1][0]) / 2.f, size[0][1], (size[0][2] + size[1][2]) / 2.f);

	float largestSize = glm::max(size[1][0] - size[0][0], glm::max(size[1][1] - size[0][1], size[1][2] - size[0][2]));

	for(i=0; i<plyVertices.size(); i+=3)
	{
		plyVertices[i] = (plyVertices[i] - baseCenter[0]) / largestSize;
		plyVertices[i+1] = (plyVertices[i+1] - baseCenter[1]) / largestSize;
		plyVertices[i+2] = (plyVertices[i+2] - baseCenter[2]) / largestSize;
	}
}

// Vertex and face data are added to the model using this function

void PLYReader::addModelToMesh(const vector<float> &plyVertices, const vector<float> &plyColors, const vector<int> &plyTriangles, TriangleMesh &mesh)
{
	mesh.initVertices(plyVertices, plyColors);
	mesh.initTriangles(plyTriangles);
}
