#pragma once
#include "Parser.h"
#include "PDTreeRouter.h"
#include <string>
#include <vector>

struct ExperimentConfig {
  struct Output { std::string output_dir="results/default"; std::string summary_report="summary.rpt"; std::string net_info="net_info.txt"; std::string plot_data_dir="plot_data"; std::string plot_dir="plots"; bool enable_plot_generation=true; bool dump_invalid_3d_nets=true; } output;
  struct PDTree { int max_nets=-1,max_candidate_parents=64,max_candidate_hbts=128,beam_width_3d=4,beam_branch_candidates_3d=6,max_local_hbt_candidates=24,max_hbt_nearest_k=16; bool verbose=true; } pd_tree;
  struct CostW { double avg_delay=1,max_delay=0.1,wirelength=1e-5,hbt_count=0.05,stretch=1e-5,hbt_depth=0.01,hbt_stack=0.03,hbt_fanout=0.01,hbt_scarcity=0.02,hbt_position=1e-5,hbt_subtree_wire=1e-5,hbt_subtree_delay_proxy=0.03; } cost_weights;
  struct RC { double source_res=50,default_sink_cap=0.001,hbt_res=5,hbt_cap=0.001,hbt_rc_scale=1,wire_r_scale=1,wire_c_scale=1; } rc;
  struct Debug { bool dump_candidate_cost=false,dump_per_net_detail=true,dump_plot_data=true; } debug;
  std::string experiment_name="default_exp", benchmark="unknown", cost_mode="proposed";
  Parser::InputFiles input;

  static bool loadFromFile(const std::string& path, ExperimentConfig& cfg, std::string& err);
  bool validate(std::string& err) const;
  std::string dumpJsonString() const;
  PDTreeRouter::Params buildRouterParams() const;
};
