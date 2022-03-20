#include <gl/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <iomanip>
#include <numeric>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <k4a/k4a.hpp>

#include "ModelLoading.h"

using namespace std;
using namespace Eigen;
using namespace cv;

/*---------model----------*/
sen::Model model;
map<string, GLuint> textureIdMap;
map<string, Mat> textureMap;
vector<GLuint> textureIDs;
vector<string> texture_paths;
/*---------model----------*/

/*---------k4a----------*/
float fx = 604.4f;
float fy = 604.3f;
float cx = 639.5f;
float cy = 359.5f;
float z_near = 500.0f;
float z_far = 10'000.0f;
int frame_width = 1280;
int frame_height = 720;
float fovy = 60.0f;
const Matrix4f ProjectionMatrix =
Matrix4f((Matrix4f() <<
		  0.974279f, 0.0f, 0.0f, 0.0f,
		  0.0f, 1.73205f, 0.0f, 0.0f,
		  0.0f, 0.0f, -1.10526f, -1052.63f,
		  0.0f, 0.0f, -1.0f, 0.0f).finished());
const Matrix4f constant_rotation =
Matrix4f((Matrix4f() <<
		  1.0f, 0.0f, 0.0f, 0.0f,
		  0.0f, -1.0f, 0.0f, 0.0f,
		  0.0f, 0.0f, -1.0f, 0.0f,
		  0.0f, 0.0f, 0.0f, 1.0f).finished());
/*---------k4a----------*/

