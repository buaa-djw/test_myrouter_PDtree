#pragma once

#include "PDTreeRouter.h"

#include <string>

// Convert a net name to a filesystem-safe file name.
// Example:
//   ctrl$a_mux_sel[1] -> ctrl_a_mux_sel_1_
std::string safeNetName(const std::string& name);

// Dump one 3D net routing result into plot_data/<safe_net_name>.json.
//
// This function is also allowed to dump invalid 3D nets. Invalid nets are
// useful during debugging because their JSON/PNG output helps locate
// topology errors such as HBT node/segment mismatch or non-HBT cross-die
// segments.
void write3DNetPlotData(const std::string& dir, const NetRouteResult& result);