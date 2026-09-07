param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$RunnerRoot=Split-Path -Parent $PSScriptRoot
$Tag='Specials_'+(Get-Date -Format yyyyMMdd_HHmmss)
$Exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
$Args=@(('"'+$RunnerRoot+'/SkylineRush.uproject"'),'/Game/SkylineRush/Maps/L_SkylineRush','-game','-windowed','-ForceRes','-ResX=1280','-ResY=720','-NoSound','-RunnerQA','-RunnerVisualQA','-RunnerQASpecials',('-RunnerQAOutput='+$Tag),'-unattended','-nosplash',('-abslog="'+$RunnerRoot+'/Saved/QA/'+$Tag+'.log"'))
$Process=Start-Process -FilePath $Exe -ArgumentList $Args -WindowStyle Hidden -PassThru
if(!$Process.WaitForExit(240000)){Stop-Process -Id $Process.Id;throw 'Special section visual QA timed out'}
if($Process.ExitCode -ne 0){throw 'Special section visual QA failed'}
$Folder=Join-Path $RunnerRoot ('Saved/QA/'+$Tag)
foreach($Name in 'forest','city','atrium','jungle','dimension'){
 if(!(Test-Path -LiteralPath (Join-Path $Folder ('special-'+$Name+'.png')))){throw ('Missing screenshot: '+$Name)}
}
if(!(Select-String -LiteralPath (Join-Path $RunnerRoot ('Saved/QA/'+$Tag+'.log')) -Pattern 'SPECIAL_REPLAY_QA passed=1')){throw 'Real gameplay replay did not pass'}
if(!(Test-Path (Join-Path $Folder 'special-results.png'))){throw 'Missing result breakdown screenshot'}
Write-Output $Folder
