param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$RunnerRoot=Split-Path -Parent $PSScriptRoot
$Tag='SFX_'+(Get-Date -Format 'yyyyMMdd_HHmmss')
$Log=Join-Path $RunnerRoot ('Saved\QA\'+$Tag+'.log')
$Arguments=@(('"'+$RunnerRoot+'\SkylineRush.uproject"'),'/Game/SkylineRush/Maps/L_SkylineRush','-game','-windowed','-ForceRes','-ResX=1280','-ResY=720','-nosplash','-RunnerQA','-RunnerVisualQA','-RunnerQAFast','-RunnerQAAudio','-seconds=180',('-RunnerQAOutput='+$Tag),('-abslog="'+$Log+'"'))
$Process=Start-Process -FilePath (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe') -ArgumentList $Arguments -WindowStyle Hidden -PassThru
$Process.WaitForExit()
if($Process.ExitCode -ne 0){throw 'SFX gameplay QA failed'}
$Content=Get-Content -LiteralPath $Log -Raw
$Expected=@{pickup='SFX_CrystalPickup';hit='SFX_ObstacleHit';crash='SFX_ObstacleHit';boost='SFX_BoostActivate';fever='SFX_FeverActivate'}
$Counts=[ordered]@{}
foreach($Event in $Expected.Keys){
 $Counts[$Event]=[regex]::Matches($Content,('RUNNER_SFX event='+$Event+' asset='+$Expected[$Event])).Count
 if($Counts[$Event] -lt 1){throw "Gameplay did not exercise expected SFX event: $Event"}
}
if($Content -match 'Fatal error:'){throw 'Fatal gameplay error'}
$Report=[ordered]@{status='passed';method='Automated gameplay with audio enabled; event-to-asset dispatch verification';events=$Counts;log=$Log}
$Report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $RunnerRoot 'Saved\QA\sfx-gameplay.json') -Encoding utf8
$Report | ConvertTo-Json -Depth 4
