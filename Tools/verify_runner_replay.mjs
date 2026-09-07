// Offline parity against fixtures exported by the latest HOO.Runner C++ tests.
// This tool makes no HTTP requests and never writes to any leaderboard.
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {validateReplay} from '../Server/Ranking/lib/simulation.mjs';
const input=new URL('../Saved/QA/Replay/ReplayFixtures.json',import.meta.url);
const fixtures=JSON.parse(fs.readFileSync(input,'utf8').replace(/^\uFEFF/,''));
const results=[];
for(const fixture of fixtures){
  const result=validateReplay(fixture);
  assert.equal(result.score,fixture.client_score);
  assert.ok(Math.abs(result.distance-fixture.distance)<.00001);
  for(const key of ['shards','fevers','launches',...('riskScore' in fixture?['riskScore','styleScore','risks','nearMisses']:[]),...('lives' in fixture?['lives','hits']:[])])
    assert.equal(result[key],fixture[key]);
  results.push({seed:fixture.seed,ticks:fixture.ticks,score:result.score,distance:result.distance,passed:true});
}
assert.throws(()=>validateReplay({...fixtures[0],ticks:144001}));
assert.throws(()=>validateReplay({...fixtures[0],events:[{tick:-1,action:'l'}]}));
const report={status:'passed',http_requests:0,rules_version:7,results};
const out=new URL('../Saved/QA/Replay/parity.json',import.meta.url);
fs.writeFileSync(out,JSON.stringify(report,null,2));
console.log(JSON.stringify(report,null,2));
