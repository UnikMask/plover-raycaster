@ECHO OFF
if not defined DevEnvDir (
    @call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"
)
cmake --build "build\x64-debug"
