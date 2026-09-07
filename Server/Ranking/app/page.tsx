"use client";
import {useCallback,useEffect,useState} from "react";
import {Trophy,ArrowUpRight,RefreshCw,Flag,Sparkles} from "lucide-react";
import {Button} from "@/components/ui/button";
import {Table,TableBody,TableCell,TableHead,TableHeader,TableRow} from "@/components/ui/table";
type Entry={rank:number;nickname:string;score:number;distance:number};
export default function Page(){
 const [rows,setRows]=useState<Entry[]>([]),[busy,setBusy]=useState(true),[error,setError]=useState(""),[updated,setUpdated]=useState("");
 const refresh=useCallback(async()=>{setBusy(true);setError("");try{const r=await fetch("/api/runner?rules_version=7");if(!r.ok)throw Error();const data=await r.json();setRows(data.entries);setUpdated(new Date().toLocaleTimeString("ko-KR"));}catch{setError("랭킹을 불러오지 못했어요. 다시 시도해 주세요.");}finally{setBusy(false);}},[]);
 useEffect(()=>{void refresh();},[refresh]);
 return <main className="ranking-shell"><header className="brand"><span className="brand-icon"><ArrowUpRight size={26}/></span><span>SKYLINE <b>RUSH</b></span><span className="season">시즌 3</span></header>
 <section className="intro"><div className="eyebrow"><Sparkles size={16}/> 더 멀리, 더 빠르게!</div><h1>하늘 끝까지 달린<br/>우리의 최고기록</h1><p>스카이라인 러시의 온라인 랭킹.<br/>당신의 다음 도전은 몇 위일까요?</p></section>
 <section className="board" aria-label="온라인 랭킹"><div className="board-title"><div><Trophy/><h2>명예의 전당 <span>TOP 10</span></h2></div><Button variant="outline" onClick={refresh} disabled={busy}><RefreshCw size={16} className={busy?"spin":""}/>{busy?"불러오는 중":"새로고침"}</Button></div>
 <p className="board-caption">러너마다 가장 높은 점수 하나를 표시해요. 점수가 같으면 더 멀리 달린 기록이 앞서요.</p>
 {error?<div role="alert" className="empty">{error}</div>:!rows.length?<div className="empty"><Flag size={34}/><h3>{busy?"러너들의 기록을 모으고 있어요":"첫 번째 주인공을 기다리고 있어요"}</h3><p>게임에서 달리기를 마친 뒤 온라인 등록을 눌러 주세요.</p></div>:<Table><TableHeader><TableRow><TableHead>순위</TableHead><TableHead>러너</TableHead><TableHead className="text-right">점수</TableHead><TableHead className="text-right">달린 거리</TableHead></TableRow></TableHeader><TableBody>{rows.map(r=><TableRow key={r.rank}><TableCell><span className={"rank rank-"+r.rank}>{String(r.rank).padStart(2,"0")}</span></TableCell><TableCell className="runner-name">{r.nickname}</TableCell><TableCell className="score">{r.score.toLocaleString("ko-KR")}</TableCell><TableCell className="distance">{r.distance.toLocaleString("ko-KR",{maximumFractionDigits:0})}<small> m</small></TableCell></TableRow>)}</TableBody></Table>}
 <div className="board-footer"><span>서버에서 주행 기록을 확인한 점수</span><span>{updated?updated+" 업데이트":""}</span></div></section>
 <aside className="how"><span>01 <b>달리기 시작</b></span><i>→</i><span>02 <b>나의 최고기록 달성</b></span><i>→</i><span>03 <b>기록실에서 온라인 등록</b></span></aside>
 <footer>닉네임과 최고기록은 온라인 등록을 선택했을 때 공개됩니다.<br/>PC에 저장된 플레이어 정보로 내 기록을 이어가요. · SKYLINE RUSH</footer></main>;
}
