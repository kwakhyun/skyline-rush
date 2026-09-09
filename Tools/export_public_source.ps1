param([Parameter(Mandatory=$true)][string]$Destination)
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ExportRoot = [IO.Path]::GetFullPath($Destination)
if (Test-Path -LiteralPath $ExportRoot) { throw 'Use a new export directory to preserve existing files.' }
New-Item -ItemType Directory -Path $ExportRoot | Out-Null
$Candidates = @(& git -C $ProjectRoot ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0) { throw 'Could not enumerate project source.' }
$RankingRoot = Join-Path $ProjectRoot 'Server/Ranking'
$Candidates += @(& git -C $RankingRoot ls-files --cached --others --exclude-standard | ForEach-Object { 'Server/Ranking/' + $_ })
if ($LASTEXITCODE -ne 0) { throw 'Could not enumerate ranking source.' }
$Copied = 0
foreach ($Relative in ($Candidates | Sort-Object -Unique)) {
    $SourceFile = Join-Path $ProjectRoot $Relative
    if (!(Test-Path -LiteralPath $SourceFile -PathType Leaf)) { continue }
    if ($Relative -match '(^|/)(\.git|node_modules|dist|\.next|\.wrangler|\.vite|Saved|Intermediate|Binaries|DerivedDataCache)(/|$)') { continue }
    if ($Relative -match '(^|/)(\.env[^/]*|\.dev\.vars|[^/]*\.(pem|key|pfx|p12|db|sqlite|sqlite3))$') { continue }
    $Allowed = $Relative -match '^(Source|Config|Tools|Docs|Server/Ranking)/' -or
        $Relative -in @('.gitignore','.gitattributes','.vsconfig','AGENTS.md','README.md','SkylineRush.uproject') -or
        $Relative -match '^SourceArt/.*\.(md|json|py|ps1|mjs)$' -or
        $Relative -match '^SourceArt/UI/Title/T_SkylineRush_(Title|Illustration)_v2\.png$'
    if (!$Allowed) { continue }
    # Raw Unreal packages and editable models are not part of the public source distribution.
    if ($Relative -match '\.(uasset|umap|fbx|blend|vrm|vroid|glb|gltf|zip|exe|dll|pdb)$') { continue }
    $TargetFile = Join-Path $ExportRoot $Relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $TargetFile) | Out-Null
    Copy-Item -LiteralPath $SourceFile -Destination $TargetFile
    $Copied++
}
Write-Output "Exported $Copied files to $ExportRoot. Original repositories and histories are unchanged."
