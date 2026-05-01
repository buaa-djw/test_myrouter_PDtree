#pragma once

#include "Grid.h"
#include "RouterDB.h"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

struct RoutedPoint
{
    int x = 0;
    int y = 0;
    DieId die = DieId::kUnknown;
};

struct RoutedSegment
{
    RoutedPoint p1;
    RoutedPoint p2;

    int layer = -1;

    bool uses_hbt = false;
    int hbt_id = -1;
};

struct TreeNodeState
{
    enum class NodeType {
        kPin = 0,
        kSteiner,
        kHBT
    };

    int pin_index = -1;
    RoutedPoint point;
    NodeType node_type = NodeType::kPin;

    int parent_index = -1;

    int depth = 0;
    int path_length_from_root = 0;
    int hbt_count_from_root = 0;

    // Branch statistics from parent -> this node.
    // These fields are populated when commitSegments() appends this sink branch.
    int incoming_wire_length = 0;
    int incoming_hbt_count = 0;
    double incoming_wire_res = 0.0;
    double incoming_wire_cap = 0.0;
    double incoming_hbt_res = 0.0;
    double incoming_hbt_cap = 0.0;

    // Explicit HBT-inner-node metadata.
    // This allows route3DNet to first build a topology skeleton (parent->HBT->subtree)
    // and then optimize/legalize HBT location at subtree level.
    int assigned_hbt_id = -1;
    std::vector<int> candidate_hbt_ids;
    bool hbt_assigned = false;
    DieId hbt_from_die = DieId::kUnknown;
    DieId hbt_to_die = DieId::kUnknown;
    double hbt_res = 0.0;
    double hbt_cap = 0.0;

    // Segment range in NetRouteResult::segments for the incoming branch.
    int incoming_segment_begin = -1;
    int incoming_segment_count = 0;
};

struct SinkDelayInfo
{
    int pin_index = -1;
    std::string pin_name;
    double delay = 0.0;
    int path_length = 0;
    int hbt_count_from_root = 0;
};

struct NetDelaySummary
{
    bool ready = false;
    double avg_sink_delay = 0.0;
    double max_sink_delay = 0.0;
    int max_delay_pin_index = -1;
    std::string max_delay_pin_name;
    std::vector<SinkDelayInfo> sink_delays;
};

struct RouteValidationResult
{
    bool valid = true;
    std::vector<std::string> errors;
    int non_hbt_cross_die_segments = 0;
    int invalid_hbt_segments = 0;
    int disconnected_components = 0;
    int hbt_node_segment_mismatches = 0;
};

struct NetRouteResult
{
    std::string net_name;
    bool success = false;
    bool is_3d = false;
    std::string status = "unrouted";
    std::string fail_reason;

    // Explicitly track which tree_nodes entry is the source/root.
    int root_tree_index = 0;

    std::vector<TreeNodeState> tree_nodes;
    std::vector<RoutedSegment> segments;

    // Per-net delay annotation result (filled by EDCompute).
    NetDelaySummary delay_summary;
    RouteValidationResult validation;
};

class PDTreeRouter
{
public:
    struct RouteRunStats {
        int routed_success = 0;
        int routed_failed = 0;
        int skipped_clock = 0;
        int skipped_special = 0;
    };

    // Report-aligned PD-tree cost parameters.
    // The core candidate formula is:
    //   Cost(i, j) = d_ij + alpha * l_i
    //              + StretchPenalty(i, j)
    //              + Phi_HBT(i, j)
    //              + CapPenalty(i, j)
    struct ReportCostParams {
        double alpha_path_depth = 0.5;
        double stretch_limit = 1.5;
        double beta_stretch = 1.0;

        double beta_hbt_depth = 1.0;
        double beta_hbt_stack = 1.0;
        double beta_hbt_branch = 1.0;
        double beta_hbt_rc = 1.0;

        double beta_cap_load = 1.0;

        // 0 means unlimited. A positive value is treated as a hard constraint.
        int max_hbt_per_path = 2;
        int max_hbt_per_net = 0;
    };

    struct Params {
        // Driver/source lumped resistance (Ohm-equivalent proxy in current model units).
        double source_res = 50.0;

        // Vertical HBT RC proxy per crossing.
        double hbt_res = 5.0;
        double hbt_cap = 0.001;

        // Default sink pin capacitance when no explicit pin cap is available.
        double default_sink_cap = 0.001;

        // Core routing cost. EDCompute is intentionally not used for candidate
        // selection; it is only used for final report delay annotation.
        ReportCostParams report_cost;

        // Deprecated legacy objective weights. They are kept only so older
        // configuration/main.cpp assignments do not break compilation. They are
        // not used by the core routing cost path.
        double weight_avg_delay = 1.0;
        double weight_max_delay = 0.10;
        double weight_wirelength = 1e-5;
        double weight_hbt_count = 0.05;
        double weight_stretch = 1e-5;
        double weight_hbt_depth = 0.01;
        double weight_hbt_stack = 0.03;
        double weight_hbt_fanout = 0.01;
        double weight_hbt_scarcity = 0.02;
        double weight_hbt_position = 1e-5;
        double weight_hbt_subtree_wire = 1e-5;
        double weight_hbt_subtree_delay_proxy = 0.03;

