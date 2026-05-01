#include "ExperimentConfig.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::string gs(const json& j, const char* k, const std::string& d) { return j.contains(k) ? j.at(k).get<std::string>() : d; }
static double gd(const json& j, const char* k, double d) { return j.contains(k) ? j.at(k).get<double>() : d; }
static int gi(const json& j, const char* k, int d) { return j.contains(k) ? j.at(k).get<int>() : d; }
static bool gb(const json& j, const char* k, bool d) { return j.contains(k) ? j.at(k).get<bool>() : d; }

bool ExperimentConfig::loadFromFile(const std::string& p, ExperimentConfig& c, std::string& e)
{
    std::ifstream ifs(p);
    if (!ifs) {
        e = "cannot open config: " + p;
        return false;
    }
    json j;
    ifs >> j;
    c.experiment_name = gs(j, "experiment_name", c.experiment_name);
    c.benchmark = gs(j, "benchmark", c.benchmark);
    if (j.contains("input")) {
        auto& i = j["input"];
        c.input.common_lef = gs(i, "common_lef", "");
        c.input.hbt_lef = gs(i, "hbt_lef", "");
        c.input.upper_lef = gs(i, "upper_lef", "");
        c.input.bottom_lef = gs(i, "bottom_lef", "");
        c.input.def_file = gs(i, "def_file", "");
        if (i.contains("extra_lefs")) c.input.extra_lefs = i["extra_lefs"].get<std::vector<std::string>>();
    }
    if (j.contains("pd_tree")) {
        auto& pdt = j["pd_tree"];
        c.pd_tree.max_nets = gi(pdt, "max_nets", c.pd_tree.max_nets);
        c.pd_tree.max_candidate_parents = gi(pdt, "max_candidate_parents", c.pd_tree.max_candidate_parents);
        c.pd_tree.max_candidate_hbts = gi(pdt, "max_candidate_hbts", c.pd_tree.max_candidate_hbts);
        c.pd_tree.beam_width_3d = gi(pdt, "beam_width_3d", c.pd_tree.beam_width_3d);
        c.pd_tree.beam_branch_candidates_3d = gi(pdt, "beam_branch_candidates_3d", c.pd_tree.beam_branch_candidates_3d);
        c.pd_tree.max_local_hbt_candidates = gi(pdt, "max_local_hbt_candidates", c.pd_tree.max_local_hbt_candidates);
        c.pd_tree.max_hbt_nearest_k = gi(pdt, "max_hbt_nearest_k", c.pd_tree.max_hbt_nearest_k);
        c.pd_tree.verbose = gb(pdt, "verbose", c.pd_tree.verbose);
    }
    if (j.contains("report_cost")) {
        auto& rc = j["report_cost"];
        c.report_cost.alpha_path_depth = gd(rc, "alpha_path_depth", c.report_cost.alpha_path_depth);
        c.report_cost.stretch_limit = gd(rc, "stretch_limit", c.report_cost.stretch_limit);
        c.report_cost.beta_stretch = gd(rc, "beta_stretch", c.report_cost.beta_stretch);
        c.report_cost.beta_hbt_depth = gd(rc, "beta_hbt_depth", c.report_cost.beta_hbt_depth);
        c.report_cost.beta_hbt_stack = gd(rc, "beta_hbt_stack", c.report_cost.beta_hbt_stack);
        c.report_cost.beta_hbt_branch = gd(rc, "beta_hbt_branch", c.report_cost.beta_hbt_branch);
        c.report_cost.beta_hbt_rc = gd(rc, "beta_hbt_rc", c.report_cost.beta_hbt_rc);
        c.report_cost.beta_cap_load = gd(rc, "beta_cap_load", c.report_cost.beta_cap_load);
        c.report_cost.max_hbt_per_path = gi(rc, "max_hbt_per_path", c.report_cost.max_hbt_per_path);
        c.report_cost.max_hbt_per_net = gi(rc, "max_hbt_per_net", c.report_cost.max_hbt_per_net);
    }
    if (j.contains("cost_weights")) {
        std::cerr << "[config] deprecated old objective weight ignored\n";
    }
    if (j.contains("rc")) {
        auto& r = j["rc"];
        c.rc.source_res = gd(r, "source_res", c.rc.source_res);
        c.rc.default_sink_cap = gd(r, "default_sink_cap", c.rc.default_sink_cap);
        c.rc.hbt_res = gd(r, "hbt_res", c.rc.hbt_res);
        c.rc.hbt_cap = gd(r, "hbt_cap", c.rc.hbt_cap);
        c.rc.hbt_rc_scale = gd(r, "hbt_rc_scale", c.rc.hbt_rc_scale);
    }
    return true;
}

bool ExperimentConfig::validate(std::string& e) const { if(input.common_lef.empty()||input.hbt_lef.empty()||input.upper_lef.empty()||input.bottom_lef.empty()||input.def_file.empty()){e="input path missing"; return false;} return true; }
PDTreeRouter::Params ExperimentConfig::buildRouterParams() const { PDTreeRouter::Params p; p.source_res=rc.source_res; p.default_sink_cap=rc.default_sink_cap; p.hbt_res=rc.hbt_res*rc.hbt_rc_scale; p.hbt_cap=rc.hbt_cap*rc.hbt_rc_scale; p.max_candidate_parents=pd_tree.max_candidate_parents; p.max_candidate_hbts=pd_tree.max_candidate_hbts; p.beam_width_3d=pd_tree.beam_width_3d; p.beam_branch_candidates_3d=pd_tree.beam_branch_candidates_3d; p.max_local_hbt_candidates=pd_tree.max_local_hbt_candidates; p.max_hbt_nearest_k=pd_tree.max_hbt_nearest_k; p.verbose=pd_tree.verbose; p.enable_hbt_inner_node_optimization=false; p.dump_candidate_cost_debug=debug.dump_candidate_cost; p.report_cost=report_cost; return p; }
std::string ExperimentConfig::dumpJsonString() const { json j; j["experiment_name"]=experiment_name; j["benchmark"]=benchmark; return j.dump(2); }
