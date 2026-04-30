# PDTreeRouter 实验仓库
用于 baseline vs proposed 以及 HBT RC sensitivity sweep.

## 依赖
Ubuntu 22.04:
- g++ cmake
- python3 python3-matplotlib python3-numpy
- OpenROAD/OpenDB

## 编译
`mkdir -p build && cd build && cmake .. -DOPENROAD_HOME=/path -DOPENDB_INCLUDE_DIR=/path/include -DOPENDB_LIB=/path/libodb.so && make -j`

## 运行
- `./build/pdtree_experiment --config configs/baseline.json`
- `./build/pdtree_experiment --config configs/proposed_default.json`
- `./build/pdtree_experiment --config configs/proposed_high_hbt_rc.json`
- 一键: `bash scripts/run_gcd_experiments.sh`

## 输出
- `results/<exp>/summary.rpt`
- `results/<exp>/net_info.txt`
- `results/<exp>/plot_data/*.json`
- `results/<exp>/plots/*.png`

若无3D net, plots可为空。找不到LEF/DEF/OpenDB请检查config和CMake变量。
