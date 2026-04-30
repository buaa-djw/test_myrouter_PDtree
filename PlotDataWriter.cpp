#include "PlotDataWriter.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

std::string safeNetName(const std::string& name)
{
    std::string safe_name = name;

    for (char& c : safe_name) {
        const bool is_valid_char =
            std::isalnum(static_cast<unsigned char>(c)) ||
            c == '_' ||
            c == '-';

        if (!is_valid_char) {
            c = '_';
        }
    }

    return safe_name;
}

namespace {

// Convert die id to the z coordinate used by the 3D plot.
//
// Top die is drawn at z = 1, bottom die is drawn at z = 0.
// Unknown die is drawn at z = 0.5 so that invalid data is still visible
// instead of being silently hidden.
double dieToZ(DieId die)
{
    if (die == DieId::kTop) {
        return 1.0;
    }

    if (die == DieId::kBottom) {
        return 0.0;
    }

    return 0.5;
}

std::string nodeTypeToString(const TreeNodeState& node, bool is_root)
{
    if (node.node_type == TreeNodeState::NodeType::kHBT) {
        return "hbt_node";
    }

    if (node.node_type == TreeNodeState::NodeType::kPin) {
        return is_root ? "source" : "sink";
    }

    return "steiner";
}

std::string segmentValidationTag(const RoutedSegment& segment)
{
    const bool cross_die = segment.p1.die != segment.p2.die;

    // In F2F-HB routing, an ordinary 2D wire segment is not allowed
    // to connect two different dies. Cross-die connection must be
    // represented by an explicit HBT segment.
    if (!segment.uses_hbt && cross_die) {
        return "NON_HBT_CROSS_DIE";
    }

    if (segment.uses_hbt) {
        const bool is_valid_hbt_segment =
            segment.p1.x == segment.p2.x &&
            segment.p1.y == segment.p2.y &&
            segment.p1.die != segment.p2.die &&
            segment.hbt_id >= 0;

        if (!is_valid_hbt_segment) {
            return "INVALID_HBT_SEGMENT";
        }
    }

    return "OK";
}

void appendTreeNodePoint(json& points,
                         const TreeNodeState& node,
                         int node_id,
                         bool is_root)
{
    points.push_back({
        {"id", node_id},
        {"type", nodeTypeToString(node, is_root)},
        {"source", "tree_node"},
        {"pin_index", node.pin_index},
        {"x", node.point.x},
        {"y", node.point.y},
        {"z", dieToZ(node.point.die)},
        {"die", static_cast<int>(node.point.die)},
        {"assigned_hbt_id", node.assigned_hbt_id}
    });
}

void appendHbtSegmentPoint(json& points, const RoutedSegment& segment)
{
    // HBT marker is emitted from the real HBT segment, not only from
    // TreeNodeState::kHBT.
    //
    // This is important because the tree node and the materialized segment
    // may become inconsistent during debugging. Drawing the segment-based
    // marker makes the real HBT crossing location visible in the PNG.
    points.push_back({
        {"type", "hbt"},
        {"source", "segment"},
        {"x", segment.p1.x},
        {"y", segment.p1.y},
        {"z", 0.5},
        {"die", "between"},
        {"hbt_id", segment.hbt_id}
    });
}

void appendSegment(json& segments, const RoutedSegment& segment)
{
    segments.push_back({
        {"x1", segment.p1.x},
        {"y1", segment.p1.y},
        {"z1", dieToZ(segment.p1.die)},
        {"die1", static_cast<int>(segment.p1.die)},

        {"x2", segment.p2.x},
        {"y2", segment.p2.y},
        {"z2", dieToZ(segment.p2.die)},
        {"die2", static_cast<int>(segment.p2.die)},

        {"uses_hbt", segment.uses_hbt},
        {"hbt_id", segment.hbt_id},
        {"validation_tag", segmentValidationTag(segment)}
    });
}

void appendValidationBlock(json& j, const NetRouteResult& result)
{
    j["validation"] = {
        {"valid", result.validation.valid},
        {"errors", result.validation.errors},
        {"non_hbt_cross_die_segments",
         result.validation.non_hbt_cross_die_segments},
        {"invalid_hbt_segments",
         result.validation.invalid_hbt_segments},
        {"disconnected_components",
         result.validation.disconnected_components},
        {"hbt_node_segment_mismatches",
         result.validation.hbt_node_segment_mismatches}
    };
}

int computeWirelength(const NetRouteResult& result)
{
    int wirelength = 0;

    for (const RoutedSegment& segment : result.segments) {
        wirelength += std::abs(segment.p1.x - segment.p2.x);
        wirelength += std::abs(segment.p1.y - segment.p2.y);
    }

    return wirelength;
}

int countHbtSegments(const NetRouteResult& result)
{
    int hbt_count = 0;

    for (const RoutedSegment& segment : result.segments) {
        if (segment.uses_hbt) {
            ++hbt_count;
        }
    }

    return hbt_count;
}

}  // namespace

void write3DNetPlotData(const std::string& dir, const NetRouteResult& result)
{
    std::filesystem::create_directories(dir);

    json j;

    j["net_name"] = result.net_name;
    j["is_3d"] = result.is_3d;
    j["success"] = result.success;
    j["status"] = result.status;
    j["fail_reason"] = result.fail_reason;

    j["wirelength"] = computeWirelength(result);
    j["hbt_count"] = countHbtSegments(result);
    j["tree_radius"] = 0;

    j["avg_sink_delay"] = result.delay_summary.avg_sink_delay;
    j["max_sink_delay"] = result.delay_summary.max_sink_delay;
    j["delay_ready"] = result.delay_summary.ready;

    appendValidationBlock(j, result);

    j["points"] = json::array();

    for (std::size_t i = 0; i < result.tree_nodes.size(); ++i) {
        const TreeNodeState& node = result.tree_nodes[i];
        const bool is_root = static_cast<int>(i) == result.root_tree_index;

        appendTreeNodePoint(
            j["points"],
            node,
            static_cast<int>(i),
            is_root
        );
    }

    for (const RoutedSegment& segment : result.segments) {
        if (segment.uses_hbt) {
            appendHbtSegmentPoint(j["points"], segment);
        }
    }

    j["segments"] = json::array();

    for (const RoutedSegment& segment : result.segments) {
        appendSegment(j["segments"], segment);
    }

    const std::filesystem::path output_path =
        std::filesystem::path(dir) / (safeNetName(result.net_name) + ".json");

    std::ofstream output(output_path);
    output << j.dump(2);
}