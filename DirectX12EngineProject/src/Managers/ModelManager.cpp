#include "ModelManager.h"

#include "../Utils/Utils.h"
#include <filesystem>


void ModelManager::LoadModel(const std::string& fileName, std::vector<Vertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
    std::filesystem::path filePath = std::filesystem::current_path() / "\\Assets\\Models\\" / fileName;

    // Create an instance of the Importer class
    Assimp::Importer importer;
    
    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile(filePath.string(),
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_ConvertToLeftHanded |
        aiProcess_GenNormals);

    // If the import failed, report it
    if (nullptr == scene || !scene->mRootNode) {
        Debug::Print(importer.GetErrorString());
        return;
    }

	outVertices.clear();
	outIndices.clear();

    // Now we can access the file's contents.
	ProcessNode(scene->mRootNode, scene, outVertices, outIndices);

    // We're done. Everything will be cleaned up by the importer destructor
    return;
}

// 노드를 재귀적으로 돌면서 모든 mesh 처리
void ModelManager::ProcessNode(aiNode* node, const aiScene* scene,
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices)
{
    // 이 노드에 속한 모든 mesh 처리
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, vertices, indices);
    }

    // 자식 노드 재귀
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, vertices, indices);
    }
}

// mesh 하나를 읽어서 vertices/indices에 push_back
void ModelManager::ProcessMesh(aiMesh* mesh,
    std::vector<Vertex>& vertices,
    std::vector<uint32_t>& indices)
{
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex v{};
        v.Position = { mesh->mVertices[i].x,
                       mesh->mVertices[i].y,
                       mesh->mVertices[i].z };
        if (mesh->HasNormals())
        {
            v.Normal = { mesh->mNormals[i].x,
                         mesh->mNormals[i].y,
                         mesh->mNormals[i].z };
        }
        vertices.push_back(v);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }
}