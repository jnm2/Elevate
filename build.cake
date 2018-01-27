// #cake version=0.25.0

var configuration = Argument("configuration", "Release");

var packageId = "jnm2-elevate";

var platforms = new[]
{
    (target: PlatformTarget.x64, outputPath: $"src/Elevate/bin/x64/{configuration}/Elevate.exe"),
    (target: PlatformTarget.x86, outputPath: $"src/Elevate/bin/Win32/{configuration}/Elevate.exe")
};

var releaseDir = Directory("artifacts/release");
var packOutputDir = Directory("artifacts/choco");


void Build(string target, PlatformTarget platformTarget)
{
    MSBuild("src", new MSBuildSettings()
        .SetConfiguration(configuration)
        .WithTarget(target)
        .SetPlatformTarget(platformTarget));
}

var clean = Task("Clean");
var build = Task("Build").IsDependentOn(clean);

foreach (var (target, _) in platforms)
{
    var cleanTarget = Task("Clean-" + target)
        .Does(() => Build("Clean", target));
    clean.IsDependentOn(cleanTarget);

    var buildTarget = Task("Build-" + target)
        .IsDependentOn(cleanTarget)
        .Does(() => Build("Build", target));
    build.IsDependentOn(buildTarget);
}

System.Diagnostics.FileVersionInfo GetVersionInfo()
{
    return System.Diagnostics.FileVersionInfo.GetVersionInfo(platforms.First().outputPath);
}

var pack = Task("Pack")
    .IsDependentOn(build)
    .Does(() =>
    {
        var files = new List<ChocolateyNuSpecContent>
        {
            new ChocolateyNuSpecContent { Source = "pkg/chocolateyInstall.ps1", Target = "tools" },
            new ChocolateyNuSpecContent { Source = "pkg/chocolateyUninstall.ps1", Target = "tools" }
        };

        foreach (var platform in platforms)
            files.Add(new ChocolateyNuSpecContent { Source = platform.outputPath, Target = "tools/" + platform.target });

        var info = GetVersionInfo();
        EnsureDirectoryExists(packOutputDir);
        ChocolateyPack(new ChocolateyPackSettings
        {
            Id = packageId,
            Title = info.ProductName,
            Version = info.ProductVersion,
            Authors = new[] { "jnm2" },
            Summary = info.FileDescription,
            Description = "Elevate a single process or elevate your shell in-window.",
            ProjectUrl = new Uri("https://github.com/jnm2/Elevate"),
            PackageSourceUrl = new Uri("https://github.com/jnm2/Elevate"),
            ProjectSourceUrl  = new Uri("https://github.com/jnm2/Elevate"),
            BugTrackerUrl = new Uri("https://github.com/jnm2/Elevate/issues"),
            Tags = new [] { "windows", "sudo", "elevate", "elevation", "admin", "administrator", "administrative", "rights", "runas", "console", "command", "prompt", "terminal" },
            Copyright = info.LegalCopyright,
            LicenseUrl = new Uri("https://github.com/jnm2/Elevate/blob/master/LICENSE.txt"),
            Files = files,
            OutputDirectory = packOutputDir
        });
    });

Task("Release")
    .IsDependentOn(pack)
    .Does(() =>
    {
        EnsureDirectoryExists(releaseDir);
        CleanDirectory(releaseDir);

        foreach (var (target, outputPath) in platforms)
        {
            if (target == PlatformTarget.x64)
                CopyFileToDirectory(outputPath, releaseDir);
            else
                Zip(File(outputPath).Path.GetDirectory(), releaseDir + File(target + ".zip"), outputPath);
        }

        Zip(packOutputDir, releaseDir + File("choco.zip"), packOutputDir + File($"{packageId}.{GetVersionInfo().ProductVersion}.nupkg"));
    });


RunTarget(Argument("target", "Build"));
