if exist *.uvgui.* del *.uvgui.*
if exist *.bak del *.bak
if exist *.dep del *.dep
for %%a in (.\Objs\*.*) do (if "%%~xa" neq ".???" if "%%~xa" neq ".????" if "%%~xa" neq ".bin" del /Q /F %%a)
if exist .\Objs\*.dep del .\Objs\*.dep
if exist *.scvd del *.scvd
