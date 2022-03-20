#pragma once

#include <string>
#include <map>
#include <vector>

#include <assimp/material.h>
#include <assimp/scene.h>

#include <Eigen/Dense>

namespace sen
{
	class Model
	{
	public:
		struct Mesh
		{
			std::vector<Eigen::Vector3f> points;
			std::vector<Eigen::Vector2f> uvs;
			std::vector<unsigned short> indices;
			std::string texture_path;
		};

	public:
		Model();
		~Model();

		void Load(const std::string &model_path, const bool &flip_uvs);

		std::vector<Mesh> OutputData() const;

	private:
		std::string folder_path;
		aiScene *scene_;

		std::vector<Mesh> processNode(const aiNode *node) const;
		Mesh readAiMesh(const aiMesh *ai_mesh) const;
	};
}