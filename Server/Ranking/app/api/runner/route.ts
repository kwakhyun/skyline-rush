import {env} from "cloudflare:workers";
import {validateReplay} from "../../../lib/simulation.mjs";
const db=()=> (env as unknown as {DB:D1Database}).DB;
const json=(data:unknown,status=200)=>Response.json(data,{status,headers:{"Cache-Control":"no-store","X-Content-Type-Options":"nosniff"}});
const digest=async(s:string)=>Array.from(new Uint8Array(await crypto.subtle.digest("SHA-256",new TextEncoder().encode(s))),b=>b.toString(16).padStart(2,"0")).join("");
const nickname=(n:unknown)=>typeof n==="string"&&/^[\p{L}\p{N} _-]{2,12}$/u.test(n.normalize("NFC").trim())?n.normalize("NFC").trim():null;
const board=(b:unknown,v:unknown)=>(b==="qa"?"qa-v":"main-v")+(v===7?7:6);
const uuid=(s:unknown)=>typeof s==="string"&&/^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$/i.test(s);
async function limit(req:Request,scope:string,max:number){
 const now=Math.floor(Date.now()/1000),ip=req.headers.get("CF-Connecting-IP")||"local";
 const key=scope+":"+await digest(ip)+":"+Math.floor(now/60);
 const row=await db().prepare("INSERT INTO request_limits(key,hits,expires) VALUES(?,1,?) ON CONFLICT(key) DO UPDATE SET hits=hits+1 WHERE hits<? RETURNING hits").bind(key,now+120,max).first();
 if(Math.random()<.05)await db().prepare("DELETE FROM request_limits WHERE expires<?").bind(now).run();
 return !!row;
}
const ranked="WITH personal AS (SELECT *,ROW_NUMBER() OVER(PARTITION BY player_id ORDER BY score DESC,distance DESC,created_at ASC,id ASC) AS personal_rank FROM runs WHERE board=?), ranked AS (SELECT p.id AS player_id,p.nickname,r.score,r.distance,ROW_NUMBER() OVER(ORDER BY r.score DESC,r.distance DESC,r.created_at ASC,r.id ASC) AS rank FROM personal r JOIN players p ON p.id=r.player_id WHERE personal_rank=1) ";
export async function GET(req:Request){
 try{
  const url=new URL(req.url),version=url.searchParams.get("rules_version")==="7"?7:6,b=board(url.searchParams.get("board"),version);
  const entries=await db().prepare(ranked+"SELECT rank,nickname,score,distance FROM ranked WHERE rank<=10 ORDER BY rank").bind(b).all();
  const id=url.searchParams.get("player");
  const mine=uuid(id)?await db().prepare(ranked+"SELECT rank FROM ranked WHERE player_id=?").bind(b,id).first<{rank:number}>():null;
  return json({season:version===7?"시즌 3 · 위험과 보상":"시즌 2 · 세 번의 도전",rules_version:version,entries:entries.results,my_rank:mine?.rank??null});
 }catch{return json({error:"랭킹을 불러오지 못했어요. 잠시 후 다시 시도해 주세요."},503);}
}
export async function POST(req:Request){
 try{
  if(Number(req.headers.get("Content-Length"))>1200000)return json({error:"기록이 너무 큽니다."},413);
  const raw=await req.text();if(raw.length>1200000)return json({error:"기록이 너무 큽니다."},413);
  let body;try{body=JSON.parse(raw);}catch{return json({error:"잘못된 요청입니다."},400);}
  if(!body||typeof body!=="object")return json({error:"잘못된 요청입니다."},400);
  if(body.action==="register"){
   const name=nickname(body.nickname);if(!name)return json({error:"닉네임은 한글·영문·숫자 2~12자로 입력해 주세요."},400);
   if(!await limit(req,"register",8))return json({error:"잠시 후 다시 시도해 주세요."},429);
   const id=crypto.randomUUID(),token=crypto.randomUUID()+crypto.randomUUID();
   await db().prepare("INSERT INTO players(id,token_hash,nickname,created_at) VALUES(?,?,?,?)").bind(id,await digest(token),name,Date.now()).run();
   return json({id,token},201);
  }
  if(body.action!=="submit")return json({error:"지원하지 않는 요청입니다."},400);
  const auth=req.headers.get("Authorization")||"";
  if(!auth.startsWith("Bearer ")||auth.length>200)return json({error:"플레이어 인증이 필요합니다."},401);
  const player=await db().prepare("SELECT id FROM players WHERE token_hash=?").bind(await digest(auth.slice(7))).first<{id:string}>();
  if(!player)return json({error:"플레이어 인증을 다시 해주세요."},401);
  if(!await limit(req,"submit",20))return json({error:"잠시 후 다시 등록해 주세요."},429);
  if(!uuid(body.run_id))return json({error:"잘못된 기록 번호입니다."},400);
  const name=nickname(body.nickname);if(!name)return json({error:"닉네임을 확인해 주세요."},400);
  const hash=await digest(JSON.stringify([body.seed,body.ticks,body.events,body.rules_version,board(body.board,body.rules_version)]));
  const existing=await db().prepare("SELECT player_id,replay_hash,score,distance FROM runs WHERE id=?").bind(body.run_id).first<{player_id:string,replay_hash:string,score:number,distance:number}>();
  if(existing)return existing.player_id===player.id&&existing.replay_hash===hash?json({verified:true,duplicate:true,score:existing.score,distance:existing.distance}):json({error:"다른 기록과 번호가 겹칩니다."},409);
  let result;
  try{result=validateReplay(body);}catch{return json({error:"주행 기록을 검증할 수 없어요."},422);}
  if(!Number.isInteger(body.client_score)||Math.abs(body.client_score-result.score)>1||result.distance<1)return json({error:"점수와 주행 기록이 일치하지 않아요."},422);
  await db().batch([
   db().prepare("UPDATE players SET nickname=? WHERE id=?").bind(name,player.id),
   db().prepare("INSERT INTO runs(id,player_id,board,score,distance,ticks,seed,replay_hash,created_at) VALUES(?,?,?,?,?,?,?,?,?)").bind(body.run_id,player.id,board(body.board,body.rules_version),result.score,result.distance,body.ticks,body.seed,hash,Date.now())
  ]);
  return json({verified:true,score:result.score,distance:result.distance});
 }catch{return json({error:"서버에 연결하지 못했어요. 기록은 게임에 보관됩니다."},503);}
}
