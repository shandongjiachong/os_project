"""Verify the unpacked B handoff without changing any file."""
from pathlib import Path
import hashlib, json
root=Path(__file__).resolve().parents[3]
manifest=root/'report/B-best-fit/evidence/deliverable-manifest.json'
items=json.loads(manifest.read_text(encoding='utf-8'))['files']
errors=[]
for item in items:
    p=(root/item['path']).resolve()
    if not p.is_relative_to(root):
        errors.append({'path':item['path'],'reason':'outside package'}); continue
    if not p.is_file():
        errors.append({'path':item['path'],'reason':'missing'}); continue
    data=p.read_bytes()
    if len(data)!=item['bytes'] or hashlib.sha256(data).hexdigest()!=item['sha256']:
        errors.append({'path':item['path'],'reason':'size or hash mismatch'})
print(json.dumps({'filesChecked':len(items),'passed':not errors,'errors':errors},ensure_ascii=False,indent=2))
raise SystemExit(1 if errors else 0)
