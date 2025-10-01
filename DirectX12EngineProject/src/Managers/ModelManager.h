#pragma once

// Assimp
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <DirectXMath.h>

#include <string>
#include <vector>
#include <unordered_map>

struct Vertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 TexCoord;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT3 Bitangent;
};

class ModelManager
{
public:

	std::unordered_map<std::string, uint32_t> loadedModels; // key: fileName, value: meshID
	
	ModelManager() = default;
	~ModelManager() = default;

	static void LoadModel(const std::string& fileName, std::vector<Vertex>& outVertices,
		std::vector<uint32_t>& outIndices);

private:
	static void ProcessMesh(aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	static void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
};

