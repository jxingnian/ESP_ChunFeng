@echo off
chcp 65001 >nul
echo ========================================
echo    Cursor 缓存清理工具 v1.0
echo    清理缓存但保留登录和配置信息
echo ========================================
echo.

:: 检查Cursor是否正在运行
tasklist /fi "imagename eq Cursor.exe" 2>nul | find /i "Cursor.exe" >nul
if %errorlevel% == 0 (
    echo [警告] 检测到Cursor正在运行！
    echo 请先关闭Cursor后再运行此脚本。
    echo.
    pause
    exit /b 1
)

echo [信息] 开始清理Cursor缓存...
echo.

:: 设置Cursor用户数据目录
set "CURSOR_DATA_DIR=%APPDATA%\Cursor"
set "CURSOR_CACHE_DIR=%APPDATA%\Cursor\User"

if not exist "%CURSOR_DATA_DIR%" (
    echo [错误] 未找到Cursor数据目录: %CURSOR_DATA_DIR%
    echo 请确认Cursor已正确安装
    pause
    exit /b 1
)

echo [信息] Cursor数据目录: %CURSOR_DATA_DIR%
echo.

:: 创建备份目录（可选）
set "BACKUP_DIR=%~dp0cursor_backup_%date:~0,4%%date:~5,2%%date:~8,2%"
echo [信息] 创建备份目录: %BACKUP_DIR%
if not exist "%BACKUP_DIR%" mkdir "%BACKUP_DIR%"

:: 备份重要配置文件
echo [信息] 备份重要配置文件...
if exist "%CURSOR_CACHE_DIR%\settings.json" (
    copy "%CURSOR_CACHE_DIR%\settings.json" "%BACKUP_DIR%\" >nul 2>&1
    echo   - settings.json 已备份
)

:: 清理可以安全删除的缓存目录
echo.
echo [信息] 开始清理缓存文件...

:: 清理工作区缓存
if exist "%CURSOR_CACHE_DIR%\workspaceStorage" (
    echo   - 清理工作区缓存...
    rd /s /q "%CURSOR_CACHE_DIR%\workspaceStorage" 2>nul
)

:: 清理扩展缓存
if exist "%CURSOR_CACHE_DIR%\CachedExtensions" (
    echo   - 清理扩展缓存...
    rd /s /q "%CURSOR_CACHE_DIR%\CachedExtensions" 2>nul
)

:: 清理日志文件
if exist "%CURSOR_CACHE_DIR%\logs" (
    echo   - 清理日志文件...
    rd /s /q "%CURSOR_CACHE_DIR%\logs" 2>nul
)

:: 清理临时文件
if exist "%CURSOR_CACHE_DIR%\CachedData" (
    echo   - 清理缓存数据...
    rd /s /q "%CURSOR_CACHE_DIR%\CachedData" 2>nul
)

:: 清理崩溃报告
if exist "%CURSOR_CACHE_DIR%\CrashDumps" (
    echo   - 清理崩溃报告...
    rd /s /q "%CURSOR_CACHE_DIR%\CrashDumps" 2>nul
)

:: 清理GPU缓存
if exist "%CURSOR_CACHE_DIR%\GPUCache" (
    echo   - 清理GPU缓存...
    rd /s /q "%CURSOR_CACHE_DIR%\GPUCache" 2>nul
)

:: 清理代码缓存
if exist "%CURSOR_CACHE_DIR%\CachedExtensionVSIXs" (
    echo   - 清理扩展VSIX缓存...
    rd /s /q "%CURSOR_CACHE_DIR%\CachedExtensionVSIXs" 2>nul
)

:: 清理临时下载文件
if exist "%CURSOR_CACHE_DIR%\Downloads" (
    echo   - 清理下载缓存...
    rd /s /q "%CURSOR_CACHE_DIR%\Downloads" 2>nul
)

:: 清理网络缓存
if exist "%CURSOR_CACHE_DIR%\Network Persistent State" (
    echo   - 清理网络持久化状态...
    del /f /q "%CURSOR_CACHE_DIR%\Network Persistent State" 2>nul
)

:: 清理会话存储（但保留用户设置）
if exist "%CURSOR_CACHE_DIR%\Session Storage" (
    echo   - 清理会话存储...
    rd /s /q "%CURSOR_CACHE_DIR%\Session Storage" 2>nul
)

:: 清理本地存储中的临时数据（小心处理，保留登录信息）
if exist "%CURSOR_CACHE_DIR%\Local Storage" (
    echo   - 选择性清理本地存储...
    :: 只删除明确的缓存文件，保留登录相关的存储
    for /f "delims=" %%i in ('dir /b "%CURSOR_CACHE_DIR%\Local Storage\*cache*" 2^>nul') do (
        del /f /q "%CURSOR_CACHE_DIR%\Local Storage\%%i" 2>nul
    )
    for /f "delims=" %%i in ('dir /b "%CURSOR_CACHE_DIR%\Local Storage\*temp*" 2^>nul') do (
        del /f /q "%CURSOR_CACHE_DIR%\Local Storage\%%i" 2>nul
    )
)

:: 清理系统临时文件中的Cursor相关文件
echo   - 清理系统临时文件...
if exist "%TEMP%\cursor*" (
    del /f /q "%TEMP%\cursor*" 2>nul
)

echo.
echo [成功] 缓存清理完成！
echo.
echo ========================================
echo 保留的重要文件:
echo   - 用户设置和配置
echo   - 扩展安装信息
echo   - 登录凭据
echo   - 工作区配置
echo.
echo 已清理的内容:
echo   - 工作区缓存
echo   - 扩展缓存  
echo   - 日志文件
echo   - 临时数据
echo   - GPU缓存
echo   - 网络缓存
echo ========================================
echo.
echo [提示] 重启Cursor以获得最佳性能
echo [提示] 备份文件保存在: %BACKUP_DIR%
echo.
pause

