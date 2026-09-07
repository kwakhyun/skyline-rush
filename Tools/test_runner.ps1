param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Build)
$ErrorActionPreference='Stop'
$RunnerRoot=Split-Path -Parent $PSScriptRoot
$RunnerProject=Join-Path $RunnerRoot 'SkylineRush.uproject'
if($Build){
 & (Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat') SkylineRushEditor Win64 Development $RunnerProject -WaitMutex -NoHotReload
 if($LASTEXITCODE -ne 0){throw 'Build failed'}
}
$RunnerReport=Join-Path $RunnerRoot 'Saved\QA\Tests'
& (Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe') $RunnerProject -unattended -NullRHI -nosplash '-ExecCmds=Automation RunTests HOO.Runner' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$RunnerReport" "-abslog=$RunnerRoot\Saved\Logs\RunnerTests.log"
if($LASTEXITCODE -ne 0){throw 'Unreal runner tests failed'}
$RunnerResults=Get-Content -LiteralPath (Join-Path $RunnerReport 'index.json') -Raw | ConvertFrom-Json
if($RunnerResults.failed -gt 0 -or $RunnerResults.succeeded -lt 13){throw 'Runner test report incomplete or failed'}
& node (Join-Path $RunnerRoot 'Tools\verify_runner_replay.mjs')
if($LASTEXITCODE -ne 0){throw 'Offline replay parity failed'}
Write-Output 'SKYLINE RUSH: 13 Unreal tests and offline ranking parity passed.'