/*---------GL----------*/
GLuint LoadShaders(const string &vertex_file_path, const string &fragment_file_path)
{
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if (VertexShaderStream.is_open())
	{
		stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else
	{
		cout << "Impossible to open " << vertex_file_path << ". Are you in the right directory ? Don't forget to read the FAQ !" << endl;
		return 0;
	}

	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if (FragmentShaderStream.is_open())
	{
		stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	char const *VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		cout << &VertexShaderErrorMessage[0] << endl;
	}

	char const *FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		cout << &FragmentShaderErrorMessage[0] << endl;
	}

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		cout << &ProgramErrorMessage[0] << endl;
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
/*---------GL----------*/

int main(int argc, char **argv)
{
	/*-----init opengl environment-----*/
	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_VISIBLE, true);

	GLFWwindow *window = glfwCreateWindow(frame_width, frame_height, "Scanning Simulation", NULL, NULL);
	if (window == NULL)
	{
		glfwTerminate();
		throw runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(window);

	glewExperimental = true;
	if (glewInit() != GLEW_OK)
	{
		glfwTerminate();
		throw runtime_error("Failed to initialize GLEW");
	}

	GLuint programID = LoadShaders("TexturedTriangleMesh.vertexshader", "TexturedTriangleMesh.fragmentshader");

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_POLYGON_OFFSET_FILL);
	/*-----init opengl environment-----*/

	sen::Model model;
	model.Load("./output-models/zebra.obj", true);
	vector<sen::Model::Mesh> meshes = model.OutputData();

	/*-----load texture-----*/
	//	1. generate unique texture ids
	for (int i = 0; i < meshes.size(); ++i)
	{
		textureIdMap[meshes[i].texture_path] = 0;
		textureMap[meshes[i].texture_path] = Mat();
	}
	int numTextures = textureIdMap.size();
	textureIDs.resize(numTextures);
	glGenTextures(static_cast<GLsizei>(numTextures), &textureIDs[0]);
	auto iter_id = textureIdMap.begin();
	for (int i = 0; i < numTextures; ++i)
	{
		iter_id->second = textureIDs[i];
		iter_id++;
	}
	//	2. read texture img for every unique texture
	for (auto &it : textureMap)
	{
		it.second = cv::imread(it.first);
		cvtColor(it.second, it.second, COLOR_BGR2BGRA);
	}
	//	3. bind textures
	for (const auto &it : textureMap)
	{
		string texture_path = it.first;
		Mat texture_img = it.second;

		GLuint texture_id = textureIdMap[texture_path];

		glBindTexture(GL_TEXTURE_2D, texture_id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_img.size().width, texture_img.size().height, 0, GL_BGRA, GL_UNSIGNED_BYTE, texture_img.data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	/*-----load texture-----*/

	/*-----load it into VAO VBO EBO-----*/
	std::vector<GLuint> VAOs(meshes.size());
	std::vector<std::array<GLuint, 2>> VBOs(meshes.size());
	std::vector<GLsizei> draw_array_sizes;
	for (int i = 0; i < meshes.size(); ++i)
	{
		glGenVertexArrays(1, &VAOs[i]);
		glBindVertexArray(VAOs[i]);

		//-----load it into VBO-----
		glGenBuffers(1, &VBOs[i][0]);
		glBindBuffer(GL_ARRAY_BUFFER, VBOs[i][0]);
		glBufferData(GL_ARRAY_BUFFER, meshes[i].points.size() * sizeof(Vector3f), &meshes[i].points[0], GL_STATIC_DRAW);
		glGenBuffers(1, &VBOs[i][1]);
		glBindBuffer(GL_ARRAY_BUFFER, VBOs[i][1]);
		glBufferData(GL_ARRAY_BUFFER, meshes[i].uvs.size() * sizeof(Vector2f), &meshes[i].uvs[0], GL_STATIC_DRAW);
		//-----load it into VBO-----

		draw_array_sizes.push_back(GLsizei(meshes[i].indices.size()));
		texture_paths.push_back(meshes[i].texture_path);
	}
	/*-----load it into VAO VBO EBO-----*/

	float scanning_circle_radius = 800.0f;
	float step_angle = 15.0f;

	// XZ Plane
	// x^2 + z^2 = R^2
	float y = 0.0f;
	vector<tuple<k4a::image, k4a::image, Affine3f>> XZ_data;
	for (float angle = 0.0f; angle < 360.0f; angle += step_angle)
	{
		/*-----View Matrix-----*/
		// 1. compute camera extrinsic
		float rad = angle * M_PI / 180.0f;

		Affine3f inplace_rotation;
		inplace_rotation = AngleAxisf(rad, Vector3f::UnitY());

		float x = scanning_circle_radius * std::sin(rad);
		float z = scanning_circle_radius * std::cos(rad);

		Affine3f camera_trans;
		camera_trans = Translation3f(x, y, z) * inplace_rotation;

		Vector3f camera_position = camera_trans.matrix().block<3, 1>(0, 3);
		Matrix3f camera_rotation = camera_trans.matrix().block<3, 3>(0, 0);

		// 2. extrinsic => local coordinate system
		Vector3f x_axis = camera_rotation * Vector3f::UnitX();
		Vector3f y_axis = camera_rotation * Vector3f::UnitY();
		Vector3f z_axis = camera_rotation * Vector3f::UnitZ();
		x_axis.normalize();
		y_axis.normalize();
		z_axis.normalize();

		// 3. local coordinate system => eye up target
		Vector3f eye = camera_position;
		Vector3f up = y_axis;
		Vector3f target = eye - z_axis;

		// 4. eye up target => view matrix
		Vector3f f = (target - eye).normalized();
		Vector3f u = up.normalized();
		Vector3f s = f.cross(u).normalized();

		u = s.cross(f);
		Matrix4f ViewMatrix = Matrix4f::Zero();
		ViewMatrix(0, 0) = s.x();	ViewMatrix(0, 1) = s.y();	ViewMatrix(0, 2) = s.z();	ViewMatrix(0, 3) = -s.dot(eye);
		ViewMatrix(1, 0) = u.x();	ViewMatrix(1, 1) = u.y();	ViewMatrix(1, 2) = u.z();	ViewMatrix(1, 3) = -u.dot(eye);
		ViewMatrix(2, 0) = -f.x();  ViewMatrix(2, 1) = -f.y();  ViewMatrix(2, 2) = -f.z();  ViewMatrix(2, 3) = f.dot(eye);
		ViewMatrix.row(3) << 0, 0, 0, 1;
		/*-----View Matrix-----*/

		/*-----Model Matrix-----*/
		Affine3f model_trans;
		model_trans = Affine3f::Identity();
		Matrix4f ModelMatrix = model_trans.matrix();
		/*-----Model Matrix-----*/

		/*-----MVP-----*/
		Matrix4f MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		Matrix4f VM = ViewMatrix * ModelMatrix;
		/*-----MVP-----*/

		/*-----Renderering-----*/
		glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
		glClearDepth(1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(programID);

		glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, MVP.data());

		for (int i = 0; i < draw_array_sizes.size(); ++i)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, textureIdMap[texture_paths[i]]);
			glUniform1i(glGetUniformLocation(programID, "texture_diffuse"), 0);

			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, VBOs[i][0]);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ARRAY_BUFFER, VBOs[i][1]);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

			glDrawArrays(GL_TRIANGLES, 0, draw_array_sizes[i]);

			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
		}
		/*-----Renderering-----*/

		double depth_scale = 1.0;

		k4a::image colorImage = k4a::image::create(K4A_IMAGE_FORMAT_COLOR_BGRA32, frame_width, frame_height, frame_width * 4 * (int)sizeof(uint8_t));
		k4a::image depthImage = k4a::image::create(K4A_IMAGE_FORMAT_DEPTH16, frame_width, frame_height, frame_width * (int)sizeof(uint16_t));

		Affine3f cloud_trans;
		cloud_trans.setIdentity();
		Matrix4f Xcv_to_Xworld = VM.inverse() * constant_rotation;
		cloud_trans.matrix() = Xcv_to_Xworld;

		vector<uint8_t> color_frame_buffer(frame_width * frame_height * 4);
		vector<float> depth_frame_buffer(frame_width * frame_height);

		glReadPixels(0, 0, frame_width, frame_height, GL_BGRA, GL_UNSIGNED_BYTE, color_frame_buffer.data());
		for (int i = 0; i < frame_height; i++)
		{
			copy(color_frame_buffer.begin() + frame_width * 4 * i, color_frame_buffer.begin() + frame_width * 4 * (i + 1), colorImage.get_buffer() + frame_width * 4 * (frame_height - 1 - i));
		}

		glReadPixels(0, 0, frame_width, frame_height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_frame_buffer.data());

		vector<float> depth_frame_buffer_flipped(frame_width * frame_height);
		for (int i = 0; i < frame_height; i++)
		{
			copy(depth_frame_buffer.begin() + frame_width * i, depth_frame_buffer.begin() + frame_width * (i + 1), depth_frame_buffer_flipped.begin() + frame_width * (frame_height - 1 - i));
		}

		uint16_t *depth_buffer = reinterpret_cast<uint16_t *>(depthImage.get_buffer());
		for (int i = 0; i < frame_height; i++)
		{
			for (int j = 0; j < frame_width; j++)
			{
				float depth_value_gl = depth_frame_buffer_flipped[frame_width * i + j];

				if (depth_value_gl == 1.0f) continue;

				float z_depth = 2.0f * z_near * z_far / ((2.0f * depth_value_gl - 1.0f) * (z_far - z_near) - (z_far + z_near));
				z_depth = -z_depth;

				depth_buffer[frame_width * i + j] = (uint16_t)min(round(depth_scale * z_depth), (double)UINT16_MAX);
			}
		}

		glfwSwapBuffers(window);

		XZ_data.push_back({ colorImage, depthImage, cloud_trans });
	}
	cout << "CAPTURE PROCESS OVER!" << endl;

	/*-----Release-----*/
	for (int i = 0; i < draw_array_sizes.size(); ++i)
	{
		glDeleteBuffers(1, &VBOs[i][0]);
		glDeleteBuffers(1, &VBOs[i][1]);
		glDeleteVertexArrays(1, &VAOs[i]);
	}
	for (int i = 0; i < textureIDs.size(); ++i)
	{
		glDeleteTextures(1, &textureIDs[i]);
	}

	VAOs.clear();
	VBOs.clear();
	textureIDs.clear();
	draw_array_sizes.clear();
	texture_paths.clear();

	glDeleteProgram(programID);

	glfwTerminate();
	glfwDestroyWindow(window);
	/*-----Release-----*/
	return 0;
}