        int max_hbt_search_radius = 0;  // 0 => auto
        int max_candidate_parents = 64;
        int max_candidate_hbts = 128;
        int beam_width_3d = 4;
        int beam_branch_candidates_3d = 6;
        int max_local_hbt_candidates = 24;
        int max_hbt_nearest_k = 16;

        bool enable_hbt_inner_node_optimization = false;
        bool use_proposed_cost = true;
        bool dump_candidate_cost_debug = false;

        bool verbose = true;
    };

    struct TimingSummary {
        int sink_count = 0;
        double avg_sink_delay = 0.0;
        double max_sink_delay = 0.0;
        double total_wirelength = 0.0;
        int hbt_count = 0;
        double avg_stretch = 0.0;
    };

private:
    struct ReportCostBreakdown {
        bool valid = false;
        double total = 0.0;

        double dij = 0.0;
        double parent_path_length = 0.0;
        double pd_depth_term = 0.0;

        double candidate_path_length = 0.0;
        double src_to_sink_manhattan = 0.0;
        double stretch_violation = 0.0;
        double stretch_penalty = 0.0;

        double hbt_depth_penalty = 0.0;
        double hbt_stack_penalty = 0.0;
        double hbt_branch_penalty = 0.0;
        double hbt_rc_penalty = 0.0;
        double hbt_penalty = 0.0;

        double sink_cap = 0.0;
        double cap_penalty = 0.0;
    };

    struct CandidateScore {
        bool valid = false;
        double objective = 0.0;
        ReportCostBreakdown cost;

        // Tie-break/debug fields. These do not define the primary cost.
        int hbt_count = 0;
        double total_wirelength = 0.0;
    };

    struct PartialRouteState {
        NetRouteResult result;
        std::vector<bool> in_tree;
        std::unordered_set<int> local_reserved_hbts;
        CandidateScore score;
        double accumulated_cost = 0.0;
        int attached_sinks = 0;
    };

public:
    PDTreeRouter(const RouterDB& db,
                 const HybridGrid& grid,
                 const Params& params);
    PDTreeRouter(const RouterDB& db,
                 const HybridGrid& grid);

    std::vector<NetRouteResult> routeAllNets();
    std::vector<NetRouteResult> routeSignalNets(int max_nets,
                                                RouteRunStats* stats = nullptr);
    NetRouteResult routeSingleNet(const Net& net);

    TimingSummary evaluateTimingSummaryPublic(const Net& net,
                                              const NetRouteResult& result) const;
    double evaluateObjectivePublic(const Net& net,
                                   const NetRouteResult& result) const;
    bool annotateDelayPublic(const Net& net,
                             NetRouteResult& result) const;
    std::vector<int> collectCandidateParentsPublic(const Net& net,
                                                   const NetRouteResult& result,
                                                   const Pin& sink) const;
    bool build2DConnectionPublic(const RoutedPoint& a,
                                 const RoutedPoint& b,
                                 std::vector<RoutedSegment>& out_segments) const;
    RoutedPoint pinToPointPublic(const Pin& pin) const;

private:
    NetRouteResult route2DNet(const Net& net);
    NetRouteResult route3DNet(const Net& net);
    NetRouteResult route3DNetRobustFallback(const Net& net,
                                            const std::string& reason);

    bool shouldRouteAsSignalNet(const Net& net) const;

    int chooseSourcePin(const Net& net) const;

    bool connectSinkSameDie(const Net& net,
                            int sink_pin_index,
                            DieId net_route_die,
                            NetRouteResult& result,
                            std::vector<bool>& in_tree);

    bool commitCrossDieBranch(NetRouteResult& result,
                             int parent_tree_index,
                             int sink_pin_index,
                             int assigned_hbt_id) const;

    bool connectSinkCrossDie(const Net& net,
                             int sink_pin_index,
                             NetRouteResult& result,
                             std::vector<bool>& in_tree,
                             std::unordered_set<int>& local_reserved_hbts);

    std::vector<int> build3DSinkOrder(const Net& net, int source_pin_index) const;

    std::vector<PartialRouteState> expandSinkSameDieCandidates(const Net& net,
                                                               int sink_pin_index,
                                                               const PartialRouteState& state) const;
    std::vector<PartialRouteState> expandSinkCrossDieCandidates(const Net& net,
                                                                int sink_pin_index,
                                                                const PartialRouteState& state) const;
    int selectRobustFallbackHBT(const Net& net,
                                const std::vector<int>& top_pin_indices,
                                const std::vector<int>& bottom_pin_indices) const;
    bool connectRemainingPinsSameDieDeterministic(const Net& net,
                                                  NetRouteResult& result,
                                                  std::vector<bool>& in_tree) const;

