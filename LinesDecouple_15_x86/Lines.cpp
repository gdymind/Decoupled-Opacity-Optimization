#ifndef LINES_H
#define LINES_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Include/Shader.hpp"

#include "commonVars.h"

using namespace std;

const unsigned int RESTART_NUM = 0x5FFFFFu;//primitive restart number

struct Vertex
{
	glm::vec3 Position_;
	GLuint LineId_;
	GLfloat Weight_;
	Vertex(GLfloat x, GLfloat y, GLfloat z): Position_(glm::vec3(x, y, z)) {}
};

typedef vector<Vertex> LINE_TYPE;//a set of points
typedef vector<unsigned int> INDEX_TYPE;//a set of indexes

class Lines
{
public:
	int segmentNum_ = 0;
	int vertexNum_ = 0;

	vector<LINE_TYPE> lines_;
	vector<float> lineLengths_;
	vector<int> lineSegNums_;
	vector<int> segLineIds_;

	GLuint VAO, VBO;//vertex array object, vertex buffer object
	GLuint ABO;//atomic buffer object

	Lines(const std::string &path, int ctrlPntsNum);
private:
	void loadModel(const string &path);
	void setupModel();
};

Lines::Lines(const std::string &path, int segNum):
	segmentNum_(segNum)
{
	loadModel(path);
	setupModel();
}

void Lines::loadModel(const string &path)
{
#pragma region read vertices and lines
	vector<Vertex> vertices;
	ifstream fileIn(path);
	string lineBuf;
	while (getline(fileIn, lineBuf))
	{
		stringstream ss(lineBuf);
		string type;//v vt g l
		getline(ss, type, ' ');

		if (type == "v")//a vertex
		{
			float x[3];
			ss >> x[0] >> x[1] >> x[2];
			vertices.push_back(Vertex(x[0], x[1], x[2]));
		}
		else if (type == "l")//a line
		{
			LINE_TYPE line;
			int lineId = lines_.size();

			int curId;
			int lastId;

			ss >> lastId;
			vertices[lastId].LineId_ = lineId;
			line.push_back(vertices[lastId]);

			float len;
			while (ss >> curId)
			{
				ss >> curId;
				len = glm::length(vertices[lastId].Position_ - vertices[curId].Position_);
				if (len > EPS)
				{
					line.push_back(vertices[curId]);
					lastId = curId;
				}
			}

			lines_.push_back(line);
		}

	}
	fileIn.close();
#pragma endregion

#pragma region compute vertices number
	vertexNum_ = 0;
	for (auto itLines = lines_.begin(); itLines != lines_.end(); ++itLines)
	{
		vertexNum_ += itLines->size();
	}
#pragma endregion

#pragma region compute line lengths, total length and points number
	float totalLength = 0.0f;
	int totalPointsNum = 0;
	lineLengths_.resize(lines_.size(), 0.0f);
	for (int i = 0; i < (int)lines_.size(); ++i)
	{
		LINE_TYPE &line = lines_[i];
		float &len = lineLengths_[i];
		len = 0.0f;
		for (int j = 1; i < (int)line.size(); ++j)
			len += glm::length(line[j].Position - line[j - 1].Position);
		totalLength += len;
		totalPointsNum += line.size();
	}
#pragma endregion

#pragma region distribute segments with approximately equal lengths
	assert(segmentNum_ * 2 > (int)lines_.size());//because a line is at least distributed into two segments
	float avgLength = totalLength / totalPointsNum;

	float leftLength = totalLength;
	float leftSegmentNum = totalPointsNum;

	//2 segments for those lines whose lengths are < avg length
	set<int> lineIds;
	for (int i = 0; i < (int)lines_.size(); ++i) lineIds.insert(i);
	for (int i = 0; i < (int)lines_.size(); ++i)
	{
		float &len = lineLengths_[i];
		if (len < avgLength)
		{
			lineIds.erase(i);
			lineSegNums_[i] = 2;
			leftSegmentNum -= 2;
			leftLength -= len;
		}
	}

	//now distribute the remaining lengths
	set< pair<float, int> > lineLeftLengths;
	if (leftSegmentNum > 0)
	{
		for (auto it = lineIds.begin(); it != lineIds.end(); ++it)
		{
			int i = *it;

			float &len = lineLengths_[i];
			float segNum = len / leftLength * leftSegmentNum;

			lineSegNums_[i] = (int)segNum;
			lineLeftLengths.insert(make_pair(segNum - (float)segNum, i));
		}
	}

	while (leftSegmentNum > 0)
	{
		auto itMax = lineLeftLengths.rbegin();
		int i = itMax->second;
		++lineSegNums_[i];
		--leftSegmentNum;
		lineLeftLengths.erase(*itMax);
	}
#pragma endregion

#pragma region assign blending weights
	int segOffset = 0;
	for (int i = 0; i < (int)lines_.size(); ++i)
	{
		LINE_TYPE &line = lines_[i];
		
		int segNum = lineSegNums_[i];
		float curLength = 0.0f;
		float lineLength = lineLengths_[i];

		line[0].Weight_ = (float)segOffset + EPS;

		for (int j = 1; j < (int)line.size(); ++j)
		{
			glm::vec3 &a = line[j - 1].Position_;
			glm::vec3 &b = line[j].Position_;
			
			float l = glm::length(b - a);
			curLength += l;

			line[j].Weight_ = segOffset + std::min(curLength / lineLength * (segNum - 1) + EPS, segNum - 1 - EPS);
		}
		segOffset += segNum;
	}
#pragma endregion

#pragma region compute segs' line indices
	{
		int segId = 0;
		for (int i = 0; i < (int)lines_.size(); ++i)
		{
			LINE_TYPE &line = lines_[i];
			for (int j = 0; j < (int)line.size(); ++j)
			{
				segLineIds_[segId++] = i;
			}
		}
	}
#pragma endregion

}

void Lines::setupModel()
{
	glGenBuffers(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &ABO);

	//bind VAO
	glBindVertexArray(VAO);

	//set VBO
	//glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glNamedBufferStorage(VBO, vertexNum_ * sizeof(Vertex), nullptr, GL_DYNAMIC_STORAGE_BIT);
	//prepare vertex data
	{
		int offset = 0;
		for (auto it = lines_.begin(); it != lines_.end(); ++it)
		{
			LINE_TYPE &line = *it;
			int sz = line.size();
			glNamedBufferSubData(VBO, offset, sz * sizeof(Vertex), &line[0]);
			offset += sz;
		}
	}
	//vertex Positon
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	//vertex LineId
	glVertexAttribPointer(1, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, LineId_));
	glEnableVertexAttribArray(1);
	//vertex Weight
	glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Weight_));
	glEnableVertexAttribArray(2);

	//unbind VAO
	glBindVertexArray(0);
}

#endif