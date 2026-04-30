#include "EDCompute.h"
#include "ExperimentConfig.h"
#include "ExperimentSummaryWriter.h"
#include "Grid.h"
#include "NetInfoWriter.h"
#include "Parser.h"
#include "PDTreeRouter.h"
#include "PlotDataWriter.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <cstdlib>

int main(int argc,char**argv){std::string cfg_path; for(int i=1;i<argc;++i){std::string a=argv[i]; if(a=="--config"&&i+1<argc) cfg_path=argv[++i];}
 if(cfg_path.empty()){std::cerr<<"Usage: pdtree_experiment --config <json>\n"; return 1;}
 auto t0=std::chrono::steady_clock::now(); ExperimentConfig cfg; std::string err; if(!ExperimentConfig::loadFromFile(cfg_path,cfg,err)){std::cerr<<err<<"\n"; return 2;} if(!cfg.validate(err)){std::cerr<<err<<"\n"; return 3;} std::cout<<"Effective config:\n"<<cfg.dumpJsonString()<<"\n";
 Parser p; if(!p.readDesign(cfg.input)){std::cerr<<"readDesign failed\n"; return 4;} RouterDB db; if(!p.buildRouteDB(db)){std::cerr<<"buildRouteDB failed\n"; return 5;}
 db.hbt.has_parasitic=true; db.hbt.parasitic_res=cfg.rc.hbt_res*cfg.rc.hbt_rc_scale; db.hbt.parasitic_cap=cfg.rc.hbt_cap*cfg.rc.hbt_rc_scale;
 HybridGrid grid; GridBuildOptions opt; opt.hbt_res=db.hbt.parasitic_res; opt.hbt_cap=db.hbt.parasitic_cap; opt.top_unit_res=db.computeEffectiveTopUnitRes()*cfg.rc.wire_r_scale; opt.top_unit_cap=db.computeEffectiveTopUnitCap()*cfg.rc.wire_c_scale; opt.bottom_unit_res=db.computeEffectiveBottomUnitRes()*cfg.rc.wire_r_scale; opt.bottom_unit_cap=db.computeEffectiveBottomUnitCap()*cfg.rc.wire_c_scale; if(!grid.build(db,opt)){std::cerr<<"grid build failed\n"; return 6;}
 PDTreeRouter router(db,grid,cfg.buildRouterParams()); auto results=router.routeSignalNets(cfg.pd_tree.max_nets,nullptr);
 EDCompute ed(db, EDCompute::Params{cfg.rc.default_sink_cap,cfg.rc.source_res,cfg.rc.hbt_res*cfg.rc.hbt_rc_scale,cfg.rc.hbt_cap*cfg.rc.hbt_rc_scale,false});
 for(size_t i=0;i<results.size();++i){auto& r=results[i]; const Net& n=db.nets[i]; if(r.success) ed.annotateNetDelay(n,r);} std::filesystem::create_directories(cfg.output.output_dir); std::string plot_data=cfg.output.output_dir+"/"+cfg.output.plot_data_dir; std::filesystem::create_directories(plot_data); std::filesystem::create_directories(cfg.output.output_dir+"/"+cfg.output.plot_dir);
 for(auto&r:results) if(r.success && r.is_3d && cfg.debug.dump_plot_data) write3DNetPlotData(plot_data,r);
 writeNetInfo(cfg.output.output_dir+"/"+cfg.output.net_info,db,results);
 auto t1=std::chrono::steady_clock::now(); double sec=std::chrono::duration<double>(t1-t0).count(); writeExperimentSummary(cfg.output.output_dir+"/"+cfg.output.summary_report,cfg,db,results,"start","end",sec);
 if(cfg.output.enable_plot_generation){std::string cmd="python3 scripts/plot_3d_nets.py --root "+cfg.output.output_dir; std::system(cmd.c_str());}
 return 0; }
