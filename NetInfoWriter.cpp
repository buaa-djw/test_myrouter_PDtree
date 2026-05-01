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
        double route_cost_total = 0.0;
        double wirelength = 0.0;
        int hbt_count = 0;
        for (const auto& n : r.tree_nodes) {
            route_cost_total += static_cast<double>(n.incoming_wire_length);
        }
        for (const auto& s : r.segments) {
            wirelength += std::abs(s.p1.x - s.p2.x) + std::abs(s.p1.y - s.p2.y);
            if (s.uses_hbt) {
                hbt_count++;
            }
        }
        o << "NET " << r.net_name << "\n";
        o << "  type: " << (r.is_3d ? "3D" : "2D") << "\n";
        o << "  success: " << r.success << "\n";
        o << "  status: " << r.status << "\n";
        o << "  fail_reason: " << r.fail_reason << "\n";
        o << "  route_cost_total: " << route_cost_total << "\n";
        o << "  hbt_count: " << hbt_count << "\n";
        o << "  wirelength: " << wirelength << "\n";
        o << "  avg_sink_delay: " << r.delay_summary.avg_sink_delay << "\n";
        o << "  max_sink_delay: " << r.delay_summary.max_sink_delay << "\n";
        o << "  validation: valid=" << r.validation.valid
          << " non_hbt_cross_die_segments=" << r.validation.non_hbt_cross_die_segments
          << " invalid_hbt_segments=" << r.validation.invalid_hbt_segments
          << " disconnected_components=" << r.validation.disconnected_components
          << " hbt_node_segment_mismatches=" << r.validation.hbt_node_segment_mismatches << "\n";
        o << "  validation_errors:\n";
        for (const auto& e : r.validation.errors) {
            o << "    - " << e << "\n";
        }
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
            if (n.node_type == TreeNodeState::NodeType::kHBT) {
                int matched_segment_index = -1;
                for (size_t si = 0; si < r.segments.size(); ++si) {
                    const auto& seg = r.segments[si];
                    if (seg.uses_hbt && seg.hbt_id == n.assigned_hbt_id) {
                        matched_segment_index = static_cast<int>(si);
                        break;
                    }
                }
                o << "      hbt_match:\n";
                o << "        matched_segment_index: " << matched_segment_index << "\n";
                o << "        node_xy: (" << n.point.x << "," << n.point.y << ")\n";
                if (matched_segment_index >= 0) {
                    const auto& ms = r.segments[matched_segment_index];
                    o << "        segment_xy: (" << ms.p1.x << "," << ms.p1.y << ")\n";
                    o << "        node_hbt_id: " << n.assigned_hbt_id << "\n";
                    o << "        segment_hbt_id: " << ms.hbt_id << "\n";
                    o << "        match_result: "
                      << ((ms.p1.x == n.point.x && ms.p1.y == n.point.y) ? "OK" : "MISMATCH")
                      << "\n";
                } else {
                    o << "        segment_xy: N/A\n";
                    o << "        node_hbt_id: " << n.assigned_hbt_id << "\n";
                    o << "        segment_hbt_id: N/A\n";
                    o << "        match_result: NO_SEGMENT\n";
                }
            }
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
