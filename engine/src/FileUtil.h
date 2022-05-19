#pragma once

#include "base.h"

#include <vector>
#include <fstream>
#include <tuple>

namespace sge::file
{
	using MeshData = std::pair<std::vector<float>, std::vector<uint32_t>>;

	MeshData LoadOBJFile(const std::string& filepath, size_t floatsPerVertex);
	void CalculateNormals(std::vector<float>& vertices, const std::vector<uint32_t>& indices);
} // namespace sge::file