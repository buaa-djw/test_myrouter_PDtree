#include "NetInfoWriter.h"

#include <fstream>

namespace {
const char* nodeTypeToString(TreeNodeState::NodeType t)
{
    switch (t) {
        case TreeNodeState::NodeType::kPin: return "pin";
        case TreeNodeState::NodeType::kSteiner: return "steiner";
        case TreeNodeState::NodeType::kHBT: return "hbt";
    }
    return "unknown";
}

const char* segTag(const RoutedSegment& s)
{
    if (!s.uses_hbt && s.p1.die != s.p2.die) return "NON_HBT_CROSS_DIE";
    if (s.uses_hbt && (s.p1.x != s.p2.x || s.p1.y != s.p2.y || s.p1.die == s.p2.die || s.hbt_id < 0)) return "INVALID_HBT_SEGMENT";
    return "OK";
}
}

void writeNetInfo(const std::string& path, const RouterDB&, const std::vector<NetRouteResult>& results)
{
    std::ofstream o(path);
    for (const auto& r : results) {
        o << "NET " << r.net_name << "\n";
        o << "  type: " << (r.is_3d ? "3D" : "2D") << "\n";
        o << "  success: " << r.success << "\n";
        o << "  status: " << r.status << "\n";
        o << "  fail_reason: " << r.fail_reason << "\n";
        o << "  validation: valid=" << r.validation.valid
          << " non_hbt_cross_die_segments=" << r.validation.non_hbt_cross_die_segments
          << " invalid_hbt_segments=" << r.validation.invalid_hbt_segments
          << " disconnected_components=" << r.validation.disconnected_components
          << " hbt_node_segment_mismatches=" << r.validation.hbt_node_segment_mismatches << "\n";
        o << "  tree_nodes:\n";
        for (size_t i = 0; i < r.tree_nodes.size(); ++i) {
            const auto& n = r.tree_nodes[i];
            o << "    - index: " << i << "\n";
            o << "      parent_index: " << n.parent_index << "\n";
            o << "      node_type: " << nodeTypeToString(n.node_type) << "\n";
            o << "      pin_index: " << n.pin_index << "\n";
            o << "      x: " << n.point.x << " y: " << n.point.y << " die: " << static_cast<int>(n.point.die) << "\n";
            o << "      depth: " << n.depth << " path_length_from_root: " << n.path_length_from_root
              << " hbt_count_from_root: " << n.hbt_count_from_root << "\n";
            o << "      incoming_wire_length: " << n.incoming_wire_length << " incoming_hbt_count: " << n.incoming_hbt_count << "\n";
            o << "      incoming_wire_res: " << n.incoming_wire_res << " incoming_wire_cap: " << n.incoming_wire_cap << "\n";
            o << "      incoming_hbt_res: " << n.incoming_hbt_res << " incoming_hbt_cap: " << n.incoming_hbt_cap << "\n";
            o << "      assigned_hbt_id: " << n.assigned_hbt_id << " hbt_assigned: " << n.hbt_assigned << "\n";
            o << "      hbt_from_die: " << static_cast<int>(n.hbt_from_die) << " hbt_to_die: " << static_cast<int>(n.hbt_to_die) << "\n";
            o << "      incoming_segment_begin: " << n.incoming_segment_begin << " incoming_segment_count: " << n.incoming_segment_count << "\n";
        }
        o << "  segments:\n";
        for (size_t i = 0; i < r.segments.size(); ++i) {
            const auto& s = r.segments[i];
            o << "    - index: " << i
              << " p1:(" << s.p1.x << "," << s.p1.y << "," << static_cast<int>(s.p1.die) << ")"
              << " p2:(" << s.p2.x << "," << s.p2.y << "," << static_cast<int>(s.p2.die) << ")"
              << " uses_hbt: " << s.uses_hbt << " hbt_id: " << s.hbt_id
              << " validation_tag: " << segTag(s) << "\n";
        }
        o << "\n";
    }
}
