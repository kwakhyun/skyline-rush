import fs from "node:fs";
import assert from "node:assert/strict";
import {validateReplay} from "../lib/simulation.mjs";
const fixtures=JSON.parse(fs.readFileSync(new URL("fixtures.json",import.meta.url),"utf8"));
const checks=[];
for(const a of fixtures){const b=validateReplay(a);assert.equal(b.score,a.client_score);assert.ok(Math.abs(b.distance-a.distance)<.00001);for(const k of ["shards","fevers","launches",...("lives" in a?["lives","hits"]:[])])assert.equal(b[k],a[k]);}
checks.push("C++ and server replay parity across 3.5 km x 2 seeds plus three-life death replay");
assert.throws(()=>validateReplay({...fixtures[0],ticks:144001}));
assert.throws(()=>validateReplay({...fixtures[0],events:[{tick:-1,action:"l"}]}));
checks.push("Replay limits and invalid input validation");
const base=process.env.RUNNER_TEST_API||"http://127.0.0.1:8787";
const call=async(body,token)=>{const r=await fetch(base+"/api/runner",{method:"POST",headers:{"Content-Type":"application/json",...(token?{Authorization:"Bearer "+token}:{})},body:JSON.stringify(body)});return {status:r.status,body:await r.json()};};
const before=await (await fetch(base+"/api/runner")).json();
assert.ok(Array.isArray(before.entries),JSON.stringify(before));
assert.equal((await call({action:"register",nickname:"!"})).status,400);
assert.equal((await call({action:"submit"})).status,401);
const profiles=[];
for(let i=0;i<fixtures.length;i++){
 const a=await call({action:"register",nickname:"QA러너"+i});assert.equal(a.status,201,JSON.stringify(a));profiles.push(a.body);
 const input={...fixtures[i],action:"submit",run_id:crypto.randomUUID(),nickname:"QA러너"+i,board:"qa"};
 assert.equal((await call({...input,client_score:input.client_score+100},a.body.token)).status,422);
 const submitted=await call(input,a.body.token);assert.equal(submitted.status,200,JSON.stringify(submitted));assert.equal(submitted.body.verified,true);
 const duplicate=await call(input,a.body.token);assert.equal(duplicate.status,200);assert.equal(duplicate.body.duplicate,true);
 assert.equal((await call({...input,ticks:input.ticks-1},a.body.token)).status,409);
}
checks.push("Registration, bearer authentication, tampered score rejection, verified submissions, idempotency");
const board=await(await fetch(base+"/api/runner?board=qa&player="+profiles[0].id)).json();
assert.ok(board.entries.length>=2);assert.ok(board.my_rank>=1);
for(let i=1;i<board.entries.length;i++)assert.ok(board.entries[i-1].score>=board.entries[i].score);
const after=await(await fetch(base+"/api/runner")).json();assert.deepEqual(after.entries,before.entries);
checks.push("Ranking order, personal rank, public and QA board isolation");
console.log(JSON.stringify({status:"passed",endpoint:base,checks,qa_entries:board.entries.length},null,2));
