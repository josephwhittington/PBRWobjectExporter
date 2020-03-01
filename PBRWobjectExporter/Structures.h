#pragma once

#include <vector>

namespace WOBJ
{
	struct WFLOAT2 { float x, y; };
	struct WFLOAT3 { float x, y, z; };
	struct WFLOAT4 { float x, y, z, w; };

	struct 
	{
		std::string BEGIN = "#BEGIN";
		std::string END = "#END";
		std::string ALBEDO = "#diffuse";
		std::string METALLIC = "#metallic";
		std::string ROUGHNESS = "#roughness";
		std::string NORMAL = "#normal";
		std::string AMBIENT_OCCLUSION = "#ao";
	} WMAT_PROPS;

	struct Header
	{
		int indexcount, vertexcount;
		int indexstart, vertexstart;
		char t_albedo[256];
		char t_normal[256];
		char t_metallic[256];
		char t_roughness[256];
		char t_ambient_occlusion[256];
	};

	struct WVertex
	{
		WFLOAT3 position;
		WFLOAT3 tangent;
		WFLOAT3 bitangent;
		WFLOAT3 normal;
		WFLOAT2 uv_diffuse;
	};

	struct WOBJMesh
	{
		std::vector<WVertex> vertices;
		std::vector<int> indices;
	};
}