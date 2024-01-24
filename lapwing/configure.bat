@ECHO OFF
if not defined DevEnvDir (
    @call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat"
)
cmake . --preset "x64-debug" -DCMAKE_EXPORT_COMPILE_COMMANDS=1
move build\x64-debug\compile_commands.json .
