param(
 [ValidateSet('Profile','Fast','Capture','Video','Debug','Review')][string]$Mode='Profile',
 [string]$Tag=('QA_'+(Get-Date -Format 'yyyyMMdd_HHmmss')),
 [int]$Width=1920,[int]$Height=1080,
 [string]$Engine='C:\Program Files\Epic Games\UE_5.8',
 [switch]$Windowed=$true,[switch]$ShowGame,[switch]$Wait
)
$ErrorActionPreference='Stop'
$projectRoot=(Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if($Tag -notmatch '^[A-Za-z0-9_-]+$'){throw 'Tag must contain only letters, numbers, underscore or dash.'}
$projectFile=Join-Path $projectRoot 'SkylineRush.uproject'
$exe=Join-Path $Engine 'Engine\Binaries\Win64\UnrealEditor.exe'
$out=Join-Path $projectRoot ('Saved\QA\'+$Tag)
if(Test-Path -LiteralPath $out){throw 'Output already exists. Choose a new tag to preserve evidence.'}
$log=Join-Path $projectRoot ('Saved\Logs\Visual_'+$Tag+'.log')
$argsText='"'+$projectFile+'" /Game/SkylineRush/Maps/L_SkylineRush -game -ForceRes -nosplash -NoSound -RunnerQA -RunnerVisualQA -ResX='+$Width+' -ResY='+$Height+' -RunnerQAOutput='+$Tag+' -abslog="'+$log+'"'
if($Windowed){$argsText+=' -windowed'}else{$argsText+=' -fullscreen'}
switch($Mode){
 'Fast'{$argsText+=' -RunnerQAFast'}
 'Review'{$argsText+=' -RunnerReviewQA'}
 'Capture'{$argsText+=' -RunnerQACaptureOnly'}
 'Video'{$argsText+=' -RunnerQAVideo'}
 'Debug'{$argsText+=' -RunnerQADebugViews -ini:Engine:[SystemSettings]:r.ForceDebugViewModes=1'}
}
$windowStyle=if($ShowGame){'Normal'}else{'Hidden'}
$process=Start-Process -FilePath $exe -ArgumentList $argsText -WindowStyle $windowStyle -PassThru
Write-Output ('Unreal process '+$process.Id+'; output '+$out)
if($Wait){$process.WaitForExit();Write-Output ('Exited '+$process.ExitCode);if(Test-Path -LiteralPath (Join-Path $out 'metrics.json')){Get-Content -LiteralPath (Join-Path $out 'metrics.json')}}
