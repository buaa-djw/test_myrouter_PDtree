#!/usr/bin/env python3
import argparse, json, os, glob
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
p=argparse.ArgumentParser(); p.add_argument('--root',required=True); a=p.parse_args()
os.makedirs(os.path.join(a.root,'plots'),exist_ok=True)
for f in glob.glob(os.path.join(a.root,'plot_data','*.json')):
  try:
    d=json.load(open(f))
    fig=plt.figure(); ax=fig.add_subplot(111,projection='3d')
    for s in d.get('segments',[]):
      c='red' if s.get('uses_hbt') else ('blue' if s.get('z1',0)>0.5 else 'green')
      ax.plot([s['x1'],s['x2']],[s['y1'],s['y2']],[s['z1'],s['z2']],color=c)
    for pt in d.get('points',[]): ax.scatter(pt['x'],pt['y'],pt['z'],marker='o')
    ax.set_title(f"{d.get('net_name')} wl={d.get('wirelength')} hbt={d.get('hbt_count')} avg={d.get('avg_sink_delay')} max={d.get('max_sink_delay')}")
    ax.set_xlabel('x'); ax.set_ylabel('y'); ax.set_zlabel('z')
    out=os.path.join(a.root,'plots',os.path.basename(f).replace('.json','.png')); plt.savefig(out); plt.close(fig)
  except Exception as e:
    print('warn',f,e)