    bool commit3DBranchWithHBTNode(NetRouteResult& result,
                                   int sink_pin_index,
                                   int parent_tree_index,
                                   int hbt_node_index,
                                   const std::vector<RoutedSegment>& seg_parent_to_hbt,
                                   const std::vector<RoutedSegment>& seg_hbt_to_sink,
                                   const RoutedPoint& hbt_point,
                                   const RoutedPoint& sink_point,
                                   const std::vector<int>& hbt_candidates,
                                   DieId from_die,
                                   DieId to_die,
                                   int assigned_hbt_id) const;

    ReportCostBreakdown computeReportPDCost(const Net& net,
                                             const NetRouteResult& current_tree,
                                             int parent_tree_index,
                                             int sink_pin_index,
                                             const RoutedPoint& attach_point,
                                             bool introduces_hbt,
                                             int hbt_id,
                                             double extra_path_after_attach = 0.0) const;

    CandidateScore scoreAttachmentCandidate(const Net& net,
                                            const NetRouteResult& current,
                                            const std::vector<RoutedSegment>& candidate_segs,
                                            int sink_pin_index,
                                            int parent_tree_index,
                                            const RoutedPoint& sink_attach_point,
                                            int extra_hbt_count,
                                            double extra_path_after_attach = 0.0,
                                            int hbt_id = -1) const;

    // Deprecated compatibility function. Core routing uses accumulated
    // report-aligned PD cost, not a post-commit EDCompute objective.
    CandidateScore evaluateStateScore(const Net& net, const NetRouteResult& state_result) const;

    bool isBetterScore(const CandidateScore& lhs,
                       const CandidateScore& rhs) const;

    TimingSummary evaluateTimingSummary(const Net& net,
                                        const NetRouteResult& result) const;

    std::vector<int> collectCandidateParents(const Net& net,
                                             const NetRouteResult& result,
                                             const Pin& sink) const;

    std::vector<int> collectCandidateHBTs(const Net& net,
                                          const Pin& pin_a,
                                          const Pin& pin_b,
                                          const std::unordered_set<int>& local_reserved_hbts) const;
    std::vector<int> collectCandidateHBTsForSubtree(const Net& net,
                                                    const std::vector<RoutedPoint>& subtree_points,
                                                    const std::unordered_set<int>& local_reserved_hbts) const;

    bool optimizeHBTInnerNodesForState(const Net& net,
                                       PartialRouteState& state) const;
    std::vector<int> evaluateHBTNodeCandidates(const Net& net,
                                               const PartialRouteState& state,
                                               int hbt_tree_node) const;
    bool materializeHBTNodesToSegments(const Net& net, PartialRouteState& state) const;
    double estimateHBTNodeCost(const Net& net,
                               const PartialRouteState& state,
                               int hbt_tree_node,
                               int candidate_hbt_id) const;
    double computeHBTScarcityPenalty(const std::unordered_set<int>& local_reserved_hbts,
                                     int hbt_id) const;
    double evaluateHBTPositionCost(const Net& net,
                                   const NetRouteResult& result) const;

    bool build2DManhattanConnection(const RoutedPoint& a,
                                    const RoutedPoint& b,
                                    std::vector<RoutedSegment>& out_segments) const;

    RouteValidationResult validateRouteResultTopology(const NetRouteResult& result) const;

    bool build3DConnectionViaHBT(const RoutedPoint& parent_pt,
                                 const Pin& sink_pin,
                                 int hbt_id,
                                 std::vector<RoutedSegment>& out_segments) const;

    void commitSegments(NetRouteResult& result,
                        const std::vector<RoutedSegment>& segs,
                        int sink_pin_index,
                        int parent_tree_index,
                        const RoutedPoint& sink_attach_point,
                        int extra_hbt_count) const;

    bool isHBTAvailableForNet(int hbt_id,
                              const std::string& net_name,
                              const std::unordered_set<int>& local_reserved_hbts) const;

    void collectHBTsFromSegments(const std::vector<RoutedSegment>& segs,
                                 std::unordered_set<int>& out_hbts) const;

    void commitNetLevelHBTReservation(const std::unordered_set<int>& local_reserved_hbts,
                                      const std::string& net_name);

    RoutedPoint pinToPoint(const Pin& pin) const;
    DieId determine2DNetDie(const Net& net, int source_pin_index) const;
    DieId resolve2DPinDie(const Pin& pin, DieId fallback_die) const;
    DieId resolve2DPointDie(const RoutedPoint& pt, DieId fallback_die) const;
    int manhattan(int x1, int y1, int x2, int y2) const;
    int manhattan(const RoutedPoint& a, const RoutedPoint& b) const;

private:
    const RouterDB& db_;
    const HybridGrid& grid_;
    Params params_;

    // Global HBT occupancy. Ownership is only committed after one full net
    // routes successfully to avoid cross-net contamination on failure.
    std::vector<bool> hbt_used_;
    std::vector<std::string> hbt_owner_;
};
