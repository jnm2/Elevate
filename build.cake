// #cake version=0.25.0

var configuration = Argument("configuration", "Release");

var platforms = new[]
{
    (target: PlatformTarget.x64, outputPath: $"src/Elevate/bin/x64/{configuration}/Elevate.exe"),
    (target: PlatformTarget.x86, outputPath: $"src/Elevate/bin/Win32/{configuration}/Elevate.exe")
};

var releaseDir = Directory("release");


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

Task("Release")
    .IsDependentOn(build)
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
    });


RunTarget(Argument("target", "Build"));
