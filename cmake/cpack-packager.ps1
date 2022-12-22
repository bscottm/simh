# Author: B. Scott Michel (scooter.phd@gmail.com)
# "scooter me fecit"

<#
.SYNOPSIS
Package the SIMH simulator suite for Windows-based installers (EXE and MSI)

.DESCRIPTION
Execute the CPack packager to create EXE and MSI installers for Windows platforms.

.EXAMPLE
PS> cpack ...

Generate/configure, build, test and install the SIMH simulator suite using
the Visual Studio 2022 toolchain in the Release (optimized) compile
configuration.
#>

param (
    ## String arguments are positional, so if the user invokes this script
    ## as "cmake-builder.ps1 vs2022 Debug", it's the same as saying
    ## "cmake-builder.ps1 -flavor vs2022 -config Debug"


    ## The build environment's "flavor" that determines which CMake generator is used
    ## to create all of the build machinery to compile the SIMH simulator suite
    ## and the target compiler.
    ## 
    ## Supported flavors:
    ## ------------------
    ## vs2022          Visual Studio 2022 (default)
    ## vs2022-xp       Visual Studio 2022 XP compat
    ## vs2019          Visual Studio 2019
    ## vs2019-xp       Visual Studio 2019 XP compat
    ## vs2017          Visual Studio 2017
    ## vs2017-xp       Visual Studio 2017 XP compat
    ## vs2015          Visual Studio 2015
    [Parameter(Mandatory=$false)]
    [string] $flavor         = "vs2022",

    ## The target build configuration. Valid values: "Release" and "Debug"
    [Parameter(Mandatory=$false)]
    [string] $config         = "Release",

    ## The packager
    [Parameter(Mandatory=$false)]
    [string] $packager,

    ## The rest are flag arguments

    ## Get help.
    [Parameter(Mandatory=$false)]
    [switch] $help           = $false
)

$scriptName = $(Split-Path -Leaf $PSCommandPath)
$scriptCmd  = ${PSCommandPath}

function Show-Help
{
    Get-Help -full ${scriptCmd}
    exit 0
}


## CMake generator info:
class GeneratorInfo
{
    [string]  $Generator
    [string]  $Suffix

    GeneratorInfo([string]$gen, [string]$suffix)
    {
        $this.Generator = $gen
        $this.Suffix    = $suffix
    }
}

$cmakeGenMap = @{
    "vs2022"      = [GeneratorInfo]::new("Visual Studio 17 2022", "win32-native");
    "vs2022-xp"   = [GeneratorInfo]::new("Visual Studio 17 2022", "win32-xp");
    "vs2019"      = [GeneratorInfo]::new("Visual Studio 16 2019", "win32-native");
    "vs2019-xp"   = [GeneratorInfo]::new("Visual Studio 16 2019", "win32-xp");
    "vs2017"      = [GeneratorInfo]::new("Visual Studio 15 2017", "win32-native");
    "vs2017-xp"   = [GeneratorInfo]::new("Visual Studio 15 2017", "win32-xp");
    "vs2015"      = [GeneratorInfo]::new("Visual Studio 14 2015", "win32-native");
}

$cpackPackagers = @{
    "NSIS" = {};
    "WIX"  = {};
    "ZIP"  = {};
}


function Get-GeneratorInfo([string]$flavor)
{
    return $cmakeGenMap[$flavor]
}

## Output help early and exit.
if ($help)
{
    Show-Help
}

$cmakeCmd = $(Get-Command -Name cmake.exe -ErrorAction Ignore).Path
$cpackCmd = $(Get-Command -Name cpack.exe -ErrorAction Ignore).Path
if ($cmakeCmd.Length -gt 0)
{
    Write-Host "** ${scriptName}: cmake is '${cmakeCmd}'"
    Write-Host "** $(& ${cmakeCmd} --version)"
}
else {
    @"
!! ${scriptName} error:

The 'cmake' command was not found. Please ensure that you have installed CMake
and that your PATH environment variable references the directory in which it
was installed.
"@

    exit 1
}
if ($cpackCmd.Length -gt 0)
{
    Write-Host "** ${scriptName}: cpack is '${cpackCmd}'"
}
else {
    @"
!! ${scriptName} error:

The 'cpack' command was not found -- unable to package the SIMH simulator
suite. Please check your CMake installation and PATH environment variable.
"@

    exit 1
}

## Validate the requested configuration.
if (!@("Release", "Debug").Contains($config))
{
    @"
${scriptName}: Invalid configuration: "${config}".

"@
    Show-Help
}

## Look for Git's /usr/bin subdirectory: CMake (and other utilities) have issues
## with the /bin/sh installed there (Git's version of MinGW.)

$tmp_path = $env:PATH
$git_usrbin = "${env:ProgramFiles}\Git\usr\bin"
$tmp_path = ($tmp_path.Split(';') | Where-Object { $_ -ne "${git_usrbin}"}) -join ';'
if ($tmp_path -ne ${env:PATH})
{
    Write-Host "** ${scriptName}: Removed ${git_usrbin} from PATH (Git MinGW problem)"
    $env:PATH = $tmp_path
}

## Setup:
$simhTopDir = $(Split-Path -Parent $(Resolve-Path -Path $PSCommandPath).Path)
While (!([String]::IsNullOrEmpty($simhTopDir) -or (Test-Path -Path ${simhTopDir}\CMakeLists.txt))) {
    $simhTopDir = $(Split-Path -Parent $simhTopDir)
}
if ([String]::IsNullOrEmpty($simhTopDir)) {
    @"
!! ${scriptName}: Cannot locate SIMH top-level source directory from
the script's path name. You should really not see this message.
"@

    exit 1
} else {
    Write-Host "** ${scriptName}: SIMH top-level source directory is ${simhTopDir}"
}

$buildDir  = "${simhTopDir}\cmake\build-${flavor}"
$genInfo = $(Get-GeneratorInfo $flavor)
if ($null -eq $genInfo)
{
    Write-Host ""
    Write-Host "!! ${scriptName}: Unrecognized build flavor '${flavor}'."
    Write-Host ""
    Show-Help
}

if ($null -eq $cpackPackagers[$packager])
{
    Write-Host ""
    Write-Host "!! ${scriptName}: Unrecognized packager '${packager}'."
    Write-Host ""
    Show-Help
}

try {
    Push-Location ${buildDir}
    $suffix=$genInfo.Suffix
    Write-Host ${cpackCmd} -G ${packager} -C ${config} "-DSIMH_PACKAGE_SUFFIX=${suffix}"
    & ${cpackCmd} -G ${packager} -C ${config} "-DSIMH_PACKAGE_SUFFIX=${suffix}"
}
catch {
    throw $_
}
finally {
    Pop-Location
}