REM Build script for engine
@ECHO OFF
SetLocal EnableDelayedExpansion

REM echo "Files:" %cFilenames%

SET assembly=engine
SET compilerFlags=-g -Wvarargs -Wall -Werror
SET testFlags=-g -Wvarargs -Wall -Werror
REM -Wall -Werror
SET includeFlags=-Isrc -I%VULKAN_SDK%/Include
SET linkerFlags=-shared -luser32 -lvulkan-1 -L%VULKAN_SDK%/Lib
SET defines=-D_DEBUG -DVEXPORT -DVULKAN_RENDER -D_CRT_SECURE_NO_WARNINGS
SET testDefines=-D_DEBUG -DVEXPORT -DVULKAN_RENDER -DTESTING -D_CRT_SECURE_NO_WARNINGS

ECHO "Building %assembly%%..."
REM Get a list of all the .c files
SET objFiles=
FOR /R %%f in (*.c) do (
  clang %%f %compilerFlags% -c -o ../obj/%%~nxf.o %defines% %includeFlags%
  SET objFiles=!objFiles! ../obj/%%~nxf.o
)
FOR /R %%f in (*.cpp) do (
  clang++ -Wno-nullability-completeness -g -std=c++23 %%f -c -o ../obj/%%~nxf.o %includeFlags%
  SET objFiles=!objFiles! ../obj/%%~nxf.o
)
clang -g %objFiles% -o ../bin/%assembly%.dll %linkerFlags%
REM clang %cFilenames% %testFlags% -o ../bin/%assembly%_test.exe %testDefines% %includeFlags% %linkerFlags%