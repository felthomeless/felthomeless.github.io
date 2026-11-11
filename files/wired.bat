:: having isues with the batch? run it and input "t" 

cls

@echo off

if "%1"==":a" goto :a
if "%1"==":b" goto :b
if "%1"==":m" goto :m
if "%1"==":r" goto :r
if "%1"==":d" goto :d
if "%1"==":c" goto :c
if "%1"==":u" goto :u

set "currentuser=%username%"

color 0a
setlocal enabledelayedexpansion
title loading wired
cls
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo                       loading complete
powershell -command "Start-Sleep -Seconds 0.5" 
echo                                       welcome %currentuser%
echo.
echo.
echo.
echo.
powershell -command "Start-Sleep -Seconds 0.5" 


title wired


:menu
title wired
cls
echo.                                                                  
echo                    #    #   #    #####     ######    #####  
echo                    #    #   #    #    #    #         #    # 
echo                    #    #   #    #    #    #####     #    # 
echo                    # ## #   #    #####     #         #    # 
echo                    ##  ##   #    #   #     #         #    # 
echo                    #    #   #    #    #    ######    #####                                                      
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.
echo.                                                    
echo 1   fetch                                                  
echo 2   terminal                                              
echo 3   shortcuts                                               
echo 4   taskmanager                                                 
echo 5   miscellaneous

                                    set /p choice=
if /i "%choice%"=="f" powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"
if /i "%choice%"=="b" call :menu
if /i "%choice%"=="u" call :update
if /i "%choice%"=="r" call :rb
if /i "%choice%"=="e" call :elevate
if /i "%choice%"=="x" call :close
if /i "%choice%"=="t" call :troubleshoot
if /i "%choice%"=="a" call :advanced
if /i "%choice%"=="1" call :fetch
if /i "%choice%"=="2" call :terminal
if /i "%choice%"=="3" call :shortcuts
if /i "%choice%"=="4" call :taskmanager
if /i "%choice%"=="5" call :extra
if /i "%choice%"=="edit" start notepad "%cd%\wired.bat" & goto menu

goto menu


:shortcuts
cls

if not exist shortcuts.txt (
    cls
    echo no shortcuts found. creating shortcuts.txt
    attrib +h shortcuts.txt
    echo shortcuts > shortcuts.txt
    echo shortcuts.txt has been created and set to hidden
    pause >nul
)

:show_shortcuts
cls
echo                            input 'create' to add a new 
echo                            input 'delete (name)' to remove
echo                            input 'edit' to edit shorcuts.txt
echo.
echo ------------------------------------------------------------
for /f "tokens=1 delims=;" %%a in (shortcuts.txt) do (
    echo %%a
)
echo ------------------------------------------------------------
echo.
echo input name to open
echo input b to go back
echo.
set /p shortcut_choice=

if /i "%shortcut_choice%"=="create" (
    call :create_shortcut
    goto show_shortcuts
) else if "%shortcut_choice:~0,7%"=="delete " (
    call :delete_shortcut "%shortcut_choice:~7%"
    goto show_shortcuts
) else if /i "%shortcut_choice%"=="b" (
    goto menu
)
    call :open_shortcut "%shortcut_choice%"
    goto show_shortcuts
)

:create_shortcut
cls
set /p shortcut_name="enter shortcut name (type B to go back) "
if /i "%shortcut_name%"=="b" goto show_shortcuts

set /p shortcut_path="enter the path for the shortcut (type B to go back) "
if /i "%shortcut_path%"=="b" goto show_shortcuts


findstr /x /c:"%shortcut_name%;%shortcut_path%" shortcuts.txt >nul
if %errorlevel%==0 (
    echo a shortcut with that name already exists
    pause >nul
    goto show_shortcuts
)


echo %shortcut_name%;%shortcut_path% >> shortcuts.txt
echo shortcut added successfully

pause >nul
goto show_shortcuts

:delete_shortcut
cls
set "delete_name=%~1"

findstr /c:"%delete_name%;" shortcuts.txt >nul
if %errorlevel% neq 0 (
    echo shortcut '%delete_name%' not found
    pause >nul
    goto show_shortcuts
)


findstr /v /c:"%delete_name%;" shortcuts.txt > temp.txt
move /y temp.txt shortcuts.txt >nul
echo shortcut '%delete_name%' deleted
pause >nul
goto show_shortcuts

:open_shortcut
cls
set "shortcut_name=%~1"

:: find the shortcut path
for /f "tokens=1,2 delims=;" %%a in (shortcuts.txt) do (
    if /i "%%a"=="%shortcut_name%" (
        if exist "%%b" (
            start explorer "%%b"
        ) else (
            echo shortcut path not found: %%b
            pause >nul
        )
        goto show_shortcuts
    )
)

echo shortcut not found
pause >nul
goto show_shortcuts

:fetch
cls
set "currentuser=%username%"

:: operating system
for /f "tokens=2*" %%i in ('reg query "hklm\software\microsoft\windows nt\currentversion" /v productname ^| findstr /c:"productname"') do (
    set "os=%%j"
)
set "os=!os:microsoft =!"

echo operating system: !os!

:: disk usage
set "output="
for /f "tokens=1,2,3 delims=," %%a in ('powershell -noprofile -command "get-psdrive -psprovider filesystem | foreach-object { if ($_.used -gt 0) { $_.name + ',' + [math]::round($_.used/1gb, 0) + ',' + [math]::round(($_.used + $_.free)/1gb, 0) } }"') do (
    set "output=!output!%%a: %%b/%%cgb "
)
echo disk usage: !output!

:: processor information
for /f "tokens=*" %%i in ('wmic cpu get name ^| findstr /r /v "^$"') do (
    set "cpu=%%i"
)
echo processor: !cpu!

:: total ram
for /f "tokens=1" %%b in ('powershell -noprofile -command "get-wmiobject win32_computersystem | foreach-object { [math]::round($_.totalphysicalmemory / 1gb, 2) }"') do (
    set "ramoutput=%%bgb"
)
echo total ram: !ramoutput!

:: ram usage (used and free)
for /f "tokens=2 delims==" %%a in ('wmic os get freephysicalmemory /value') do set "freeram=%%a"
for /f "tokens=2 delims==" %%a in ('wmic os get totalvisiblememorysize /value') do set "totalram=%%a"

if not defined totalram (
    echo failed to retrieve total ram.
    pause >nul
    exit /b
)

if not defined freeram (
    echo failed to retrieve free ram.
    pause >nul
    exit /b
)

set /a "usedram=%totalram% - %freeram%"
set /a "freerammb=%freeram% / 1024"
set /a "usedrammb=%usedram% / 1024"

echo used ram: %usedrammb% mb
echo free ram: %freerammb% mb

:: gpu information 
for /f "tokens=*" %%a in ('powershell -command "get-wmiobject win32_videocontroller | select-object -expandproperty caption"') do (
    set "gpu=%%a"
)


:: system architecture
for /f "skip=1" %%i in ('wmic os get osarchitecture') do (
    set "arch=%%i"
    goto :break_arch
)
:break_arch
echo architecture: !arch!

:: motherboard model
for /f "tokens=*" %%i in ('wmic baseboard get product ^| findstr /r /v "^$"') do (
    set "motherboard=%%i"
)
echo motherboard: !motherboard!

:: pc model
for /f "tokens=*" %%i in ('wmic csproduct get name ^| findstr /r /v "^$"') do (
    set "pcmodel=%%i"
)
echo pc model: !pcmodel!

:: display resolution
for /f "tokens=1,2" %%a in ('wmic path win32_videocontroller get currenthorizontalresolution^,currentverticalresolution ^| findstr /r "^[0-9]"') do (
    set "width=%%a"
    set "height=%%b"
)
echo screen resolution: !width! x !height!

:: last boot time and uptime calculation
for /f "skip=1 tokens=1" %%a in ('wmic os get lastbootuptime') do (
    set lastboot=%%a
    goto :parselastboot
)

:: last boot time and uptime calculation
for /f "skip=1 tokens=1" %%a in ('wmic os get lastbootuptime') do (
    set lastboot=%%a
    goto :parselastboot
)

:parselastboot
set lastbootyear=!lastboot:~0,4!
set lastbootmonth=!lastboot:~4,2!
set lastbootday=!lastboot:~6,2!
set lastboothour=!lastboot:~8,2!
set lastbootminute=!lastboot:~10,2!

:: format boot time for display
set boottime=!lastbootyear!-!lastbootmonth!-!lastbootday! !lastboothour!:!lastbootminute!

:: calculate uptime
for /f "tokens=2 delims==" %%a in ('wmic os get localdatetime /value') do set currenttime=%%a
set currentyear=!currenttime:~0,4!
set currentmonth=!currenttime:~4,2!
set currentday=!currenttime:~6,2!
set currenthour=!currenttime:~8,2!
set currentminute=!currenttime:~10,2!

:: convert times to total minutes since a reference point
set /a lastbootminutes=(1!lastbootyear:~2,2! * 525600) + (1!lastbootmonth! * 43800) + (1!lastbootday! * 1440) + (1!lastboothour! * 60) + 1!lastbootminute!
set /a currentminutes=(1!currentyear:~2,2! * 525600) + (1!currentmonth! * 43800) + (1!currentday! * 1440) + (1!currenthour! * 60) + 1!currentminute!

