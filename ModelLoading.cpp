#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cexport.h>
#include <assimp/Exporter.hpp>

#include "ModelLoading.h"

using namespace std;
using namespace Eigen;
using namespace Assimp;

namespace sen
{
	Model::Model()
	{
		scene_ = nullptr;
	}

	Model::~Model()
	{
		aiFreeScene(this->scene_);
	}

	void Model::Load(const std::string &model_path, const bool &flip_uvs)
	{
		Importer importer;
		const aiScene *scene;
		if (flip_uvs)
		{
			scene = importer.ReadFile(model_path, aiProcess_FlipUVs | aiProcess_GenBoundingBoxes);
		}
		else
		{
			scene = importer.ReadFile(model_path, aiProcess_GenBoundingBoxes);
		}

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}

		this->folder_path = model_path.substr(0, model_path.find_last_of('/'));

		aiCopyScene(scene, &this->scene_);
	}

	vector<Model::Mesh> Model::OutputData() const
	{
		vector<Mesh> meshes = processNode(this->scene_->mRootNode);

		return meshes;
	}

	vector<Model::Mesh> Model::processNode(const aiNode *node) const
	{
		vector<Model::Mesh> meshes;
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh *ai_mesh = this->scene_->mMeshes[node->mMeshes[i]];
			Mesh mesh = readAiMesh(ai_mesh);
			meshes.push_back(mesh);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			vector<Model::Mesh> meshes_node;
			meshes_node = processNode(node->mChildren[i]);
			meshes.insert(meshes.end(), meshes_node.begin(), meshes_node.end());
		}

		return meshes;
	}
	Model::Mesh Model::readAiMesh(const aiMesh *ai_mesh) const
	{
		Mesh mesh;

		for (int i = 0; i < static_cast<int>(ai_mesh->mNumVertices); ++i)
		{
			mesh.points.push_back(Vector3f(ai_mesh->mVertices[i].x, ai_mesh->mVertices[i].y, ai_mesh->mVertices[i].z));
			mesh.uvs.push_back(Vector2f(ai_mesh->mTextureCoords[0][i].x, ai_mesh->mTextureCoords[0][i].y));
		}

		for (unsigned int i = 0; i < ai_mesh->mNumFaces; i++)
		{
			mesh.indices.push_back(ai_mesh->mFaces[i].mIndices[0]);
			mesh.indices.push_back(ai_mesh->mFaces[i].mIndices[1]);
			mesh.indices.push_back(ai_mesh->mFaces[i].mIndices[2]);
		}

		aiMaterial *material = this->scene_->mMaterials[ai_mesh->mMaterialIndex];
		aiString str;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &str);

		mesh.texture_path = folder_path + "/" + str.C_Str();

		return mesh;
	}
}