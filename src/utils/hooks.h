#pragma once

#include "common.h"

namespace hooks
{
	bool Initialize(std::vector<std::string> &missing);
	bool Cleanup();
} // namespace hooks