set /a uptime_minutes=currentminutes - lastbootminutes
set /a uptime_days=uptime_minutes / 1440
set /a uptime_hours=(uptime_minutes %% 1440) / 60
set /a uptime_remaining_minutes=uptime_minutes %% 60


:fetching
cls
echo                            /\     /\ 
echo                           /  \   /  \ 
echo                          /    \_/    \ 
echo                         ^| _     _     \ 
echo                       .^'  _     _  '.. ^| 
echo                      /   (_)   (_)     \ 
echo                     ^|     _______      ^| 
echo                     ^|    ^|       ^|     ^| 
echo                     ^|    ^|       ^|   ; ^| 
echo                      \    \_____/     / 
echo                       '^.           .'^  
echo                         ^'\\____._///^'  
echo current user: %username%
echo operating system: !os!
echo disk usage: !output!
echo processor: !cpu!
echo gpu: !gpu!
echo ram: %usedrammb%mb / !ramoutput!
echo architecture: !arch!
echo motherboard: !pcmodel!

echo screen resolution: !width! x !height!
echo boot time: !boottime!

echo uptime: %uptime_days% days, %uptime_hours% hours, %uptime_remaining_minutes% minutes

pause > nul
goto menu

timeout /t 60
goto :fetch

:matrix
start cmd /k "call %~dp0wired.bat :m"
goto extra

:m
cls




echo .. 
timeout /t 1 >nul 
echo ... 
timeout /t 1 >nul



cls
goto numbers


:numbers
color 0a
powershell -command "Start-Sleep -Seconds 0.0"
echo   3            f         2   1        1   3            f         2   1        1
powershell -command "Start-Sleep -Seconds 0.0"
echo           0   q         e   1   q    0   g        0   q         e   1   q    0
powershell -command "Start-Sleep -Seconds 0.0"
echo   0           3         e   2   q    1   0        h   3         e   2   q    1
powershell -command "Start-Sleep -Seconds 0.0"
echo      1        f         d   k    d    g      1     q   f         d   k    d    g
powershell -command "Start-Sleep -Seconds 0.0"
echo           f    4        7   7    0    g      f     f    4        7   7    0    g
powershell -command "Start-Sleep -Seconds 0.0"
echo      1     z    1        g   2    1     2      1     z    1        g   2    1     2
powershell -command "Start-Sleep -Seconds 0.0"
echo                     0   w             q         1         0   w             q
powershell -command "Start-Sleep -Seconds 0.0"
echo          r         2   0        0    q   2        r         2   0        0    q
powershell -command "Start-Sleep -Seconds 0.0"
echo   F                 3            0    1   F      1           3            0    1
powershell -command "Start-Sleep -Seconds 0.0"
echo         q           4            1    1   k      q           4            1    1
powershell -command "Start-Sleep -Seconds 0.0"
echo              b     5        w        q   t      f     b     5        w        q
powershell -command "Start-Sleep -Seconds 0.0"
echo        f     b              w    1   j      f     b              w    1
powershell -command "Start-Sleep -Seconds 0.0"
echo        b     f              0    1   7      b     f              0    1  r

goto numbers


:ascii

cls
echo                        ascii
echo  B back
echo  F fullscreen
echo.
echo  1 hanekawa
echo  2 coplandOS
echo  3 tux
echo  4 
echo  5 flicker (doesnt stop)
echo  6 lain
set /p choice=

if /i "%choice%"=="f" powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"
if /i "%choice%"=="b" goto :extra
if "%choice%"=="1" goto :hanekawa
if "%choice%"=="2" goto :copland
if "%choice%"=="3" goto :tux
if "%choice%"=="4" goto :
if "%choice%"=="5" goto :flicker
if "%choice%"=="6" goto :lain1

goto :ascii

:flicker
ping localhost -n 1 >nul
cls
if "%color%" == "0A" (set color=A0) else (set color=0A)
color %color%
goto :flicker

pause > nul
goto ascii



:tux
cls
echo there appears to be more than one variant
echo tux 1
echo tux 2
set /p t=

if "%t%"=="1" goto :tux1
if "%t%"=="2" goto :tux2
if "%t%"=="b" goto :ascii

goto tux

:tux1
cls
echo.
echo                                                                 #####
echo                                                                #######
echo                   #                                            ##O#O##
echo  ######          ###                                           #VVVVV#
echo    ##             #                                          ##  VVV  ##
echo    ##         ###    ### ####   ###    ###  ##### #####     #          ##
echo    ##        #  ##    ###    ##  ##     ##    ##   ##      #            ##
echo    ##       #   ##    ##     ##  ##     ##      ###        #            ###
echo    ##          ###    ##     ##  ##     ##      ###       QQ#           ##Q
echo    ##       # ###     ##     ##  ##     ##     ## ##    QQQQQQ#       #QQQQQQ
echo    ##      ## ### #   ##     ##  ###   ###    ##   ##   QQQQQQQ#     #QQQQQQQ
echo  ############  ###   ####   ####   #### ### ##### #####   QQQQQ#######QQQQQ
pause >nul
goto ascii

:tux2
cls
echo              a8888b.
echo             d888888b.
echo             8P"YP"Y88
echo             8^|o^|^|^o^|88
echo             8'    .88
echo             8`.^_.' Y8.
echo            d/      `8b.
echo          .dP   .     Y8b.
echo         d8:'   "   `::88b.
echo        d8"           `Y88b
echo       :8P     '       :888
echo        8a.    :      _a88P
echo      .^_^/"Yaa_ :    .^| 88P|
echo      ^\    YP"      `^| 8P  `.
echo      ^/     ^\._____.d^|    .'
echo      `--..^_^_)888888P`.^_^.'
pause >nul
goto ascii




:neko
cls
echo     ____      /| __   _           
echo    / \ \____/ |//___/ \ 
echo    | <   \ \   \|/_   \>      YOU ARE NOT
echo    | <   /   /   \  \ \/  IMMUNE TO PROPAGANDA
echo     \ //     |    |  \ \
echo    / // /   /|    |\  | \
echo   /  /`’ \-/-|-/\/--\-/ \\
echo  /   |  ( ()   )(()   ) | \
echo  -|  |  /-   -   -  -/  |\|
echo   \/\ \/ \_  (_/\_)_’/\ |
echo  __  \/ `’\! `_’ \/ `/ `’
echo (  )     /  )    \\ 
echo | |     /  /      \ \
echo \  \ _--_ //       \\--_
echo  \ _/    \ \______/|    |
echo     \_  _//  / |  \ \--
echo          `/` /”`|`|`’
echo          /’’/   |’| 
echo         |---|  |---|
echo         /   / |    \
echo        |   |   ““\__|
echo         ---


pause >nul

goto ascii

