del ActiveWindowsLogger.wixobj
del ActiveWindowsLogger.wixpdb
"C:\Program Files (x86)\WiX Toolset v3.11\bin\candle.exe" -ext WixUtilExtension ActiveWindowsLogger.wxs
"C:\Program Files (x86)\WiX Toolset v3.11\bin\light.exe" -ext WixUtilExtension ActiveWindowsLogger.wixobj