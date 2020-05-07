@ECHO OFF
XCOPY /D /Y /Q "..\..\brave\script\redirect-cc.py" "%CD%/redirect.*" 1>nul
python redirect.py %*
