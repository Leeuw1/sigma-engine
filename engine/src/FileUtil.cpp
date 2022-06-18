#include "FileUtil.h"

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>

#include <string>
#include <thread>
#include <unordered_map>
#include <map>

namespace sge::file
{
	MeshData LoadOBJFile(const std::string& filepath, size_t floatsPerVertex)
	{
		std::ifstream file(filepath);
		SGE_ASSERTF(file.is_open(), "Could not open file '%s'.", filepath.c_str());

		SGE_INFOF("Parsing file '%s'...", filepath.c_str());

		std::vector<float> vertices;
		std::vector<uint32_t> indices;

		float x, y, z;
		char c;
		
		std::string temp;
		for (;;)
		{
			file >> c;

			if (c == 'v')
				break;

			std::getline(file, temp);
		}

		// Re-adjust before entering loop
		file >> x >> y >> z;
		vertices.insert(vertices.end(), { x, -y, z }); // Flip y-coordinate

		// Pad with zeros based on stride
		for (size_t i = 0; i < floatsPerVertex - 3; i++)
			vertices.push_back(0.0f);

		SGE_INFO("Reading vertices...");
		for (;;)
		{
			file >> c;

			if (c == 'f')
				break;
			
			file >> x >> y >> z;
			vertices.insert(vertices.end(), { x, -y, z });
			
			// Pad with zeros based on stride
			for (size_t i = 0; i < floatsPerVertex - 3; i++)
				vertices.push_back(0.0f);
		}
		SGE_TRACE("Loaded vertices.");

		uint32_t v0, v1, v2;

		// Re-adjust before entering loop
		file >> v0 >> v1 >> v2;
		indices.insert(indices.end(), { v0 - 1, v1 - 1, v2 - 1 });

		SGE_INFO("Reading indices...");
		while (!file.eof())
		{
			file >> c >> v0 >> v1 >> v2;
			// Indices are off by 1 in this file format
			indices.insert(indices.end(), { v0 - 1, v1 - 1, v2 - 1 });
		}
		SGE_TRACE("Loaded indices.");
		SGE_TRACEF("Finished parsing file. %d vertices, %d indices.", vertices.size() / floatsPerVertex, indices.size());

		return { vertices, indices };
	}

	// This function assumes that vertices are in the format: Vertex{ float, float, float }, Normal{ float, float, float }
	void CalculateNormals(std::vector<float>& vertices, const std::vector<uint32_t>& indices)
	{
		SGE_INFO("Calculating normals...");

		auto CalcNormals = [&](size_t startIndex, size_t endIndex)
		{
			// Loop through all indices
			for (size_t i = startIndex; i < endIndex; i++)
			{
				glm::vec3 vertexNormal = {};
				float divisor = 0.0f;

				// Check for index in each triangle primitive
				for (size_t j = 0; j < indices.size(); j += 3)
				{
					uint32_t i0 = indices[j];
					uint32_t i1 = indices[j + 1];
					uint32_t i2 = indices[j + 2];

					if (i0 == i || i1 == i || i2 == i)
					{
						// Calculate face normal
						glm::vec3 v0 = { vertices[6 * i0], vertices[6 * i0 + 1], vertices[6 * i0 + 2] };
						glm::vec3 v1 = { vertices[6 * i1], vertices[6 * i1 + 1], vertices[6 * i1 + 2] };
						glm::vec3 v2 = { vertices[6 * i2], vertices[6 * i2 + 1], vertices[6 * i2 + 2] };

						glm::vec3 a = v1 - v0;
						glm::vec3 b = v2 - v0;

						// Order of a and b is important
						vertexNormal += glm::normalize(glm::cross(b, a));
						divisor++;
					}
				}

				// Average the face normals
				vertexNormal /= divisor;

				vertices[6 * i + 3] = vertexNormal.x;
				vertices[6 * i + 4] = vertexNormal.y;
				vertices[6 * i + 5] = vertexNormal.z;
			}
		};

		constexpr size_t floatsPerVertex = 6;
		size_t verticesCount = vertices.size() / floatsPerVertex;
		size_t quarterVerticesCount = verticesCount / 4;

		std::jthread thread0(CalcNormals, 0, quarterVerticesCount);
		std::jthread thread1(CalcNormals, quarterVerticesCount, 2 * quarterVerticesCount);
		std::jthread thread2(CalcNormals, 2 * quarterVerticesCount, 3 * quarterVerticesCount);
		CalcNormals(3 * quarterVerticesCount, verticesCount);

		SGE_TRACE("Finished calculating normals.");
	}
} // namespace sge::file