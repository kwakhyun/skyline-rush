param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$RunnerRoot=Split-Path -Parent $PSScriptRoot
$ReportRoot=Join-Path $RunnerRoot ('Saved\QA\Music-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $ReportRoot | Out-Null
$MusicLog=Join-Path $ReportRoot 'music.log'
& (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') (Join-Path $RunnerRoot 'SkylineRush.uproject') '/Game/SkylineRush/Maps/L_SkylineRush' -game -NullRHI -unattended -nosplash -RunnerQA -RunnerMusicQATailSeconds=2 -seconds=16 "-abslog=$MusicLog"
if($LASTEXITCODE -ne 0){throw 'Runner music runtime failed'}
$MusicText=Get-Content -LiteralPath $MusicLog -Raw
$TrackStarts=@([regex]::Matches($MusicText,'RUNNER_MUSIC_STARTED track=(\d+)') | ForEach-Object { [int]$_.Groups[1].Value })
if($TrackStarts.Count -lt 3){throw 'Music did not finish and alternate twice. Check audio device availability and the log.'}
for($Index=0;$Index -lt $TrackStarts.Count;$Index++){
    if($TrackStarts[$Index] -ne (1+$Index%2)){throw 'Music sequence is not alternating'}
}
if($MusicText -match 'RUNNER_MUSIC_UNAVAILABLE|Fatal error:'){throw 'Music runtime reported an error'}
$Report=[ordered]@{status='passed';method='Real audio completion; seek to last two seconds of each full track';tracks=$TrackStarts;log=$MusicLog}
$Report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $ReportRoot 'result.json') -Encoding utf8
$Report | ConvertTo-Json -Depth 4
