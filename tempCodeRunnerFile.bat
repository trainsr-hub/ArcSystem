@echo off
echo ==========================================
echo   KHOI DONG RABBIT-TANK SCHEDULER SYSTEM
echo ==========================================

:: 1. Mo Backend trong mot cua so moi
start "BACKEND - Node.js" cmd /k "cd smart-scheduler-server && echo Dang khoi dong Backend... && node server.js"

:: 2. Doi mot chut de Backend san sang (khoang 3 giay)
timeout /t 3 /nobreak > nul

:: 3. Mo Frontend trong mot cua so moi
start "FRONTEND - React" cmd /k "cd Front-End/smart-scheduler-client && echo Dang khoi dong Frontend... && npm start"

echo ------------------------------------------
echo He thong dang duoc kich hoat tren 2 cua so rieng biet.
echo Backend: localhost:5000
echo Frontend: localhost:3000
echo ------------------------------------------
pause


