#pragma once

#include "astrodyn/state.hpp"
#include <string>
#include <vector>

namespace astrodyn {

void write_samples_csv(const std::string& path, const std::vector<SampleRow>& rows);
void ensure_parent_dir(const std::string& path);

} // namespace astrodyn
