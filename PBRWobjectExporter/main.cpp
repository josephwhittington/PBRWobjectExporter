#include <iostream>
#include <fstream>
#include <string>

#include "Structures.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

void LoadModel(const std::string& modelname);
void LoadNode(aiNode* node, const aiScene* scene);
void LoadMesh(aiMesh* mesh, const aiScene* scene);
void LoadMaterials(const std::string& materialfilename);
void ExportWobject(std::string filename);

// Globals
WOBJ::WOBJMesh g_mesh;
WOBJ::Header g_header;

#define WOBJ_VERSION 2.0

int main(int argc, char** argv)
{
	std::cout << "WOBJ Exporter v2.0 (PBR Patch)" << std::endl;

	// Validate the input parameters
	if (argc < 2)
	{
		std::cout << "Need to specify an FBX file" << std::endl;
		return EXIT_SUCCESS;
	}

	std::string filename, materialname, outputname;
	if (argc == 2)
	{
		// No material file specified
		filename = std::string(argv[1]);
		materialname = filename.substr(0, filename.find_first_of(".fbx") + 2).append(".wmf");
	}
	else if (argc == 3)
	{
		filename = std::string(argv[1]);
		materialname = std::string(argv[2]);
	}

	// Do the wobj conversion
	outputname = filename.substr(0, filename.find_first_of(".fbx") + 2).append(".wobj");

	// load material and mesh
	LoadModel(materialname);
	LoadMaterials(materialname);
	ExportWobject(outputname);

	return EXIT_SUCCESS;
}

void ExportWobject(std::string filename)
{
	// Generate the header
	g_header.indexcount = g_mesh.indices.size();
	g_header.vertexcount = g_mesh.vertices.size();
	g_header.indexstart = sizeof(WOBJ::Header);
	g_header.vertexstart = g_header.indexstart + (g_header.indexcount * 4);

	// Start writing shit to the file
	std::ofstream file(filename, std::ios::trunc | std::ios::binary | std::ios::out);

	file.write((const char*)&g_header, sizeof(WOBJ::Header));
	file.write((const char*)g_mesh.indices.data(), 4 * g_mesh.indices.size());
	file.write((const char*)g_mesh.vertices.data(), sizeof(WOBJ::WVertex) * g_mesh.vertices.size());

	file.close();

	double file_size_in_bytes = float(sizeof(WOBJ::Header) + (sizeof(WOBJ::WVertex) * g_mesh.vertices.size()) + (sizeof(g_mesh.indices.size() * 4)));

	// Output data
	std::cout << "Converted File: " << filename.substr(0, filename.find(".wobj")) << ".fbx" << std::endl;
	std::cout << "Output File: " << filename << std::endl;
	std::cout << "File size: " << double(file_size_in_bytes) / (double)1024 << " kb" << std::endl;
	std::cout << "Format: " << "Wobj Version " << WOBJ_VERSION << std::endl;
	std::cout << "Vertex Count: " << g_mesh.vertices.size() << std::endl;
	std::cout << "Index Count: " << g_mesh.indices.size() << std::endl;
	std::cout
		<< "Data Format:\n"
		<< "Position (Float3)\n"
		<< "Normal (Float3)\n"
		<< "Tangent (Float3)\n"
		<< "Bitangent (Float3)\n"
		<< "uv_diffuse (Float2)\n";
}

void LoadModel(const std::string& modelname)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(modelname,
		aiProcess_Triangulate | aiProcess_FlipUVs |
		aiProcess_GenSmoothNormals | aiProcess_GenUVCoords |
		aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);

	if (!scene)
	{
		std::cout << "Model (%s) failed to load: " << modelname << importer.GetErrorString() << std::endl;
		return;
	}

	LoadNode(scene->mRootNode, scene);
}

void LoadNode(aiNode* node, const aiScene* scene)
{
	for (int i = 0; i < node->mNumMeshes; i++)
	{
		LoadMesh(scene->mMeshes[node->mMeshes[i]], scene);
	}

	for (int i = 0; i < node->mNumChildren; i++)
	{
		LoadNode(node->mChildren[i], scene);
	}
}

void LoadMesh(aiMesh* mesh, const aiScene* scene)
{
	// Reusable
	WOBJ::WVertex vert;
	// Loop over the vertices
	for (int i = 0; i < mesh->mNumVertices; i++)
	{
		// Make this shit a macro, I'm not about to type this shit ever again
		vert.position = WOBJ::WFLOAT3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		vert.normal = WOBJ::WFLOAT3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals->z };
		vert.uv_diffuse = WOBJ::WFLOAT2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
		vert.tangent = WOBJ::WFLOAT3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
		vert.bitangent = WOBJ::WFLOAT3{ mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };

		g_mesh.vertices.push_back(vert);
	}

	// Loop over the faces, insert indices
	for (int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; j++)
		{
			g_mesh.indices.push_back(face.mIndices[j]);
		}
	}
}

void LoadMaterials(const std::string& materialfilename)
{
	std::ifstream material(materialfilename);
	std::string line;	

	bool found_start = false;

	// Default headers
	g_header.t_albedo[0] = '`';
	g_header.t_normal[0] = '`';
	g_header.t_metallic[0] = '`';
	g_header.t_roughness[0] = '`';
	g_header.t_ambient_occlusion[0] = '`';

	// This little loop is hacky AF, but the file is too small to justify writing real code for this TBH
	while (std::getline(material, line))
	{
		if (!found_start && line.find(WOBJ::WMAT_PROPS.BEGIN) == std::string::npos)
			continue;
		else if (!found_start && line.find(WOBJ::WMAT_PROPS.BEGIN) != std::string::npos)
		{
			found_start = true;
			continue;
		}

		if (found_start && line.find(WOBJ::WMAT_PROPS.END) != std::string::npos) break;

		if (line.find(WOBJ::WMAT_PROPS.ALBEDO) != std::string::npos)
		{
			std::getline(material, line);
			strcpy_s(g_header.t_albedo, line.c_str());
		}
		else if (line.find(WOBJ::WMAT_PROPS.NORMAL) != std::string::npos)
		{
			std::getline(material, line);
			strcpy_s(g_header.t_normal, line.c_str());
		}
		else if (line.find(WOBJ::WMAT_PROPS.METALLIC) != std::string::npos)
		{
			std::getline(material, line);
			strcpy_s(g_header.t_metallic, line.c_str());
		}
		else if (line.find(WOBJ::WMAT_PROPS.ROUGHNESS) != std::string::npos)
		{
			std::getline(material, line);
			strcpy_s(g_header.t_roughness, line.c_str());
		}
		else if (line.find(WOBJ::WMAT_PROPS.AMBIENT_OCCLUSION) != std::string::npos)
		{
			std::getline(material, line);
			strcpy_s(g_header.t_ambient_occlusion, line.c_str());
		}
	}
}