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
    int routed_2d = 0;
    int routed_3d = 0;
    int topology_valid = 0;
    int delay_ready = 0;
    double total_cost = 0.0;
    double total_wl = 0.0;
    int total_hbt = 0;
    for (const auto& r : results) {
        routed_success += r.success ? 1 : 0;
        routed_2d += (!r.is_3d && r.success) ? 1 : 0;
        routed_3d += (r.is_3d && r.success) ? 1 : 0;
        topology_valid += r.validation.valid ? 1 : 0;
        delay_ready += r.delay_summary.ready ? 1 : 0;
        total_cost += r.route_cost_total;
        for (const auto& s : r.segments) {
            total_wl += std::abs(s.p1.x - s.p2.x) + std::abs(s.p1.y - s.p2.y);
            total_hbt += s.uses_hbt ? 1 : 0;
        }
    }
    o << "Experiment Info\n";
    o << "experiment_name=" << cfg.experiment_name << "\nbenchmark=" << cfg.benchmark << "\ncost_mode=" << cfg.cost_mode << "\nstart=" << st << "\nend=" << et << "\nruntime=" << rt << "\noutput_dir=" << cfg.output.output_dir << "\n\n";
    o << "Overall Metrics\n";
    o << "total_nets=" << results.size() << "\nrouted_success=" << routed_success << "\nrouted_failed=" << (static_cast<int>(results.size()) - routed_success) << "\nrouted_2d_nets=" << routed_2d << "\nrouted_3d_nets=" << routed_3d << "\ntopology_valid_nets=" << topology_valid << "\ndelay_ready_nets=" << delay_ready << "\n\n";
    o << "Aggregated Metrics\n";
    o << "total_wirelength=" << total_wl << "\ntotal_route_cost=" << total_cost << "\naverage_route_cost=" << (results.empty()?0.0:total_cost/results.size()) << "\ntotal_hbt_count=" << total_hbt << "\n\n";
}
