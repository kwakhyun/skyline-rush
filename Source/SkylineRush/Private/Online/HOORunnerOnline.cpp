#include "Gameplay/HOORunnerPawn.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    FString JsonText(const TSharedRef<FJsonObject>& Object)
    {
        FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Object,Writer);return Text;
    }
    TSharedPtr<FJsonObject> ReadJson(const FHttpResponsePtr& Response)
    {
        TSharedPtr<FJsonObject> Object;
        if(Response.IsValid()) {auto Reader=TJsonReaderFactory<>::Create(Response->GetContentAsString());FJsonSerializer::Deserialize(Reader,Object);}
        return Object;
    }
}
void AHOORunnerPawn::RecordInput(const TCHAR* Action)
{
    if(Run.Phase!=EHOORunnerPhase::Running) return;
    if(ReplayInputs.Num()>=10000) {bReplayOverflow=true;return;}
    ReplayInputs.Add(FString::Printf(TEXT("{\"tick\":%d,\"action\":\"%s\"}"),Run.SimulationTicks,Action));
}
FString AHOORunnerPawn::RankingApi() const
{
    FString Url=TEXT("https://skyline-rush-ranking.khyun97.chatgpt.site");
    GConfig->GetString(TEXT("RunnerOnline"),TEXT("ApiBase"),Url,GGameIni);
    if(FParse::Param(FCommandLine::Get(),TEXT("RunnerQA"))) FParse::Value(FCommandLine::Get(),TEXT("RunnerApi="),Url);
    Url.RemoveFromEnd(TEXT("/"));return Url;
}
void AHOORunnerPawn::EnsureOnlineProfile(TFunction<void(bool)> Completion)
{
    if(!OnlinePlayerId.IsEmpty() && !OnlineToken.IsEmpty()) {Completion(true);return;}
    auto Body=MakeShared<FJsonObject>();Body->SetStringField(TEXT("action"),TEXT("register"));Body->SetStringField(TEXT("nickname"),PlayerNickname);
    auto Request=FHttpModule::Get().CreateRequest();Request->SetURL(RankingApi()+TEXT("/api/runner"));
    Request->SetVerb(TEXT("POST"));Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));Request->SetTimeout(12);
    Request->SetContentAsString(JsonText(Body));
    TWeakObjectPtr<AHOORunnerPawn> Weak(this);
    Request->OnProcessRequestComplete().BindLambda([Weak,Completion](FHttpRequestPtr,FHttpResponsePtr Response,bool Ok)
    {
        if(!Weak.IsValid()) return;
        auto Object=ReadJson(Response);
        if(!Ok || !Response.IsValid() || Response->GetResponseCode()!=201 || !Object.IsValid())
        {Weak->RankStatus=TEXT("서버에 연결하지 못했어요. 잠시 뒤 다시 시도해 주세요.");Completion(false);return;}
        FString Id,Token;
        if(!Object->TryGetStringField(TEXT("id"),Id) || !Object->TryGetStringField(TEXT("token"),Token) || Id.IsEmpty() || Token.IsEmpty())
        {Weak->RankStatus=TEXT("서버 응답을 확인할 수 없어요.");Completion(false);return;}
        Weak->OnlinePlayerId=Id;Weak->OnlineToken=Token;
        Weak->SavePreferences();Completion(true);
    });
    if(!Request->ProcessRequest()) {RankStatus=TEXT("인터넷 연결을 확인해 주세요.");Completion(false);}
}
void AHOORunnerPawn::RefreshOnlineRanking()
{
    if(bOnlineBusy) return;
    bOnlineBusy=true;RankStatus=TEXT("순위를 불러오는 중이에요...");
    const FString Board=FParse::Param(FCommandLine::Get(),TEXT("RunnerQA"))?TEXT("qa"):TEXT("main");
    auto Request=FHttpModule::Get().CreateRequest();Request->SetURL(RankingApi()+TEXT("/api/runner?rules_version=7&board=")+Board+TEXT("&player=")+OnlinePlayerId);
    Request->SetVerb(TEXT("GET"));Request->SetTimeout(12);
    TWeakObjectPtr<AHOORunnerPawn> Weak(this);
    Request->OnProcessRequestComplete().BindLambda([Weak](FHttpRequestPtr,FHttpResponsePtr Response,bool Ok)
    {
        if(!Weak.IsValid()) return;Weak->bOnlineBusy=false;
        auto Object=ReadJson(Response);const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
        if(!Ok || !Response.IsValid() || Response->GetResponseCode()!=200 || !Object.IsValid() || !Object->TryGetArrayField(TEXT("entries"),Rows))
        {Weak->RankStatus=TEXT("연결이 잠시 끊겼어요. 내 기록은 안전하게 저장돼요.");return;}
        Weak->OnlineRanks.Reset();
        for(const auto& Value:*Rows)
        {
            if(!Value.IsValid() || Value->Type!=EJson::Object)continue;
            const auto Row=Value->AsObject();if(!Row.IsValid()) continue;
            FHOORunnerRankEntry Entry;double Rank=0,Score=0,Distance=0;
            if(!Row->TryGetStringField(TEXT("nickname"),Entry.Nickname) || !Row->TryGetNumberField(TEXT("rank"),Rank) ||
                !Row->TryGetNumberField(TEXT("score"),Score) || !Row->TryGetNumberField(TEXT("distance"),Distance)) continue;
            if(!FMath::IsFinite(Rank) || Rank<1 || Rank>MAX_int32 || !FMath::IsFinite(Score) || Score<0 || Score>MAX_int32 ||
                !FMath::IsFinite(Distance) || Distance<0 || Distance>MAX_flt || Entry.Nickname.IsEmpty() || Entry.Nickname.Len()>12)continue;
            Entry.Rank=Rank;Entry.Score=Score;Entry.Distance=Distance;Weak->OnlineRanks.Add(Entry);
            if(Weak->OnlineRanks.Num()>=10) break;
        }
        double Mine=0;
        Weak->RankStatus=Object->TryGetNumberField(TEXT("my_rank"),Mine) && Mine>0?
            FString::Printf(TEXT("현재 내 순위는 %d위예요!"),static_cast<int32>(Mine)):
            Weak->OnlineRanks.IsEmpty()?TEXT("첫 번째 주인공이 되어 보세요!"):TEXT("최신 순위를 불러왔어요.");
    });
    if(!Request->ProcessRequest()) {bOnlineBusy=false;RankStatus=TEXT("인터넷 연결을 확인해 주세요.");}
}
void AHOORunnerPawn::SubmitBestRun()
{
    if(bOnlineBusy) return;
    if(Run.Phase==EHOORunnerPhase::Crashed) SaveRecord();
    if(LocalScores.IsEmpty()) {RankStatus=TEXT("먼저 한 번 달려 볼까요?");return;}
    const auto* Candidate=HOORunnerRecords::BestSubmittable(LocalScores);
    if(!Candidate) {RankStatus=TEXT("등록 가능한 기록이 없어요. 20분 이내의 새 주행으로 도전해 주세요.");return;}
    const auto Entry=*Candidate;
    bOnlineBusy=true;RankStatus=TEXT("내 최고기록을 확인하고 있어요...");
    TWeakObjectPtr<AHOORunnerPawn> Weak(this);
    EnsureOnlineProfile([Weak,Entry](bool Ready)
    {
        if(!Weak.IsValid()) return;
        if(!Ready) {Weak->bOnlineBusy=false;return;}
        TArray<TSharedPtr<FJsonValue>> Events;auto Reader=TJsonReaderFactory<>::Create(Entry.ReplayJson);
        if(!FJsonSerializer::Deserialize(Reader,Events)) {Weak->bOnlineBusy=false;Weak->RankStatus=TEXT("입력 기록을 읽을 수 없어요.");return;}
        auto Body=MakeShared<FJsonObject>();Body->SetStringField(TEXT("action"),TEXT("submit"));
        Body->SetStringField(TEXT("run_id"),Entry.RunId);Body->SetStringField(TEXT("nickname"),Weak->PlayerNickname);
        Body->SetNumberField(TEXT("seed"),Entry.Seed);Body->SetNumberField(TEXT("ticks"),Entry.Ticks);
        Body->SetNumberField(TEXT("client_score"),Entry.Score);Body->SetNumberField(TEXT("rules_version"),7);
        Body->SetStringField(TEXT("board"),FParse::Param(FCommandLine::Get(),TEXT("RunnerQA"))?TEXT("qa"):TEXT("main"));
        Body->SetArrayField(TEXT("events"),Events);
        auto Request=FHttpModule::Get().CreateRequest();Request->SetURL(Weak->RankingApi()+TEXT("/api/runner"));
        Request->SetVerb(TEXT("POST"));Request->SetTimeout(25);Request->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
        Request->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+Weak->OnlineToken);Request->SetContentAsString(JsonText(Body));
        Request->OnProcessRequestComplete().BindLambda([Weak](FHttpRequestPtr,FHttpResponsePtr Response,bool Ok)
        {
            if(!Weak.IsValid()) return;Weak->bOnlineBusy=false;
            auto Object=ReadJson(Response);
            if(Ok && Response.IsValid() && Response->GetResponseCode()==200 && Object.IsValid())
            {Weak->RankStatus=TEXT("최고기록 등록 완료!");Weak->RefreshOnlineRanking();}
            else if(Response.IsValid() && Response->GetResponseCode()==401)
            {Weak->OnlineToken.Empty();Weak->OnlinePlayerId.Empty();Weak->SavePreferences();Weak->RankStatus=TEXT("연결 정보를 갱신했어요. 등록을 다시 눌러 주세요.");}
            else if(Response.IsValid() && Response->GetResponseCode()==422)
                Weak->RankStatus=TEXT("기록 검증이 일치하지 않아요. 새 주행으로 다시 도전해 주세요.");
            else Weak->RankStatus=TEXT("등록하지 못했어요. 내 기록은 보관 중이니 다시 눌러 주세요.");
        });
        if(!Request->ProcessRequest()) {Weak->bOnlineBusy=false;Weak->RankStatus=TEXT("인터넷 연결을 확인해 주세요.");}
    });
}
bool AHOORunnerPawn::SetPlayerNickname(const FString& Name)
{
    FString Clean=Name.TrimStartAndEnd();
    if(Clean.Len()<2 || Clean.Len()>12) {RankStatus=TEXT("닉네임은 2~12자로 입력해 주세요.");return false;}
    for(TCHAR C:Clean) if(!FChar::IsAlnum(C) && C!=TEXT(' ') && C!=TEXT('_') && C!=TEXT('-'))
    {RankStatus=TEXT("닉네임에는 글자·숫자·공백·_·-를 사용할 수 있어요.");return false;}
    PlayerNickname=Clean;SavePreferences();RankStatus=TEXT("닉네임을 저장했어요.");return true;
}
FString AHOORunnerPawn::GetRankingSnapshot() const
{
    auto Object=MakeShared<FJsonObject>();Object->SetStringField(TEXT("nickname"),PlayerNickname);
    Object->SetNumberField(TEXT("local_count"),LocalScores.Num());Object->SetNumberField(TEXT("online_count"),OnlineRanks.Num());
    Object->SetBoolField(TEXT("busy"),bOnlineBusy);Object->SetStringField(TEXT("status"),RankStatus);
    Object->SetNumberField(TEXT("ticks"),Run.SimulationTicks);Object->SetNumberField(TEXT("inputs"),ReplayInputs.Num());
    return JsonText(Object);
}
