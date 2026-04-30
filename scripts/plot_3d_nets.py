#!/usr/bin/env python3
import argparse, json
from pathlib import Path
import matplotlib.pyplot as plt


def load_jsons(plot_dir):
    return sorted(plot_dir.glob('*.json'))


def draw_net(data, out_path):
    fig = plt.figure(figsize=(10, 8))
    ax = fig.add_subplot(111, projection='3d')
    name = data.get('net_name', 'unknown')
    valid = data.get('validation', {}).get('valid', True)

    for s in data.get('segments', []):
        x = [s['x1'], s['x2']]; y = [s['y1'], s['y2']]; z = [s['z1'], s['z2']]
        if (not s.get('uses_hbt', False)) and z[0] != z[1]:
            ax.plot(x, y, z, 'm--', linewidth=2, label='invalid non-HBT cross-die')
            print(f"[WARN] invalid non-HBT cross-die segment in {name}")
        elif s.get('uses_hbt', False) and (x[0] != x[1] or y[0] != y[1] or z[0] == z[1]):
            ax.plot(x, y, z, 'r--', linewidth=2, label='invalid HBT vertical')
            print(f"[WARN] invalid HBT segment in {name}")
        elif s.get('uses_hbt', False):
            ax.plot(x, y, z, 'g-', linewidth=2, label='HBT vertical')
        elif z[0] > 0.5:
            ax.plot(x, y, z, 'b-', linewidth=1, label='top wire')
        else:
            ax.plot(x, y, z, 'c-', linewidth=1, label='bottom wire')

    for p in data.get('points', []):
        t = p.get('type', 'steiner')
        if t == 'source':
            ax.scatter(p['x'], p['y'], p['z'], marker='*', s=120, c='gold', label='source')
        elif t == 'sink':
            ax.scatter(p['x'], p['y'], p['z'], marker='o', s=30, c='black', label='sink')
        elif t == 'hbt':
            ax.scatter(p['x'], p['y'], p['z'], marker='^', s=80, c='green', label='HBT')
        else:
            ax.scatter(p['x'], p['y'], p['z'], marker='x', s=20, c='gray', label='steiner')

    title = f"{name}\nvalidation={'OK' if valid else 'INVALID'}"
    ax.set_title(title)
    handles, labels = ax.get_legend_handles_labels()
    uniq = dict(zip(labels, handles))
    ax.legend(uniq.values(), uniq.keys(), loc='upper left')
    plt.savefig(out_path, bbox_inches='tight')
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    args = ap.parse_args()
    root = Path(args.root)
    plot_dir = root / 'plot_data'
    out_dir = root / 'plots'
    out_dir.mkdir(parents=True, exist_ok=True)
    for f in load_jsons(plot_dir):
        data = json.loads(f.read_text())
        suffix = '_INVALID' if not data.get('validation', {}).get('valid', True) else ''
        draw_net(data, out_dir / f"{f.stem}{suffix}.png")


if __name__ == '__main__':
    main()
