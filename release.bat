@echo off
rem ============================================================
rem release.bat - Publie une release GitHub avec GitHub CLI (gh)
rem
rem Usage :
rem   release [/draft] [/prerelease] <tag> <fichier-notes> <fichier1> [fichier2 ...]
rem
rem Exemple :
rem   release v2026.11.27 release-notes.md myprog-2026.11.27.zip myprog-2026.11.27.exe
rem
rem Variables d'environnement facultatives :
rem   RELEASE_REPO   proprietaire/depot (sinon : depot du dossier courant)
rem
rem Un fichier SHA256SUMS.txt est genere et joint automatiquement.
rem ============================================================
setlocal EnableExtensions

set "FLAGS="

:opts
if /i "%~1"=="/draft" set "FLAGS=%FLAGS% --draft" & shift & goto :opts
if /i "%~1"=="/prerelease" set "FLAGS=%FLAGS% --prerelease" & shift & goto :opts

set "TAG=%~1"
set "NOTES=%~2"
if "%TAG%"=="" goto :usage
if "%NOTES%"=="" goto :usage
if "%~3"=="" goto :usage

where gh >nul 2>&1
if errorlevel 1 goto :nogh

gh auth status >nul 2>&1
if errorlevel 1 goto :noauth

if not exist "%NOTES%" goto :nonotes

rem --- Verification des fichiers et construction de la liste ---
set "FILES="
shift
shift
:collect
if "%~1"=="" goto :collected
if not exist "%~1" goto :nofile
set FILES=%FILES% "%~1"
shift
goto :collect
:collected

rem --- Generation du fichier de sommes de controle SHA-256 ---
set "TMPDIR=%TEMP%\release-%RANDOM%%RANDOM%"
mkdir "%TMPDIR%" >nul 2>&1
set "SUMS=%TMPDIR%\SHA256SUMS.txt"
type nul > "%SUMS%"

rem On reparcourt les arguments d'origine pour calculer les empreintes
for %%F in (%FILES%) do call :hashone "%%~F"

set "REPOARG="
if defined RELEASE_REPO set "REPOARG=--repo %RELEASE_REPO%"

echo Publication de la release %TAG% ...
gh release create "%TAG%" %FILES% "%SUMS%" --title "%TAG%" --notes-file "%NOTES%" %REPOARG% %FLAGS%
set "RC=%ERRORLEVEL%"

rmdir /s /q "%TMPDIR%" >nul 2>&1

if not "%RC%"=="0" goto :failed
echo.
echo Release %TAG% publiee avec succes.
exit /b 0

:hashone
set "HASH="
for /f "skip=1 tokens=* delims=" %%L in ('certutil -hashfile "%~1" SHA256 ^| findstr /v /i "certutil"') do if not defined HASH set "HASH=%%L"
set "HASH=%HASH: =%"
>>"%SUMS%" echo %HASH%  %~nx1
goto :eof

:usage
echo Usage : release [/draft] [/prerelease] ^<tag^> ^<fichier-notes^> ^<fichier1^> [fichier2 ...]
echo Exemple : release v2026.11.27 release-notes.md myprog-2026.11.27.zip myprog-2026.11.27.exe
exit /b 2

:nogh
echo Erreur : GitHub CLI (gh) est introuvable. Installez-le : winget install GitHub.cli
exit /b 3

:noauth
echo Erreur : gh n'est pas authentifie. Lancez "gh auth login" ou definissez GH_TOKEN.
exit /b 3

:nonotes
echo Erreur : fichier de notes introuvable : %NOTES%
exit /b 4

:nofile
echo Erreur : fichier introuvable : %~1
exit /b 4

:failed
echo.
echo Erreur : la creation de la release a echoue (code %RC%).
exit /b %RC%
