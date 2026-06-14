@echo off
setlocal

set "WIX=C:\Program Files\WiX Toolset v7.0\bin\wix.exe"

del /q ActiveWindowsLogger.wixobj 2>nul
del /q ActiveWindowsLogger.wixpdb 2>nul

"%WIX%" build ActiveWindowsLogger.wxs -arch x86 -out ActiveWindowsLogger.msi
