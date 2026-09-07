param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [switch]$Build,
    [switch]$Windowed
)
$ErrorActionPreference = 'Stop'
$RunnerProjectRoot = Split-Path -Parent $PSScriptRoot
$RunnerProject = Join-Path $RunnerProjectRoot 'SkylineRush.uproject'
$RunnerEditor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path -LiteralPath $RunnerEditor)) { throw "Unreal Editor not found: $RunnerEditor" }
if ($Build) {
    $RunnerBuild = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
    & $RunnerBuild SkylineRushEditor Win64 Development $RunnerProject -WaitMutex -NoHotReload
    if ($LASTEXITCODE -ne 0) { throw 'Runner build failed.' }
}
$RunnerLog = Join-Path $RunnerProjectRoot 'Saved\Logs\RunnerManualPlay.log'
$RunnerArguments = @(
    ('"'+$RunnerProject+'"'),
    '/Game/SkylineRush/Maps/L_SkylineRush',
    '-game', '-nosplash',
    ('-abslog="'+$RunnerLog+'"')
)
if ($Windowed) { $RunnerArguments += @('-windowed','-ResX=1600','-ResY=900') }
else { $RunnerArguments += @('-fullscreen','-ResX=1920','-ResY=1080') }
Start-Process -FilePath $RunnerEditor -ArgumentList $RunnerArguments -WorkingDirectory $RunnerProjectRoot -WindowStyle Normal
Write-Output 'SKYLINE RUSH: SPACE start/jump, A/D lanes, S slide, ESC pause, F1 settings, F2 records/ranking, R new course. Menus: arrows + ENTER or mouse.'
