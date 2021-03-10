set rootDir=%1
set targetDllPath=%2
set platform=%3

set opensslpath=%rootDir%/Depends/openssl/bin/windows/x%platform%/release
set screenshotpath=%rootDir%/Depends/Screenshot/bin/x%platform%
set qtutilpath=%rootDir%/Depends/QtUtil/bin/x%platform%
set quazippath=%rootDir%/Depends/quazip/bin/x%platform%
set eventbuspath=%rootDir%/Depends/EventBus/bin/x%platform%
set qzxingpath=%rootDir%/Depends/qzxing/bin/x%platform%
set curlpath=%rootDir%/Depends/curl/bin/windows/x%platform%
set logicbase=%rootDir%/Depends/LogicBase/bin/x%platform%


copy "%opensslpath%/\*.dll" "%targetDllPath%"
copy "%screenshotpath%/\*.dll" "%targetDllPath%"
copy "%qtutilpath%/\*.dll" "%targetDllPath%"
copy "%quazippath%/\*.dll" "%targetDllPath%"
copy "%eventbuspath%/\*.dll" "%targetDllPath%"
copy "%qzxingpath%/\*.dll" "%targetDllPath%"
copy "%curlpath%/\libcurl.dll" "%targetDllPath%"
copy "%logicbase%/\*.dll" "%targetDllPath%"
