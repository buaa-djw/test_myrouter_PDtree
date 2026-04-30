#include "PlotDataWriter.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>
using json=nlohmann::json;
std::string safeNetName(const std::string& n){std::string s=n; for(char&c:s) if(!(isalnum((unsigned char)c)||c=='_'||c=='-')) c='_'; return s;}
static double z(DieId d){return d==DieId::kTop?1.0:0.0;}
void write3DNetPlotData(const std::string& dir,const NetRouteResult& r){std::filesystem::create_directories(dir); json j; j["net_name"]=r.net_name; j["avg_sink_delay"]=r.delay_summary.avg_sink_delay; j["max_sink_delay"]=r.delay_summary.max_sink_delay; double wl=0; int hc=0; for(auto&s:r.segments){wl+=abs(s.p1.x-s.p2.x)+abs(s.p1.y-s.p2.y); if(s.uses_hbt) hc++;} j["wirelength"]=wl; j["hbt_count"]=hc; j["tree_radius"]=0; j["points"]=json::array(); for(size_t i=0;i<r.tree_nodes.size();++i){auto&n=r.tree_nodes[i]; j["points"].push_back({{"id",(int)i},{"type",n.node_type==TreeNodeState::NodeType::kHBT?"hbt":"steiner"},{"name",std::to_string(n.pin_index)},{"x",n.point.x},{"y",n.point.y},{"z",z(n.point.die)}});} j["segments"]=json::array(); for(auto&s:r.segments){j["segments"].push_back({{"x1",s.p1.x},{"y1",s.p1.y},{"z1",z(s.p1.die)},{"x2",s.p2.x},{"y2",s.p2.y},{"z2",z(s.p2.die)},{"uses_hbt",s.uses_hbt},{"hbt_id",s.hbt_id}});} std::ofstream(dir+"/"+safeNetName(r.net_name)+".json")<<j.dump(2);
}
