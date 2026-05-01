#!/usr/bin/env python3
import argparse
import json
from pathlib import Path
import matplotlib.pyplot as plt


def draw_one(path: Path, out_dir: Path):
    data = json.loads(path.read_text())
    fig = plt.figure(figsize=(8, 6))
    ax = fig.add_subplot(111, projection='3d')
    for seg in data.get('segments', []):
        ax.plot([seg['x1'], seg['x2']], [seg['y1'], seg['y2']], [seg['z1'], seg['z2']], c='r' if seg.get('uses_hbt') else 'b')
    for pt in data.get('points', []):
        ax.scatter([pt['x']], [pt['y']], [pt.get('z', 0.0)], c='k', s=10)
    v = data.get('validation', {})
    ax.set_title(f"{data.get('net_name')} type={'3D' if data.get('is_3d') else '2D'} status={data.get('status')} val={'OK' if v.get('valid', False) else 'INVALID'} cost={data.get('route_cost_total',0):.3f} delay={data.get('avg_sink_delay',0):.3f}/{data.get('max_sink_delay',0):.3f} hbt={data.get('hbt_count',0)}")
    out_dir.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_dir / (path.stem + '.png'), dpi=120)
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    args = ap.parse_args()
    root = Path(args.root)
    for t in ('2d', '3d'):
        for js in (root / 'plot_data' / t).glob('*.json'):
            draw_one(js, root / 'plots' / t)


if __name__ == '__main__':
    main()
