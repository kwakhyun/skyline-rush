from pathlib import Path
import csv,json,statistics,sys
root=Path(__file__).resolve().parents[1]/'Saved/QA'
keys=['GPUMem/LocalUsedMB','GPUMem/SystemUsedMB','GPU/SlateUI','RunnerUI/GameThread/Canvas','RunnerUI/GameThread/UMG','GPU/LumenSceneUpdate','GPU/LumenReflections','GPU/TemporalSuperResolution','GPU/ShadowDepths','GPU/Translucency','Exclusive/RenderThread/Niagara']
def summarize(tag):
 p=root/tag;m=json.loads((p/'metrics.json').read_text())
 rows=[r for r in csv.DictReader((p/'profile.csv').open()) if (r.get('FrameTime') or '').replace('.','',1).isdigit()]
 stats={}
 for k in keys:
  if k not in rows[0]:continue
  values=sorted(float(r[k]) for r in rows if r.get(k))
  stats[k]={'mean':statistics.mean(values),'max':max(values),'p95':values[int(len(values)*.95)]}
 frames=list(csv.DictReader((p/'frames.csv').open()))
 wall=[float(r['wall_ms']) for r in frames]
 result={'profile_frames':len(rows),'metrics':m,'csv_stats':stats,'wall_max_ms':max(wall),'frames_above_16_67':sum(x>1000/60 for x in wall),'passes_60fps_mean_and_p95':m['fps']>=60 and m['p95_wall_ms']<=1000/60}
 (p/'profile-summary.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
 return result
if __name__=='__main__':
 import argparse
 parser=argparse.ArgumentParser(description='Summarize existing Saved/QA profile captures.')
 parser.add_argument('tags',nargs='+',help='One or more capture directory names')
 args=parser.parse_args()
 for tag in args.tags:
  if Path(tag).name!=tag or tag in ['.','..']:parser.error('Use a capture directory name, not a path')
 report={tag:summarize(tag) for tag in args.tags}
 print(json.dumps({tag:{'fps':r['metrics']['fps'],'p95_ms':r['metrics']['p95_wall_ms'],'passes_60fps_mean_and_p95':r['passes_60fps_mean_and_p95']} for tag,r in report.items()},indent=2))
