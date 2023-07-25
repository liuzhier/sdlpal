copy app\build\outputs\apk\release\app-release-unsigned.apk .\

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore sdlpal.pwd -storepass sdlpal "app-release-unsigned.apk" sdlpal

zipalign -v 4 "app-release-unsigned.apk" "sdlpal mod -release.apk"

del /F /S /Q "app-release-unsigned.apk"