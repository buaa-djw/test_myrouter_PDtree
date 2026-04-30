#pragma once
#include "PDTreeRouter.h"
#include <string>
void write3DNetPlotData(const std::string& dir,const NetRouteResult& result);
std::string safeNetName(const std::string& n);
