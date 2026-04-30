#include "PlotDataWriter.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
std::string safeNetName(const std::string& n)
{
    std::string s = n;
    for (char& c : s) {
        if (!(isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')) {
            c = '_';
        }
    }
    return s;
}

double z(DieId d) { return d == DieId::kTop ? 1.0 : 0.0; }
}

void write3DNetPlotData(const std::string& dir, const NetRouteResult& r)
{
    std::filesystem::create_directories(dir);
    json j;
    j["net_name"] = r.net_name;
    j["validation"] = {
        {"valid", r.validation.valid},
        {"errors", r.validation.errors},
        {"non_hbt_cross_die_segments", r.validation.non_hbt_cross_die_segments},
        {"invalid_hbt_segments", r.validation.invalid_hbt_segments},
        {"disconnected_components", r.validation.disconnected_components},
        {"hbt_node_segment_mismatches", r.validation.hbt_node_segment_mismatches}
    };
    j["points"] = json::array();
    for (size_t i = 0; i < r.tree_nodes.size(); ++i) {
        const auto& n = r.tree_nodes[i];
        std::string type = "steiner";
        if (n.node_type == TreeNodeState::NodeType::kPin) type = (i == static_cast<size_t>(r.root_tree_index)) ? "source" : "sink";
        if (n.node_type == TreeNodeState::NodeType::kHBT) type = "hbt_node";
        j["points"].push_back({{"id", static_cast<int>(i)}, {"type", type}, {"source", "tree_node"}, {"x", n.point.x}, {"y", n.point.y}, {"z", z(n.point.die)}});
    }
    for (const auto& s : r.segments) {
        if (s.uses_hbt) {
            j["points"].push_back({{"type", "hbt"}, {"source", "segment"}, {"x", s.p1.x}, {"y", s.p1.y}, {"z", 0.5}, {"hbt_id", s.hbt_id}});
        }
    }
    j["segments"] = json::array();
    for (const auto& s : r.segments) {
        j["segments"].push_back({{"x1", s.p1.x}, {"y1", s.p1.y}, {"z1", z(s.p1.die)}, {"x2", s.p2.x}, {"y2", s.p2.y}, {"z2", z(s.p2.die)}, {"uses_hbt", s.uses_hbt}, {"hbt_id", s.hbt_id}});
    }
    std::ofstream(dir + "/" + safeNetName(r.net_name) + ".json") << j.dump(2);
}
