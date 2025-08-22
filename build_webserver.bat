@echo off
REM Build script for Hyperion Web Server with WebSocket support

echo Building Hyperion Web Server...

set INCLUDE_DIRS=-I. -I.\core -I.\models\text -I.\interface -I.\utils
set SOURCE_FILES=interface\web_server.c interface\websocket.c core\config.c core\memory.c models\text\tokenizer.c models\text\generate.c utils\performance_monitor.c
set LIBS=-lws2_32

echo Compiling...
gcc %INCLUDE_DIRS% %SOURCE_FILES% %LIBS% -o web_server.exe -DHYPERION_MEMORY_TRACKING

if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful! 
echo.
echo To run the web server:
echo   web_server.exe --port 8080 --document-root interface
echo.
echo Then open your browser to:
echo   http://localhost:8080
echo.
echo For WebSocket testing:
echo   http://localhost:8080/websocket_test.html
echo.
pause