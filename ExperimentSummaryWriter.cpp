#include "ExperimentSummaryWriter.h"

#include <fstream>

void writeExperimentSummary(const std::string& path,
                            const ExperimentConfig& cfg,
                            const RouterDB&,
                            const std::vector<NetRouteResult>& results,
                            const std::string& st,
                            const std::string& et,
                            double rt)
{
    std::ofstream o(path);
    int routed_success = 0;
    int failed = 0;
    int invalid_topology_nets = 0;
    int non_hbt_cross_die_segments = 0;
    int invalid_hbt_segments = 0;
    int disconnected_3d_nets = 0;
    int hbt_node_segment_mismatch_nets = 0;

    for (const auto& r : results) {
        if (r.success) routed_success++; else failed++;
        if (r.status == "invalid_topology") invalid_topology_nets++;
        non_hbt_cross_die_segments += r.validation.non_hbt_cross_die_segments;
        invalid_hbt_segments += r.validation.invalid_hbt_segments;
        if (r.validation.disconnected_components > 1) disconnected_3d_nets++;
        if (r.validation.hbt_node_segment_mismatches > 0) hbt_node_segment_mismatch_nets++;
    }

    o << "Experiment Info\nname=" << cfg.experiment_name << "\nstart=" << st << "\nend=" << et << "\nruntime=" << rt << "\n\n";
    o << "Overall Metrics\n";
    o << "total_nets=" << results.size() << "\n";
    o << "routed_success=" << routed_success << "\n";
    o << "failed_nets=" << failed << "\n";
    o << "invalid_topology_nets=" << invalid_topology_nets << "\n";
    o << "non_hbt_cross_die_segments=" << non_hbt_cross_die_segments << "\n";
    o << "invalid_hbt_segments=" << invalid_hbt_segments << "\n";
    o << "disconnected_3d_nets=" << disconnected_3d_nets << "\n";
    o << "hbt_node_segment_mismatch_nets=" << hbt_node_segment_mismatch_nets << "\n\n";

    o << "Per-net Summary\n";
    for (const auto& r : results) {
        o << "net=" << r.net_name << " status=" << r.status << " route_function_success=" << r.success
          << " topology_valid=" << r.validation.valid << " delay_ready=" << r.delay_summary.ready
          << " component_count=" << r.validation.disconnected_components
          << " non_hbt_cross_die_segments=" << r.validation.non_hbt_cross_die_segments
          << " invalid_hbt_segments=" << r.validation.invalid_hbt_segments
          << " hbt_node_segment_mismatches=" << r.validation.hbt_node_segment_mismatches
          << " validation_errors=" << r.fail_reason << "\n";
    }
}
