#include "astrodyn/csv.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace astrodyn {

void ensure_parent_dir(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
}

void write_samples_csv(const std::string& path, const std::vector<SampleRow>& rows) {
    ensure_parent_dir(path);
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Could not open CSV for writing: " + path);

    out << "t_s,object,x_m,y_m,vx_m_s,vy_m_s,mass_kg,radius_m,speed_m_s,energy_J_kg,h_m2_s\n";
    out << std::setprecision(17);
    for (const SampleRow& r : rows) {
        out << r.t_s << ',' << r.object << ',' << r.x_m << ',' << r.y_m << ','
            << r.vx_m_s << ',' << r.vy_m_s << ',' << r.mass_kg << ','
            << r.radius_m << ',' << r.speed_m_s << ',' << r.energy_J_kg << ',' << r.h_m2_s << '\n';
    }
}

} // namespace astrodyn
