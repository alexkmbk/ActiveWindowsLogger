@echo off
setlocal

set "WIX=C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
set /p PRODUCT_VERSION=<ProductVersion.txt

del /q ActiveWindowsLogger.wixobj 2>nul
del /q ActiveWindowsLogger.wixpdb 2>nul

"%WIX%" build ActiveWindowsLogger.wxs -arch x86 -d ProductVersion=%PRODUCT_VERSION% -out ActiveWindowsLogger.msi
