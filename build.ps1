[CmdletBinding()]
param (
    [string]$Script = "build.cake",
    [string]$Target,
    [string]$Configuration,
    [ValidateSet("Quiet", "Minimal", "Normal", "Verbose", "Diagnostic")]
    [string]$Verbosity,
    [switch]$ShowDescription,
    [Alias("WhatIf", "Noop")]
    [switch]$DryRun,
    [switch]$Experimental,
    [switch]$Mono,
    [switch]$SkipToolPackageRestore,
    [Parameter(Position=0,Mandatory=$false,ValueFromRemainingArguments=$true)]
    [string[]]$ScriptArgs
)

function Parse-CakeDirectives([string] $scriptPath) {
    $cakeDirectiveOptions = @{}

    try {
        foreach ($optionMatch in
            select-string -Path $scriptPath '(?im)^[^\S\n\r]*(?://[^\S\n\r]*)?#cake[^\S\n\r]+(?<options>.*)$' -AllMatches |
            foreach Matches |
            foreach { $_.Groups['options'].Value } |
            select-string '(?<key>version|dir|source)\s*=\s*(?<value>[^"\s]+|"([^"]|"")*")' -AllMatches |
            foreach Matches |
            where { $_ -ne $null }
        ) {
            $optionKey = $optionMatch.Groups['key'].Value
            if ($cakeDirectiveOptions.ContainsKey($optionKey)) {
                throw "`"#cake $optionKey`" must only appear once in '$scriptPath'."
            }

            $optionValue = $optionMatch.Groups['value'].Value
            if ($optionValue.StartsWith('"')) {
                $optionValue = $optionValue.Substring(1, $optionValue.Length - 2).Replace('""', '"')
            }

            $cakeDirectiveOptions.Add($optionKey, $optionValue)
        }
    }
    catch [System.Management.Automation.ItemNotFoundException] {
        throw "Cannot find build script at '$scriptPath'."
    }

    return $cakeDirectiveOptions
}

function Extract-ZipFromUrlToDirectory([uri] $zipUrl, [string] $targetDir) {
    Add-Type -assembly System.IO.Compression.FileSystem

    if ($zipUrl.IsFile)
    {
        [System.IO.Compression.ZipFile]::ExtractToDirectory(
            [System.IO.Path]::Combine((get-location), $zipUrl.LocalPath),
            [System.IO.Path]::Combine((get-location), $targetDir))
    }
    else
    {
        Add-Type -assembly System.IO.Compression

        $webClient = [System.Net.WebClient]::new()
        $downloadStream = $webClient.OpenRead($zipUrl)
        $zip = [System.IO.Compression.ZipArchive]::new($downloadStream)

        [System.IO.Compression.ZipFileExtensions]::ExtractToDirectory($zip,
            [System.IO.Path]::Combine((get-location), $targetDir))

        $zip.Dispose()
        $downloadStream.Dispose()
        $webClient.Dispose()
    }
}

function Ensure-CakeInstall([string] $targetDir, [version] $pinnedVersion, [uri] $nugetSource) {
    $cakePath = join-path $targetDir 'Cake.exe'

    if (test-path $cakePath)
    {
        if ($pinnedVersion -eq $null) { return $cakePath }

        $rawVersion = (get-item $cakePath).VersionInfo.ProductVersion
        $buildMetadataIndex = $rawVersion.IndexOf('+')
        $currentVersion = if ($buildMetadataIndex -eq -1) { $rawVersion } else { $rawVersion.Substring(0, $buildMetadataIndex) }
        if ($currentVersion -eq $pinnedVersion) { return $cakePath }

        Write-Host "Cake $currentVersion found; replacing with pinned version $pinnedVersion. Downloading..."
    }

    $downloadUrl =
        if ($nugetSource.IsFile) { "$nugetSource\Cake.$pinnedVersion.nupkg" }
        else { "$nugetSource/package/Cake/$pinnedVersion" }

    Remove-Item -Recurse -Force $targetDir -ErrorAction Ignore
    Extract-ZipFromUrlToDirectory $downloadUrl $targetDir

    return $cakePath
}

$cakeDirectiveOptions = Parse-CakeDirectives $Script

$pinnedVersion = $cakeDirectiveOptions.version
$targetDir = if ($cakeDirectiveOptions.dir -ne $null) { $cakeDirectiveOptions.dir } else { 'tools\Cake' }
$nugetSource = if ($cakeDirectiveOptions.source -ne $null) { $cakeDirectiveOptions.source } else { 'https://www.nuget.org/api/v2' }

$cakePath = Ensure-CakeInstall $targetDir $pinnedVersion $nugetSource


$cakeArguments = @($Script)
if ($Target) { $cakeArguments += "--target=$Target" }
if ($Configuration) { $cakeArguments += "--configuration=$Configuration" }
if ($Verbosity) { $cakeArguments += "--verbosity=$Verbosity" }
if ($ShowDescription) { $cakeArguments += "--showdescription" }
if ($DryRun) { $cakeArguments += "--dryrun" }
if ($Experimental) { $cakeArguments += "--experimental" }
if ($Mono) { $cakeArguments += "--mono" }
$cakeArguments += $ScriptArgs

Write-Host "Running build script..."
&$cakePath $cakeArguments
exit $LASTEXITCODE