:lain1
chcp 65001
cls
echo            _..--¯¯¯¯--.._
echo        ,-''              `-.
echo      ,'                     `.
echo     ,                         \
echo    /                           \
echo   /          ′.                 \
echo  '          /  ^│^│                ;
echo  ;       n /│  ^│/         ^│      ^│
echo  ^│      / v    /\/`-'v√\'.^│^\     ,
echo  :    /v`,———         ————.^.     ;
echo  '   ^│  /′@@`,        ,@@ `\^│    ;
echo  ^│  n^│  '.@@/         \@@  ^/^│^\  │;
echo  ` ^│ `    ¯¯¯          ¯¯¯  ^│ ^\^/^│^│
echo   \ \ \                     ^│ /\/
echo   '; `-\          `′       /^│/ ^│′
echo    `    \       —          /^│  ^│
echo     `    `.              .' ^│  ^│
echo      v,_   `;._     _.-;    ^│  ^/
echo         `'`\^│-_`'-''^_^_^/^'^' │ ^│
echo                ¯¯¯¯¯        ^│ ^│
echo                             ^│ ^/
echo                             ^│^│
echo                             ^│^│
echo                             ^│,
pause >nul
chcp 437
goto ascii



pause >nul

goto ascii


:hanekawa
cls
echo                        WWMMMMGGGGGGGGGGGGGGGGGMMMMMMMMGGMMMMMMMMMMMMWWWWMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMW
echo                       WWMMMMGGGGGGGGGGGGGGGGGGMMMMMMMMGGMMMMMMMMMMMMWWWWRWWMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMW
echo                       WWMMMGGGGGGGGGGGGGGGGGGGMMMMMMMMMMMMMMMMMMMMGWWWWGMMRRMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMW
echo                      WWMMMGGGGGGGGGGGGGGGGGGGGGMMMMMGMMMMMMMMMMMGWWW.WWGMMMMMMWMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMGGGGMMMMMMMMMMMMMMMMMMMMMMMW
echo                     WWMMMMGGGGGGGGGGGGGGGGGGGGGGGMMGGGMMGGMMMGGGWW....WGMMMMMMMWWWWWMMMMMMMMMMMGGGGGGGGGGGGGGGGGGGGGGGGGMGMMWWWWMMMMMMMMMMMMMMMW
echo                     WWMMMMGGGGGGGGGGGGGGGGGGGGGGGGMGGMMMMMMGGGWW......WWGMMMMMGGGGWMMRWWMMMMMMMMMMMGGGGGGGGMMMMGGGGGGGGGGMWWWWWWMMGMMMMMMMMMMMMW
echo                    WWMMMMGGGGGGGGGGGGGGGGGGGGGGGGMGGGGGGGGGGWWW........WWMMMMMMGGGGGGMWWMMMWMGGGGGGGGGGGGGGGGGGGGGGGGGGGGMWWMMGGGGGGGGGGMMMMMMMWW
echo                   WWMMMMMGGGWWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGWW..WWWW....WWGMMMMMGGGGGGGGGGMMMMMMMMGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGMMMGMMMMMWWW
echo                  WWMMMMMGGWWWWWGGGGGGGGGGGGGGGGGGGGGGGGGWW.WWW...WW....WWGMMMMGGGGGGGGGGGGGGMMMWWWWGMGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGMGMMMWWWW
echo                  WWMMMMMMMW.WWWWGGGGGGGGGGGGGGGGGGGGGGGGWW.WW......WW....WWGMMMMGGGGGGGGGGGGGGGGMMWRMWWMWGGGGGGGGGGGGGGWGGGGGGGGGGGGGGGGMMMMWWWW
echo                 WMMMMMMMW...WWWGGGGGGGGGGGGGGGGGGGGGGWWWWWWWWW....WWW....WWMMMMGGGGGGGGGGGGGGGGGGGGGGWWMWWWMMMMGGGGGGGWWGGGGGGGGGGGGGMMMWWWWWMWW
echo                 WWMMMMMMMW..WWWWGGGGGGGGGGGGGGGGGGGGGWWWW.....WWWWWWWWW....WWMMMMGGGGWWGGGGGGGGGGGGGGGGGGGGGWMWMMMMMMMMWWGGMMMWWWWWWWWWWWWWWMMMW
echo                WWMMMMMMMMW.WW.WWGGGGGGGGGGGGGGGGGGGWWWW............WWWWW....WWMMMMGGGWWWWGGGGGGGMWWGGGGGGGGGGGGGGGGGGGGGWWGWWGGWWWWWWWWWMGMMMMGW
echo               WWMMMMMMMMMW.W..WWGGGGGGGGGGGGGGGGGGWWWW................WWWW...WWMMMMMMMWWWWGGGGGGWWWGGGGGGGGGGGGGGGGGGGGGGWGGGGGGGGGGGGGGGGGGMMGW
echo               WWMGGGGGGMMW.WW..WGGGGGGGGGGGGGGGGGWWW.............WWWW...WWWWW.WWMMMMMMMW.WWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWWGGGGGGGGGGGGGGGGGGGGW
echo              WWMGGGGGGGGGW.WW..WGGGGGGGGGGGGGGGGWWWWWWWWWWW........WWWW..WW.WWWWWMMMMMMMW..WWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGWGGGGGGGGGGGGGGGGGGGGMW
echo             WWMGGGGGGGGGGGWW...WWGGGGGGGGGGGGGGGGWWWWWWWWWWWWWWW.....WWWW.WWW.W.WWWMMMMMMW...WWWWGGGGGGGGGGGGGGGGGGGGGGGGGWWGGGGGGGGGGGGGGGGGGGGW
echo            WWMGGGGGGGGGGGGWWW..WWGGGGGGGGGGGGGGGGWWWGGGMGGMMMMMWWWWWW.WWW...WWW...WWMMMMMMW....WWWWGGGGGGGGGGGGGGGGGGGGGGGWWGGGGGGGGGGGGGGGGGGGMW
echo           WWMGGGGGGGGGGGGGW.WW.WWGGGGGGGGGGGGGGGWWWWGGGWGGWWWGWWWWWWWWWWW.....WW...WWWMMMMMW.....WWWWWGGGGGGGGGGGGGGGGGGGGWGGGGGGGGGGGGGGGGGGGGGW
echo          WWGGGGGGGGGGGGGGGW.WWWWGGGGGGGGGGGGGGGWWWWWWWGGWWW.WWWW...WWWW.......WWWWW.WWWMMMMW...WW..WWWWGGGGGGGGGGGGGGGGGWWWWWWWWWWGGGGGGGGGGGGGW
echo          WWMGGGGGGGGGGGGGGGWW...WWGGGGGGGGGGGGGMWWWWWWWWWWWWWWWW.................WWW...WWMMMMWW........WWWWGGGGGGGGGGGGGGWWWWWWWWWGGGGGGGGGGGGGGWW
echo         WWGGGGGGGGGGGGGGGGGGGWW.WWGGGGGGGGGGGGGMW.WWWWWWWWWWWWW...................WWWW..WWWMMMWW....WWWWW.WWWWGMGGGGGGGGGWGGGGGGGGGGGGGGGGGGGGGGWW
echo        WWGGGGGGGGGGGGGGGGGGGGGWWWWWGGGGGGGGGGGMMW...WWWWWWWWW....................WWWWWWW.WWWWWMWW..WWWWWWWWWWWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWW
echo       WWGGGGGGGGGGGGGGGGGGGGGGGGWWWGGGGGGWWWGGMMW.......WWWWWWW..................WWWW..WWWWWWWWWMWWWWWWWWWWWWWWWWWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWW
echo      WWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWW.WWGMMW...............................WW.....WWWWWWW.WWGWGGGGGWWWWWWWWWWWWWWWWGGWGGGGGGGGGGGGGGGGGGGGGGWW
echo     WWWWWGGGGGGGGGGGGGGGGGGGGGGGGGWGGGGWW..WWGMW.............................WWW.......WW..WWWWWGWWWWWGWWW....WWWWW..WWGGGGGGGGGGGGGGGGGGGGGGGGGWW
echo    WWGWGGGGGGGGGGGGGGGGGGGGGGGGGGGWWGGW......WMWWW.......................WWWWW........WW.....WWWWWWWWWWWW.....WWWWWWWWWGGGGGGGGGGGGGGGGGGGGGGGGGWW
echo    WWGGGGGGGWGGGGGGGGGGGGGGGGGGGGGGWWWWW..........WWWWW................WWWWWW.........WW.....WWWWWWWWWW......WWW...WWWGGWGGGGGGGGGGGGGGGGGGGGGGW
echo   WWGGGGGGGGGKIZUMONOGATARIGGGGGGGGWWWW..............WWWWWWW...WWWWWWWWW..............WW......WWWWWWW.......WWWWW.WWWGGGGGGGGGGGGGGGGGGGGGGGGGGW
echo   WWGGWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWWWW...................WWWWWW......................WW........WWWWWW......WW.WWWWWWGGGGWGGGGGGGGGGGGGGGGGGGGWW
echo   WWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWWWWW.........................................W....WWW......................WWWWWGGGGGGGGGGGGGGGGGGGGGGGGGGW
echo    WWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWTW.WW.......................................WW.....WWW....................WWWWWGGGGGGGGGGGGGGWGGGGGGGGGGGGW
echo    WWGWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWTWW.WW......................................WW.......WWW..................WWWWWGGGGGGGGGGGGGWGGGGGGGGGGGGGW
echo     WWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGWO.WW.WW...................................WWW..........WWW...............WWWWWGWGGWGGGGGGGGGWGGGGGGGGGGGGGGW
echo      WWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGWO..WW.WW................................................WWWWW..........WWWWWWGGGGWGGGGGGGWWGGGGGGGGGGGGGGW
echo       WWGGGGGGGGGGGGGGGGGGGGGGGGWWWWWW...WW.WW...............WWW................................WWWWWWWWWWWWWWWWWGGGGWGGGGGGGWWGGGGGGGGGGGGGGGW
echo       WWWGGGGGGGGGGGGGGGGGGGGWWWWRRRRW....WW.WW...............WWW...................................WWWWWWWWGGGGGGGGGGGGGWGGGWGGGGWGGGGGGGGGGGW
echo        WWWGGGGGGGGGGGGGGGGGWWMRRRRRRRW.....WWWWW................WWWW...................................WWWGGGGGGGGGGGGGGGWGWGGGGGGGGGGGGGGGGGWW
echo         WWWGGGGGGGGGGGGGGGWWMRRRRRRRRW.......WW.WW.................WWWWWW......WW....................WWWGGGGGGGGGGWGGGGGGWWGGGGGGGGWWGGGGGGGGW
echo          WWWGGGGGGGGGGWWWWWRRRRRRRRRRW........WW.WW.....................WWWWWWWW...................WWWGGGGGGGGGGGGGGGGGGGGGGGGGGGWGGWGGGGGGGWW
echo          WWWWGGGGGWWWW..WWRRRRRRRRRRRWT.........WWWWW...........................................WWWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWGGGGGGGW
echo            WWWGWWWW....WWRRRRRRRRRRRRWW..........WWWWW.......................................WWWWRRRWWWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWGGGGGW
echo              WWW......WWRRRRRRRRRRRRWWW............WWWWW.................................WWWWWRRRRRRRRWWGGGGGGGGGGGGGGGGGGGGGGGGGGGGGWGGGGWW
echo         WWWWW.WW.....WWRRRRRRRRRRRRRWTW..............WWWWW.........................WWWWWWWRRRRRRRRRRWOWWWWWWGGGGGGGGGGGGGGGGGGGGGGGGGWGGGW
echo WWWWWWWWWW.W...W....WWWRRRRRRRRRRRRRRW.W................WWWWW.................WWWWWWWWWRRRRRRRRRRRRRAWWW...WWWWWGGGGGGGGGGGGGGGGGGGGGGWGGW
echo WW.............W...WWRRRRRRRRRRRRRRRWW.W...................WWWW.........WWWWWWW..WWWWRRRRRRRRRRRRRRATWW........WWWWWGGGGGGGGGGGGGGGGWGGWW
echo ...............WW.WWRRRRRRRRRRRRRRRRWT.W.....................WWWWWWWWWWWW......WWWWWRRRRRRRRRRRRRROTTW............WWWWWGGGGGGGGGGGGWGGWW
echo ................WWWRRRRRMMMRMMMRRRRWW..W....................................WWWWWWRRRRRRRRRRRRRWATTTW................WWWWWWGGGGGGGGGWW
echo ................WWWRRRMMMMMMMMMMMRMWT..W.................................WWWWWWWRRRRRRMRMMMRRRAOTTTWW....................WWWWWWWWWWWWWWW
echo ..............WWWRRRRMMMMMMMMMMMMMMWT.TW..............................WWWWTTWWRRRRRWWAARMMMWAOTTTTWW..................................WWWWWWWWWWWW
echo ..............WWWWWWWMMMMMMMMMMMMMMWT.WT............................WWW...TWWRRRRRAOOOAMMMRAOTTTTTW............................................WWW


pause >nul

goto ascii

:cmd
cls

start cmd /k "echo cmd succsesfully opened"

goto extra

:internet

echo.
timeout /t 1 /nobreak >nul
for /f "tokens=14" %%i in ('ipconfig ^| findstr /i "ipv4"') do set localip=%%i

for /f "delims=" %%i in ('powershell -command "(Invoke-WebRequest -Uri 'https://api.ipify.org').Content"') do set publicip=%%i


echo local ip address is: %localip%
echo public ip address is: %publicip%

timeout /t 99999 >nul
goto extra

:refresh
taskkill /im explorer.exe /f
del /a:h "%localappdata%\microsoft\windows\explorer\iconcache*" >nul 2>&1
start explorer.exe
echo icon cache refreshed.
pause >nul
goto extra

:colors
cls

echo select a text color
echo -1- black
echo -2- red
echo -3- green
echo -4- yellow
echo -5- blue
echo -6- magenta
echo -7- cyan
echo -8- white
set /p fgchoice=


echo select a background color
echo -1- black
echo -2- red
echo -3- green
echo -4- yellow
echo -5- blue
echo -6- magenta
echo -7- cyan
echo -8- white
set /p bgchoice=


if "%fgchoice%"=="b" call :extra
if "%fgchoice%"=="1" set fgcolor=0
if "%fgchoice%"=="2" set fgcolor=4
if "%fgchoice%"=="3" set fgcolor=2
if "%fgchoice%"=="4" set fgcolor=6
if "%fgchoice%"=="5" set fgcolor=1
if "%fgchoice%"=="6" set fgcolor=5
if "%fgchoice%"=="7" set fgcolor=3
if "%fgchoice%"=="8" set fgcolor=7

if "%bgchoice%"=="b" call :extra
if "%bgchoice%"=="1" set bgcolor=0
if "%bgchoice%"=="2" set bgcolor=4
if "%bgchoice%"=="3" set bgcolor=2
if "%bgchoice%"=="4" set bgcolor=6
if "%bgchoice%"=="5" set bgcolor=1
if "%bgchoice%"=="6" set bgcolor=5
if "%bgchoice%"=="7" set bgcolor=3
if "%bgchoice%"=="8" set bgcolor=7

color %bgcolor%%fgcolor%

cls
echo you have selected text color %fgchoice% and background color %bgchoice%.


pause >nul
goto extra

:taskmanager

:taskmenu
cls
echo.

echo                    95 task manager                    
echo. 
echo                        {b} menu                           
echo.
echo                        {1} processes                      
echo.
echo                        {2} kill process                   
echo.
echo                        {3} performance                    
echo.
echo                        {4} disable GUI 
set /p choice= 

if "%choice%"=="1" goto :processes
if "%choice%"=="3" goto :performance
if "%choice%"=="2" goto :killprocess
if "%choice%"=="4" goto :disablegui
if "%choice%"=="b" goto :menu
if /i "%choice%"=="f" powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"
if /i "%choice%"=="u" call :update
if /i "%choice%"=="e" call :elevate
if /i "%choice%"=="x" call :close
if /i "%choice%"=="a" call :advanced


goto :taskmenu

:taskmenu2
timeout /t 3 /nobreak >nul

echo {B} back
echo {K} kill process
set /p choice= 

if "%choice%"=="k" goto :killprocess
if "%choice%"=="b" goto :taskmenu

goto :taskmenu2

:processes
cls
echo                        processes
wmic process get description,processid | more
goto :taskmenu2

:performance
start cmd /k "call %~dp0wired.bat :a"
goto :taskmenu


:a


:a2
cls


for /f "tokens=2 delims==" %%a in ('wmic cpu get loadpercentage /value') do set cpu=%%a

for /f "tokens=2 delims==" %%a in ('wmic os get freephysicalmemory /value') do set freeram=%%a
for /f "tokens=2 delims==" %%a in ('wmic os get totalvisiblememorysize /value') do set totalram=%%a

set /a usedram=%totalram% - %freeram%
set /a freerammb=%freeram% / 1024
set /a usedrammb=%usedram% / 1024
set /a totalrammb=%totalram% / 1024

echo  cpu: %cpu%%  
echo  ram: %usedrammb%mb / %totalrammb%mb  

ping -n 2 127.0.0.1 >nul

goto :a2

:killprocess
cls

echo                        kill process 
echo input name of the process to kill (e.g cmd.exe)
echo input b to back
echo.
set /p processname=process: 

if /i "%processname%"=="b" goto taskmenu

taskkill /im "%processname%" /f >nul 2>&1

if %errorlevel%==0 (
    echo process "%processname%" has been killed.
) else (
    echo failed to kill process "%processname%" or it may not exist.
)

pause >nul
goto killprocess

:packages
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto extra)
cls
echo                        packages
echo.
echo  B back
echo  1 install choco (package manager) 
echo  2 install packages                
echo  3 usage      
echo  4 check packages                  
echo  5 uninstall choco  
echo  6 install WSL (linux subsystem for windows)               
echo. 
echo packages are listed in  "usage"
set /p choice=


if /i "%choice%"=="b" call :extra
if /i "%choice%"=="1" call :choco
if /i "%choice%"=="2" call :installpgk
if /i "%choice%"=="3" call :usage
if /i "%choice%"=="4" call :installed
if /i "%choice%"=="5" call :uninstallchoco
if /i "%choice%"=="6" call :wsl



goto packages

:choco
echo opening powershell to enable script execution and install chocolatey
start cmd /k "call %~dp0wired.bat :c"
pause >nul
goto :packages

:c
powershell -noexit -command ^
"set-executionpolicy remotesigned -scope currentuser -force; ^
iex ((new-object system.net.webclient).downloadstring('https://community.chocolatey.org/install.ps1')); ^
[system.environment]::setenvironmentvariable('path', $env:path + ';c:\programdata\chocolatey\bin', [system.environmentvariabletarget]::user); ^
echo installation complete
pause >nul
exit

:installpgk
cls

echo running as admin...

echo oopening powershell to install packages with chocolatey...
echo this may take some time depending on internet speed.


powershell -command "
    choco install nano -y --confirm --accept-license
    choco install wget -y --confirm --accept-license
    choco install git -y --confirm --accept-license
    choco install 7zip -y --confirm --accept-license
    choco install mediainfo -y --confirm --accept-license
    choco install vim -y --confirm --accept-license
    choco install squid -y --confirm --accept-license
    choco install poppler -y --confirm --accept-license
"

echo installation process completed
pause >nul
goto packages

:usage
cls
echo nano: terminal-based text editor. run: nano
echo wget: downloads files from the web. run: wget [url]
echo git: version control tool. run: git
echo 7zip: archive manager. run: 7z
echo vim: terminal based code/text editor run: vim
echo squid: hosting proxy run: squid
echo mediainfo: displays media file info. run: mediainfo
echo poppler: pdf rendering library. run: pdftoppm [file]
echo.
echo some programs like squid may need configuration
echo you can edit them eg: squid.conf with any text editor

pause >nul
goto packages

:installed
cls

choco list

pause >nul
goto packages

:f

echo faggot

timeout /t 99999 >nul
goto menu

:systeminfo
cls

systeminfo 

pause >nul
goto :extra

:uninstallchoco
start cmd /k "call %~dp0wired.bat :u"
goto packages

:u
echo running as admin...

choco uninstall chocolatey -y

rd /s /q c:\programdata\chocolatey

setx path "%path:c:\programdata\chocolatey\bin;=%"

echo chocolatey has been uninstalled successfully.
pause >nul
exit

:terminal
title terminal
cls
echo                 %date% %time%
echo.
echo  input B to go back to batch
echo  input C to clear screen cmd
echo  input F to shift fullscreen
echo  input CMD to detangle batch
echo.
goto :realterminal

:realterminal

set /p "devinput=%cd%>"

if "%devinput%"=="b" goto :menu
if "%devinput%"=="f" powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"
if "%devinput%"=="x" exit
if "%devinput%"=="c" cls & goto :terminal
if "%devinput%"=="a" (net session >nul 2>&1 && (call :advanced) || echo error: requires admin) & call :realterminal





%devinput%

call :realterminal

:windows
cls  
echo alt + enter  ^- fullscreen for command prompt  
echo alt + esc  ^- cycle through open windows  
echo alt + f4  ^- close active window/shutdown pc  
echo alt + left arrow  ^- go back in file explorer  
echo alt + right arrow  ^- go forward in file explorer  
echo alt + space  ^- open window menu  
echo alt + tab  ^- switch between open windows  
echo ctrl + a  ^- select all  
echo ctrl + alt + delete  ^- open security options  
echo ctrl + alt + esc  ^- open task manager directly  
echo ctrl + alt + up arrow  ^- rotate screen to normal  
echo ctrl + alt + down arrow  ^- rotate screen upside down  
echo ctrl + alt + left arrow  ^- rotate screen left  
echo ctrl + alt + right arrow  ^- rotate screen right  
echo ctrl + b  ^- bold text (in some apps)  
echo ctrl + c  ^- copy selected text  
echo ctrl + d  ^- delete selected file  
echo ctrl + e  ^- focus on search bar in file explorer  
echo ctrl + f  ^- find text in most applications  
echo ctrl + h  ^- open history in some browsers  
echo ctrl + i  ^- italic text (in some apps)  
echo ctrl + l  ^- highlight address bar in browsers  
echo ctrl + m  ^- enter move mode in command prompt  
echo ctrl + n  ^- open a new window in some apps  
echo ctrl + p  ^- print  
echo ctrl + r  ^- refresh the page  
echo ctrl + s  ^- save file  
echo ctrl + shift + esc  ^- open task manager  
echo ctrl + shift + n  ^- create new folder  
echo ctrl + v  ^- paste  
echo ctrl + w  ^- close current tab  
echo ctrl + x  ^- cut selected text or file  
echo ctrl + y  ^- redo last action  
echo ctrl + z  ^- undo last action  
echo f1  ^- open help  
echo f2  ^- rename selected file  
echo f3  ^- open search in file explorer  
echo f4  ^- focus on address bar in file explorer  
echo f5  ^- refresh page or folder  
echo f6  ^- cycle through elements in a window  
echo f10  ^- activate menu bar in some apps  
echo home  ^- move to beginning of line  
echo end  ^- move to end of line  
echo shift + delete  ^- permanently delete file  
echo shift + f10  ^- open right-click menu  
echo shift + tab  ^- move focus backward  
echo tab  ^- move focus forward  
echo windows + 1-9  ^- open taskbar programs by position  
echo windows + a  ^- open action center  
echo windows + ctrl + d  ^- create new virtual desktop  
echo windows + ctrl + left/right  ^- switch virtual desktops  
echo windows + ctrl + f4  ^- close virtual desktop  
echo windows + d  ^- show desktop  
echo windows + e  ^- open file explorer  
echo windows + g  ^- open game bar  
echo windows + h  ^- open voice typing  
echo windows + i  ^- open settings  
echo windows + k  ^- open connect menu  
echo windows + l  ^- lock computer  
echo windows + m  ^- minimize all windows  
echo windows + p  ^- project screen settings  
echo windows + r  ^- open run dialog  
echo windows + s  ^- open search  
echo windows + shift + s  ^- take screenshot (snipping tool)  
echo windows + t  ^- cycle through taskbar programs  
echo windows + u  ^- open accessibility settings  
echo windows + v  ^- open clipboard history  
echo windows + x  ^- open quick link menu  
echo windows + .  ^- open emoji panel  
echo windows + space  ^- switch keyboard language  
echo windows + tab  ^- open task view
echo.
echo fullscreen is reccomended

pause >nul
goto extra


:ddr
cls
echo  note that anything above ddr2 cannot be displayed due to batch code limitation
echo  but from mhz you might be able to determine the ddr
echo  ddr1 400-667
echo  ddr2 400-800
echo  ddr3 800-2133
echo  ddr4 1600-4000
echo  ddr5 4800-7200

echo  pc's ram speed is listed below each ram stick will be displayed
echo.
wmic memorychip get speed

set foundddr=0

for /f "skip=1" %%a in ('wmic memorychip get memorytype 2^>nul') do (
    set "ramtype=%%a"
    
    if "!ramtype!"=="20" (
        echo  ram type is ddr
        set foundddr=1
    )
    if "!ramtype!"=="21" (
        echo  ram type is ddr2
        set foundddr=1
    )
)

if "!foundddr!"=="0" (
    echo ram type is ddr3 or higher
)

pause >nul
goto :extra

:advanced 
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto extra)


cls
echo                        advanced
echo  B back
echo  F fix system
echo  S system copy
echo  P power options 
echo  W wake up on LAN
echo  R remote desktop connection
echo  D date and time configuration 

set /p choice=

if /i "%choice%"=="b" call :extra
if /i "%choice%"=="s" call :backup 
if /i "%choice%"=="f" call :repair 
if /i "%choice%"=="d" call :date   
if /i "%choice%"=="p" call :power
if /i "%choice%"=="r" call :rdp
if /i "%choice%"=="w" call :wakeuponlan
goto :advanced

:backup
cls


echo copying entire OS (C:) is not possible yet
echo. 
echo enter the path or drive to backup
echo do not enter drive letter without "\"
echo (e.g., C:\ , H:\hentai)

set /p source=

if /i "%source%"=="b" call :advanced

if not exist "%source%" (
    echo source path does not exist.
    pause >nul
goto backup
)


echo enter the destination directory (e.g., B:\ or any folder path)
set /p destination=

if /i "%destination%"=="b" call :advanced

if not exist "%destination%" (
    echo destination path does not exist.
    pause >nul
goto backup
)

echo choose backup type:
echo f folder
echo z zip file {need 7zip installed}
set /p backuptype=

if "%backuptype%"=="b" call :advanced
if "%backuptype%"=="f" goto :backup_folder
if "%backuptype%"=="z" goto :backup_zip

echo invalid selection
goto :backup

:backup_folder
echo name the folder
set /p foldername=

echo in 5 seconds copying files from 
echo "%source%" to "%destination%\%foldername%" 
echo hit ctrl + c to cancel
timeout /t 5 >nul

mkdir "%destination%\%foldername%"

robocopy "%source%" "%destination%\%foldername%" /mir /COPY:DAT /xj

echo operation completed

pause >nul
goto backup

:backup_zip
echo name the zip file (without extension)
set /p zipname=

echo in 5 seconds copying files from 
echo "%source%" to "%destination%\%zipname%" 
echo hit ctrl + c to cancel
timeout /t 5 >nul

echo compressing "%source%" to "%destination%\%zipname%.zip"...


"c:\program files\7-zip\7z.exe" a -tzip "%destination%\%zipname%.zip" "%source%\*" -mx9

if not exist "c:\program files\7-zip\7z.exe" (
    echo.
    echo 7zip path not found, install 7zip to program files
    pause >nul
goto backup
)

echo zip operation completed
goto backup

pause >nul
goto :backup


:repair

sfc /scannow
pause >nul
goto :advanced



:date

echo the date and time is %date% %time%
echo change? (y/n)
set /p choice=
if /i "%choice%" neq "y" goto :advanced

for /f "tokens=1 delims=/.- " %%a in ("%date%") do set delf=%%a
set /a delf=%delf% 2>nul
if %delf% gtr 12 (set fmt=ddmmyyyy) else (set fmt=mmddyyyy)

echo enter new date:
if "%fmt%"=="ddmmyyyy" (
    echo format: dd-mm-yyyy
) else (
    echo format: mm-dd-yyyy
)
set /p newdate=
date %newdate%

echo enter new time (hh:mm:ss):
set /p newtime=
time %newtime%

timeout /t 999 >nul
goto :advanced

:power
cls
echo                        power
echo  S shutdown
echo  Z Zzzzzzz
echo  R restart
echo  D display
echo  H hibernate
echo.
echo  B back
echo  SS schedule mode
set /p option=

if /i "%option%"=="s" goto :off
if /i "%option%"=="z" goto :sleep
if /i "%option%"=="r" goto :restart
if /i "%option%"=="b" goto :advanced
if /i "%option%"=="h" goto :hibernate
if /i "%option%"=="d" goto :displayoff
if /i "%option%"=="ss" goto :schedulepower
if /i "%option%"=="f" powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"

goto :power

:hibernate
powercfg /hibernate on
shutdown /h
goto :power

:off
echo on
shutdown /s /f /t 0

:restart
echo on
shutdown /r /f /t 0

:sleep
powercfg -hibernate off
rundll32.exe powrprof.dll,SetSuspendState Sleep
powercfg -hibernate on
goto :power

:displayoff
powershell -command "(Add-Type '[DllImport(\"user32.dll\")] public static extern bool PostMessage(int hWnd, int msg, int wParam, int lParam);' -Name 'User32' -Namespace 'Win32' -PassThru)::PostMessage(0xffff, 0x112, 0xF170, 2)"
goto :power

:update
echo checking for updates from https://github.com/bokuhikki/thewiredbatch

curl -s https://api.github.com/repos/bokuhikki/thewiredbatch/releases/latest > latest_release.json

for /f "delims=: tokens=2" %%a in ('findstr /i "tag_name" latest_release.json') do set latest_version=%%a

set latest_version=%latest_version:"=%
set latest_version=%latest_version:,=%
for /f "tokens=* delims= " %%b in ("%latest_version%") do set latest_version=%%b

set current_version=2
set current_version=%current_version: =%

echo current_version: %current_version%
echo latest_version: %latest_version%

del latest_release.json

if "%current_version%"=="%latest_version%" (
    echo no update yet :(
) else (
echo downloading the latest batch file...
curl -L -o wired.bat https://github.com/bokuhikki/thewiredbatch/releases/latest/download/wired.bat
echo update complete
)
pause >nul
goto :extra

:elevate
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo requesting administrator privileges
    powershell -command "start-process cmd -argumentlist '/c \"%~f0\"' -verb runas"
    exit
)

goto extra

:close
exit



:batchoptions
cls
echo                        batch options
echo  B back
echo  1 toggle echo
echo  2 add batch to startup
set /p option=

if /i "%option%"=="1" call :toggleecho
if /i "%option%"=="2" call :startup
if /i "%option%"=="B" call :extra

goto :batchoptions

:toggleecho
echo  1 on
echo  2 off
set /p option=

if /i "%option%"=="1" (
    @echo on
)
if /i "%option%"=="2" (
    @echo off
)

goto batchoptions

:startup
cls
echo  B back
echo  A add bacth to shell:startup
echo  R remove batch shell:startup
echo.
echo if asked dir or file input f

set /p option=

if /i "%option%"=="b" call :batchoptions
if /i "%option%"=="a" call :add
if /i "%option%"=="r" call :remove

goto startup

:add
set scriptname=%~nx0
xcopy "%~f0" "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\%scriptname%" /y

pause >nul 
goto startup

:remove
set scriptname=%~nx0
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\%scriptname%"
echo operation complete

pause >nul 
goto startup



:rdp
cls

:rdpmenu
echo  B back
echo  R rdp
echo  I instructions
echo  D display own info and enable rdp
set /p choice=

if /i "%choice%"=="r" call :remotedesktop
if /i "%choice%"=="i" call :rdpinstructions
if /i "%choice%"=="b" call :advanced
if /i "%choice%"=="d" call :rdpinfo

goto rdpmenu

:rdpinfo
echo opening info in new window

start cmd /k "call %~dp0wired.bat :d"

timeout /t 9999 >nul
goto rdp


:d
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server" /v fDenyTSConnections /t REG_DWORD /d 0 /f
netsh advfirewall firewall set rule group="Remote Desktop" new enable=Yes
cls

for /f "tokens=14" %%i in ('ipconfig ^| findstr /i "ipv4"') do set localip=%%i
echo.
echo.
echo        username: %USERNAME%
echo        ip adress: %localip%
echo.
echo             extra info
echo      hostname: %COMPUTERNAME%
echo      domain: %USERDOMAIN%\%USERNAME%
echo.
pause >nul
exit

:rdpinstructions
cls
echo  ip or hostname can be found with systeminfo command
echo   example:
echo   Host name: IDIOTSERVER
echo.
echo  {   IP address(es)   }
echo  {[01]: 192.981.60.100}
echo.
echo  rdp needs to be enabled on server
echo  to do that just type sconfig on server and enable Remote Desktop
echo  at sconfig you can also find useful other stuff, like domain
echo  to enable remote desktop on any pc copy this and paste into terminal as admin
echo.
echo  reg add "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server" /v fDenyTSConnections /t REG_DWORD /d 0 /f
echo  netsh advfirewall firewall set rule group="Remote Desktop" new enable=Yes
echo. 
echo  you dont have to do that to this pc batch will do that
echo  username and password just refers to the account on the server
echo  you cannot rdp into an account that does not have a password
echo  B to go back
set /p choice=

if /i "%choice%"=="b" call :rdp

goto :rdpinstructions

:remotedesktop
start cmd /k "call %~dp0wired.bat :r"
goto rdp

:r
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto rdp)


echo enabling rdp

reg add "HKLM\SYSTEM\CurrentControlSet\Control\Terminal Server" /v fDenyTSConnections /t REG_DWORD /d 0 /f
netsh advfirewall firewall set rule group="Remote Desktop" new enable=Yes

echo.

set /p rdp_ip=enter the ip or hostname:  
set /p rdp_user=enter username:  
set /p rdp_pass=enter user password:  

cmdkey /generic:%rdp_ip% /user:%rdp_user% /pass:%rdp_pass%
echo if you connected succsefully you may close this window
mstsc /v:%rdp_ip%
cmdkey /delete:%rdp_ip%

goto rdp

:timer
cls
start cmd /k "call %~dp0wired.bat :b"
goto extra

:b
cls
mode 15,4
set /p "hours=hours "
set /p "minutes=minutes "
set /p "seconds=seconds "
set /a totaltime=(%hours%*3600) + (%minutes%*60) + %seconds%

:countdown
if %totaltime% lss 1 goto bee
cls
set /a hoursleft=%totaltime% / 3600
set /a minutesleft=(%totaltime% %% 3600) / 60
set /a secondsleft=%totaltime% %% 60
echo time remaining
echo !hoursleft! : !minutesleft! : !secondsleft!
set /a totaltime=%totaltime%-1
timeout /t 1 >nul
goto countdown

:bee
cls
echo time is up
echo time is up
echo time is up

:beep
powershell -c "[console]::beep(1000,200)"
goto beep

:diskhealth
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto extra)


set /p disk=which disk would you like to scan (default C:)

if /i "%disk%"=="b" call :extra
if /i "%disk%"=="" chkdsk C: /r
if exist %disk% (
    chkdsk /r %disk%
    pause >nul
    goto extra
) else (
    echo invalid disk
    goto diskhealth
)
pause >nul
goto extra


echo this menu display all options in one window without extra sections

:troubleshoot
cls
echo         here are all found bugs and their fixes.
echo.
echo  B back
echo  F fullscreen
echo.
echo  1 The system cannot write to the specified device.
echo  2 font changing when fullscreen
echo  3 ECHO IS OFF at shortcuts
echo  4 some sections not opening in new window

set /p f=

if /i "%f%"=="1" call :error1
if /i "%f%"=="2" call :error2
if /i "%f%"=="3" call :error3
if /i "%f%"=="4" call :error4
if /i "%f%"=="b" call :extra
if /i "%f%"=="f" (
    powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"
    timeout /t 1 >nul
    cls
)

goto troubleshoot

:error1
cls
echo  The system cannot write to the specified device.
echo  this glitch has to do with the special characters
echo  to fix it you have to right click window
echo  properties
echo  disable "use legacy console"
echo  relaunch
echo  if the issue still presists 
echo  you are probably on an old OS

pause >nul
goto troubleshoot

:error2
cls
echo  no found patch yet
echo  the best you can do is set font to Consolas 14

pause >nul
goto troubleshoot

:error3
cls
echo  goto shorcuts
echo  input 'edit' to edit shortcuts.txt 
echo  remove the space in the beginning
echo  save

pause >nul
goto troubleshoot

:error4
cls
echo  if some section do not wanna open
echo  you probably renamed the batch to something else
echo  the script relies on the filename for opening new windows
echo  either rename it or duplicate and rename the duplicate back
pause >nul
goto troubleshoot

:schedulepower
cls
echo                        schedule mode
echo.
echo  S shutdown
echo  Z Zzzzzzzz
echo  R restart
echo  D display
echo  H hibernate
echo.
echo  B back
set /p option=

if /i "%option%"=="s" set optionvalue=off & goto timer2
if /i "%option%"=="z" set optionvalue=sleep & goto timer2
if /i "%option%"=="r" set optionvalue=restart & goto timer2
if /i "%option%"=="h" set optionvalue=hibernate & goto timer2
if /i "%option%"=="d" set optionvalue=displayoff & goto timer2
if /i "%option%"=="b" goto :power

goto schedulepower

:timer2
echo option: %optionvalue%

set /p "hours=hours "
set /p "minutes=minutes "
set /p "seconds=seconds "
set /a totaltime=(%hours%*3600) + (%minutes%*60) + %seconds%

:countdown2
if %totaltime% lss 1 goto %optionvalue%
cls
set /a hoursleft=%totaltime% / 3600
set /a minutesleft=(%totaltime% %% 3600) / 60
set /a secondsleft=%totaltime% %% 60
echo time remaining !hoursleft! : !minutesleft! : !secondsleft!
set /a totaltime=%totaltime%-1
timeout /t 1 >nul
goto countdown2


:rb
start cmd /k "call %~dp0wired.bat"

exit


:logs

:allforone

:calculator
cls

echo                        calculator
echo.
echo  P plus       M minus       D divide       X multiply
echo.
echo                                               C clear
echo                                               B back 

:calc


set /p choice= option? 
if /i "%choice%"=="p" call :plus
if /i "%choice%"=="m" call :minus
if /i "%choice%"=="d" call :division
if /i "%choice%"=="x" call :multiply
if /i "%choice%"=="c" call :calculator
if /i "%choice%"=="b" call :extra

goto :calc

:plus
set /p number1=
echo +
set /p number2=
set /a total = %number1% + %number2%
echo = %total%
goto :calc

:minus
set /p number1=
echo -
set /p number2=
set /a total = %number1% - %number2%
echo = %total%
goto :calc

:division
set /p number1=
echo /
set /p number2=
set /a total = %number1% / %number2%
echo = %total%
goto :calc

:multiply 
set /p number1=
echo x
set /p number2=
set /a total = %number1% * %number2%
echo = %total%
goto :calc

:disablegui
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto taskmanager)

cls
echo  this feature disables all GUI and only leaves cmd
echo  this can be useful for optimizing tasks or servers
echo.
echo  B back
echo  D disable gui
echo  E enable gui
set /p f=

if /i "%f%"=="d" (
    taskkill /f /im explorer.exe
    echo GUI has been disabled
    timeout /t 5 >nul
    goto disablegui
)

if /i "%f%"=="e" (
    start  explorer.exe 
    echo GUI has been enabled
    timeout /t 5 >nul
    goto disablegui
)

if /i "%f%"=="b" call :taskmanager


goto disablegui



:bootoptions
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto extra)
echo                        boot options
echo.
echo  R restart computer
echo  1 boot options
echo  2 set default: boot into graphical (normal)
echo  3 set default: boot into safe mode graphical
::  4 set default: boot into cmd non graphical

set /p f=

if /i "%f%"=="b" call :extra
if /i "%f%"=="r" shutdown /r /t 0
if /i "%f%"=="1" shutdown /r /o /f /t 0
if /i "%f%"=="2" bcdedit /deletevalue {default} safeboot & echo restart required
if /i "%f%"=="3" bcdedit /set {default} safeboot minimal & echo restart required

goto :bootoptions

:wsl
cls
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto packages)
echo                        windows subsystem for linux
echo.
echo  E enable   wsl
echo  D disable  wsl
echo  U update   wsl
echo  I instructions

set /p f=

if /i "%f%"=="b" call :packages
if /i "%f%"=="e" call :installwsl
if /i "%f%"=="d" call :uninstallwsl
if /i "%f%"=="i" call :wslinstructions
if /i "%f%"=="u" call :wslupdate

goto :wsl

:wslupdate
echo updating..
wsl.exe --update
wsl.exe --set-default-version 2
echo done
pause >nul
goto wsl

:installwsl
echo do you have virtulazation enabled in BIOS?
echo.
echo y  yes
echo n  no
echo w  what is bios
set /p f=

if /i "%f%"=="b" call :wsl
if /i "%f%"=="y" call :installwsl2
if /i "%f%"=="n" echo you need to have it enabled for this & pause >nul & goto wsl
if /i "%f%"=="w" call :wslinstructions

goto installwsl

:installwsl2
echo this can take a long while

powershell -Command Set-ExecutionPolicy RemoteSigned -Scope CurrentUser

:: developer mode
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\AppModelUnlock" /t REG_DWORD /v "AllowDevelopmentWithoutDevLicense" /d 1 /f

:: wsl
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart


:: hyperv
dism.exe /online /enable-feature /featurename:Microsoft-Hyper-V-All /norestart


echo.
echo restart needed to apply changes sometimes
echo.  
echo if you encountered any errors its either HyperV not working, vtx not being enabled or unsupported OS
echo OR just windows being windows
echo.
echo if there were no problems restart pc and go to instructions

pause >nul
goto wsl

:wslinstructions
cls
echo.
echo  if you have installed wsl you can now install a distro
echo  to list available distros do: wsl.exe --list --online
echo  to install a distro do:       wsl.exe --install -d (name)
echo  now you can have linux inside windows without limits
echo  you can even install a desktop environment or any app
echo.
echo  beware that you need wsl 2 not 1
echo  check wsl version with:  wsl --version
echo  distro versions also need to be 2 check with: wsl -l -v
echo  to set version for wsl: wsl --set-default-version 2
echo  to set version for a distro:  wsl --set-version (distro) 2
echo.
echo  if you are on version 1 and every command just brings up help
echo  uninstall wsl and try again otherwise check if os is supported
echo.
echo  what is bios? bios is pc configuration coded into the motherboard
echo  to access it and enable virtualization spam ESC before booting into os
echo  if ESC doesnt work search up computer model and how to access its bios
echo  you will find virtualization in the security tab
echo  if you can’t find vtx then the cpu most likely does not support vtx
echo.
echo  if you need further help do wsl.exe --help in terminal

pause >nul
goto wsl

:uninstallwsl

dism.exe /online /disable-feature /featurename:Microsoft-Windows-Subsystem-Linux /norestart
dism.exe /online /disable-feature /featurename:VirtualMachinePlatform /norestart
dism.exe /online /disable-feature /featurename:Microsoft-Hyper-V-All /norestart


echo optional: delete the folder related to your distro to remove leftover files
echo.
echo restart pc to apply changes

pause >nul
goto wsl

:wakeuponlan
cls
echo                        wake up on LAN
echo.
echo NOTE
echo for this to work you need to have wake up on LAN enabled in BIOS and settings
echo.

for /f "tokens=14" %%i in ('ipconfig ^| findstr /i "ipv4"') do set localip=%%i

for /f "tokens=2 delims==" %%a in ('wmic nic where "NetEnabled=true" get Name /value') do set adapter=%%a

echo %adapter%
getmac
echo %localip%
echo.
echo personal computer listed above (Ethernet, MAC, and IP)
echo input A to enter automated mode
echo input B to go back
echo input any other key to continue manual 
set /p n=
if /i "%n%"=="b" call :advanced
if /i "%n%"=="" call :lan
if /i "%n%"=="a" call :lanlist

:lan
echo.
echo.
echo.
echo.
echo enter the local ip of the computer to be woken up

set /p targetip=

echo enter the mac address of the target machine (e.g 00-1A-2B-3C-4D-5E)
echo you can aquire this by typing getmac into cmd
set /p targetmac=

echo sending magic packet to %targetip%

powershell -Command "$mac = '%targetmac%'; $broadcast = '%targetip%'; $macBytes = $mac -split '[:-]'; [byte[]]$magicPacket = (,0xFF * 6) + ($macBytes | ForEach-Object { [byte]('0x' + $_) }) * 16; $udpClient = New-Object System.Net.Sockets.UdpClient; $udpClient.Connect($broadcast, 9); $udpClient.Send($magicPacket, $magicPacket.Length); $udpClient.Close()"

echo operation complete
pause >nul
goto advanced

:lanlist
cls

if not exist LANlist.txt (
    cls
    echo no list found. creating LANlist.txt
    attrib +h LANlist.txt
    echo LANlist > LANlist.txt
    echo LANlist.txt has been created and set to hidden.
    pause >nul
)

:show_lanlist
cls
echo                            input 'create' to add a new 
echo                            input 'delete (name)' to remove
echo                            input 'edit' to edit LANlist.txt
echo.
echo ------------------------------------------------------------
for /f "tokens=1 delims=;" %%a in (LANlist.txt) do (
    echo %%a
)
echo ------------------------------------------------------------
echo.
echo input name to wake up
echo input B to go back
echo.
set /p shortcut_choice=

if /i "%shortcut_choice%"=="create" (
    call :create_lan
    goto show_lanlist
) else if "%shortcut_choice:~0,7%"=="delete " (
    call :delete_lan "%shortcut_choice:~7%"
    goto show_lanlist
) else if /i "%shortcut_choice%"=="b" (
    goto wakeuponlan
) else if /i "%shortcut_choice%"=="edit" (
    start notepad "%cd%\LANlist.txt" & goto show_lanlist
) else (
    call :wake_lan "%shortcut_choice%"
    goto show_lanlist
)


:create_lan
cls
set /p shortcutname="name the shortcut (input B to go back): "
if /i "%shortcutname%"=="B" goto show_lanlist

set /p targetmac="target mac (input B to go back): "
if /i "%targetmac%"=="B" goto show_lanlist

set /p targetip="target ip (input B to go back): "
if /i "%targetip%"=="B" goto show_lanlist

findstr /x /c:"%shortcutname%;%targetip%;%targetmac%" LANlist.txt >nul
if %errorlevel%==0 (
    echo a lan with that info already exists.
    pause >nul
    goto show_lanlist
)


echo %shortcutname%;%targetip%;%targetmac% >> LANlist.txt
echo LAN added successfully

pause >nul
goto show_lanlist

:delete_LAN
cls
set "delete_name=%~1"

findstr /c:"%delete_name%;" LANlist.txt >nul
if %errorlevel% neq 0 (
    echo LAN '%delete_name%' not found.
    pause >nul
    goto show_lanlist
)


findstr /v /c:"%delete_name%;" LANlist.txt > temp.txt
move /y temp.txt LANlist.txt >nul
echo shortcut '%delete_name%' deleted.
pause >nul
goto show_LANlist

:wake_LAN
cls
set "LAN_name=%~1"

for /f "tokens=1,2,3 delims=;" %%a in (LANlist.txt) do (
    if /i "%%a"=="%LAN_name%" (
        set targetip=%%b
        set targetmac=%%c
        goto :wakeup_magic_packet
    )
)

echo lan not found
pause >nul
goto show_LANlist

:wakeup_magic_packet
echo sending magic packet to %targetip%

powershell -Command "$mac = '%targetmac%'; $broadcast = '%targetip%'; $macBytes = $mac -split '[:-]'; [byte[]]$magicPacket = (,0xFF * 6) + ($macBytes | ForEach-Object { [byte]('0x' + $_) }) * 16; $udpClient = New-Object System.Net.Sockets.UdpClient; $udpClient.Connect($broadcast, 9); $udpClient.Send($magicPacket, $magicPacket.Length); $udpClient.Close()"
echo operation complete
pause >nul
goto show_LANlist

:extra2
cls
echo   sorry this is still under construction :3
pause >nul
goto extra

:extra
cls
echo  X close U update E elevate A advanced R relaunch F fullscreen T troubleshoot
echo.
echo.
echo                                 01  cmd
echo                                 02  ascii
echo                                 03  colors
echo                                 04  matrix
echo                                 05  ip check
echo                                 06  packages
echo                                 07  systeminfo
echo                                 08  batchoptions
echo                                 09  refresh explorer
echo                                 10  batch timer
echo                                 11  exact specs
echo                                 12  check ram
echo                                 13  calculator
echo                                 14  diskhealth
echo                                 15  networkshare
echo                                 16  windows hotkeys
echo                                 17  turn off display
echo                                 18  toggle network
echo                                 19  cmd commands
echo                                 20  explorer conf
echo                      (N) NEXT PAGE                 

set /p choice=                        

if /i "%choice%"=="f" powershell -Command "Add-Type -TypeDefinition 'using System; using System.Runtime.InteropServices; public class KeySimulator { [DllImport(\"user32.dll\", SetLastError = true)] public static extern uint keybd_event(byte bVk, byte bScan, uint dwFlags, int dwExtraInfo); public static void PressF11() { keybd_event(0x7A, 0, 0, 0); keybd_event(0x7A, 0, 2, 0); } }'; [KeySimulator]::PressF11()"
if /i "%choice%"=="b" call :menu
if /i "%choice%"=="n" call :extra2
if /i "%choice%"=="t" call :troubleshoot
if /i "%choice%"=="u" call :update
if /i "%choice%"=="r" call :rb
if /i "%choice%"=="e" call :elevate
if /i "%choice%"=="x" call :close
if /i "%choice%"=="a" call :advanced
if /i "%choice%"=="1" call :cmd
if /i "%choice%"=="2" call :ascii
if /i "%choice%"=="3" call :colors
if /i "%choice%"=="4" call :matrix
if /i "%choice%"=="5" call :internet
if /i "%choice%"=="6" call :packages
if /i "%choice%"=="7" call :systeminfo
if /i "%choice%"=="8" call :batchoptions
if /i "%choice%"=="9" call :refresh
if /i "%choice%"=="10" call :timer
if /i "%choice%"=="11" call :directx
if /i "%choice%"=="12" call :ddr
if /i "%choice%"=="13" call :calculator
if /i "%choice%"=="14" call :diskhealth
if /i "%choice%"=="15" call :networkshare
if /i "%choice%"=="16" call :windows
if /i "%choice%"=="17" powershell -command "(Add-Type '[DllImport(\"user32.dll\")] public static extern bool PostMessage(int hWnd, int msg, int wParam, int lParam);' -Name 'User32' -Namespace 'Win32' -PassThru)::PostMessage(0xffff, 0x112, 0xF170, 2)"
if /i "%choice%"=="18" call :disableinternet
if /i "%choice%"=="19" call :commandcommands
if /i "%choice%"=="20" call :explorerconf
goto extra

:disableinternet
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto extra)

cls
netsh interface show interface
echo B back
echo E enable network
echo D disable network
set /p f=

if /i "%f%"=="d" call :disablenet
if /i "%f%"=="e" call :enablenet
if /i "%f%"=="b" call :extra

goto disableinternet

:disablenet
echo.
echo type the name of personal computer interface listed above
echo.
set /p interface=

netsh interface set interface "%interface%" admin=disable
echo operation complete
pause >nul
goto disableinternet

:enablenet
echo.
echo type the name of personal computer interface listed above
echo.
set /p interface=

netsh interface set interface "%interface%" admin=enable
echo operation complete
pause >nul
goto disableinternet

:commandcommands
cls
echo.
echo some useful cmd commands
echo.
echo cls - clears screen
echo ^@echo off - hides cmd output
echo cd.. - goes back in path
echo cd (dir) - goes to a dir
echo dir - lists all directories
echo ^echo. - prints out a space
echo DISKPART - conf for disks
echo ^exit - exits cmd 
echo ^echo - prints text
echo ^mkdir (name) - makes a dir
echo ^rmdir (name) - removes a dir
echo ^del   (name) - deletes a file
echo ^ren (oldname) (newname) - renames file or dir
echo ^pause >nul - pauses script
echo ^goto (section) - goes to a section
pause >nul
goto extra

:explorerconf
cls
echo 1 show hidden files
echo 2 hide hidden files
echo 3 show system files
echo 4 hide system files
echo 5 show file extenstions
echo A all explorer settings
echo B back
set /p f=

@echo off

if /i "%f%"=="1" (
    powershell -Command "Set-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced' -Name Hidden -Value 1" & echo done & goto explorerconf
)

if /i "%f%"=="2" (
    powershell -Command "Set-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced' -Name Hidden -Value 2" & echo done & goto explorerconf
)

if /i "%f%"=="3" (
    powershell -Command "Set-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced' -Name ShowSuperHidden -Value 1" & echo done & goto explorerconf
)

if /i "%f%"=="4" (
    powershell -Command "Set-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced' -Name ShowSuperHidden -Value 0" & echo done & goto explorerconf
)

if /i "%f%"=="5" (
    powershell -Command "Set-ItemProperty -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced' -Name HideFileExt -Value 0" & echo done & goto explorerconf
)

if /i "%f%"=="a" control folders & goto explorerconf
if /i "%f%"=="b" goto extra 

goto exlorerconf

:networkshare
cls
echo 1 show shared folders
echo 2 share a dir/drive
echo 3 delete shares
echo 4 open a net share
echo 5 instructions
echo B back

set /p f=

if /i "%f%"=="1" net share & pause >nul & goto networkshare
if /i "%f%"=="2" call :share
if /i "%f%"=="3" call :deleteshares
if /i "%f%"=="4" call :opennetshare
if /i "%f%"=="5" call :howtonetshare
if /i "%f%"=="b" goto :extra
if /i "%f%"=="e" call :elevate

 
goto networkshare

:opennetshare
cls

echo note; you can find hostname or ip with systeminfo command
echo enter the shares computer name/hostname or local ip adress
set /p host=
echo note; you can find this with net share command
echo enter share name (leave blank if you want all shares)

set /p name=
echo.
echo note; instructions (5)
echo.
echo if acess is limited it will need user and password
echo if you did net share %name%=C:/ /grant:everyone,FULL or used wired choose N
echo enter credentials? (y/n/b)
set /p f=

if /i "%f%"=="y" call :netuser
if /i "%f%"=="n" call :opennetshareC
if /i "%f%"=="b" call :networkshare

:opennetshareC 
echo the path is \\%host%\%name%
echo this may take a while
net use z: \\%host%\
start  \\%host%\%name%
echo done
pause >nul
goto :networkshare

:netuser
echo the path is \\%host%\%name%
echo enter your user name
set /p user=
echo enter your password
set /p password=
echo this may take a while
net use z: \\%host%\
start \\%host%\%name% /user:%user% %password%
echo done 
pause >nul
goto :networkshare

:howtonetshare
cls
echo.
echo.
echo note; there is a built in way to open shares (4)
echo.
echo to open a shared folder simply do:
echo go to 'network' on file explorer if it does not appear do:
echo Win + R or file explorer network search bar
echo type the following:
echo \\192.158.58.110\ssd                    (this is an example)
echo replace the ip with the ip of the computer you used to share
echo entering PC name/hostname also works
echo replace 'ssd' with your share name (shown above)
echo dont forget to use '\' instead of '/'
echo you may get prompted to enter this PCs password
echo.
echo you could also connect using linux but you may need to install SMB
echo.
echo if you want to manaully share input this in cmd:
echo net share examplename=C:/users /grant:everyone,FULL
echo replace examplename with a name and C:/users with your path
pause >nul
goto networkshare

:share
echo enter path or drive you wanna share
set /p share=
echo give the share a name
set /p name=

if exist %share% (
net share %name%=%share% /grant:everyone,FULL
pause >nul
goto :networkshare
) else (
    echo invalid path
    goto networkshare
)

goto networkshare

:deleteshares
net session >nul 2>&1 || (echo this script requires admin privileges & pause >nul & goto networkshare)
cls
net share
echo this will delete shares not the actual files
echo.
echo enter share name to stop sharing input B to go back
set /p share=

if /i "%share%"=="b" call :networkshare

net share "%share%" /delete

pause >nul
goto :deleteshares
) else (
    echo invalid path & pause >nul
    goto deleteshares
)

goto deleteshares

:directx
echo opening directX diagnostics
dxdiag
echo done
pause >nul
goto extra