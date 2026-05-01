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
    int route_function_success = 0;
    int topology_valid_nets = 0;
    int delay_ready_nets = 0;
    int routed_2d_nets = 0;
    int routed_3d_nets = 0;
    int invalid_3d_nets = 0;
    int unknown_die_nets = 0;
    int invalid_topology_nets = 0;
    int non_hbt_cross_die_segments = 0;
    int invalid_hbt_segments = 0;
    int disconnected_3d_nets = 0;
    int hbt_node_segment_mismatch_nets = 0;

    for (const auto& r : results) {
        if (r.success) route_function_success++;
        if (r.validation.valid) topology_valid_nets++;
        if (r.delay_summary.ready) delay_ready_nets++;
        if (!r.is_3d && r.success) routed_2d_nets++;
        if (r.is_3d && r.success) routed_3d_nets++;
        if (r.is_3d && !r.validation.valid) invalid_3d_nets++;
        if (r.status == "invalid_topology") invalid_topology_nets++;
        non_hbt_cross_die_segments += r.validation.non_hbt_cross_die_segments;
        invalid_hbt_segments += r.validation.invalid_hbt_segments;
        if (r.validation.disconnected_components > 1) disconnected_3d_nets++;
        if (r.validation.hbt_node_segment_mismatches > 0) hbt_node_segment_mismatch_nets++;
        for (const auto& e : r.validation.errors) {
            if (e.find("unknown_die") != std::string::npos) {
                unknown_die_nets++;
                break;
            }
        }
    }

    o << "Experiment Info\nname=" << cfg.experiment_name << "\nstart=" << st << "\nend=" << et << "\nruntime=" << rt << "\n\n";
    o << "Overall Metrics\n";
    o << "total_nets=" << results.size() << "\n";
    o << "route_function_success=" << route_function_success << "\n";
    o << "topology_valid_nets=" << topology_valid_nets << "\n";
    o << "invalid_topology_nets=" << invalid_topology_nets << "\n";
    o << "delay_ready_nets=" << delay_ready_nets << "\n";
    o << "routed_2d_nets=" << routed_2d_nets << "\n";
    o << "routed_3d_nets=" << routed_3d_nets << "\n";
    o << "invalid_3d_nets=" << invalid_3d_nets << "\n";
    o << "unknown_die_nets=" << unknown_die_nets << "\n";
    o << "non_hbt_cross_die_segments=" << non_hbt_cross_die_segments << "\n";
    o << "invalid_hbt_segments=" << invalid_hbt_segments << "\n";
    o << "disconnected_3d_nets=" << disconnected_3d_nets << "\n";
    o << "hbt_node_segment_mismatch_nets=" << hbt_node_segment_mismatch_nets << "\n\n";
    o << "config.output.dump_invalid_3d_nets=" << cfg.output.dump_invalid_3d_nets << "\n\n";
    o << "report_cost.alpha_path_depth=" << cfg.report_cost.alpha_path_depth << "\n";
    o << "report_cost.stretch_limit=" << cfg.report_cost.stretch_limit << "\n";
    o << "report_cost.beta_stretch=" << cfg.report_cost.beta_stretch << "\n";
    o << "report_cost.beta_hbt_depth=" << cfg.report_cost.beta_hbt_depth << "\n";
    o << "report_cost.beta_hbt_stack=" << cfg.report_cost.beta_hbt_stack << "\n";
    o << "report_cost.beta_hbt_branch=" << cfg.report_cost.beta_hbt_branch << "\n";
    o << "report_cost.beta_hbt_rc=" << cfg.report_cost.beta_hbt_rc << "\n";
    o << "report_cost.beta_cap_load=" << cfg.report_cost.beta_cap_load << "\n";
    o << "report_cost.max_hbt_per_path=" << cfg.report_cost.max_hbt_per_path << "\n";
    o << "report_cost.max_hbt_per_net=" << cfg.report_cost.max_hbt_per_net << "\n\n";

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
