@echo off     

set HAWKSDK_HOME=E:\Program\HawkSDK
set PROJECT_HOME=E:\Program\HawkProject

echo ����HawkSDK�����ļ���
mkdir 	 %HAWKSDK_HOME%
mkdir 	 %HAWKSDK_HOME%\include
mkdir 	 %HAWKSDK_HOME%\lib
mkdir 	 %HAWKSDK_HOME%\bin

echo ����HawkSDK�ļ�
cd		 %PROJECT_HOME%
xcopy /y .\HawkUtil\Hawk*.h  			%HAWKSDK_HOME%\include\
xcopy /y .\HawkLog\Hawk*.h  			%HAWKSDK_HOME%\include\
xcopy /y .\HawkWin32\Hawk*.h  			%HAWKSDK_HOME%\include\
xcopy /y .\HawkMemLeak\Hawk*.h  		%HAWKSDK_HOME%\include\
xcopy /y .\HawkGeometry\Hawk*.h  		%HAWKSDK_HOME%\include\
xcopy /y .\HawkProfiler\Hawk*.h     	%HAWKSDK_HOME%\include\
xcopy /y .\HawkGateway\Hawk*.h      	%HAWKSDK_HOME%\include\
xcopy /y .\HawkRedis\Hawk*.h      		%HAWKSDK_HOME%\include\

xcopy /y .\Debug\HawkUtil_d.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkLog_d.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkWin32_d.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkMemLeak.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkGeometry_d.lib 	%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkProfiler_d.lib 	%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkGateway_d.lib  	%HAWKSDK_HOME%\lib
xcopy /y .\Debug\HawkRedis_d.lib  		%HAWKSDK_HOME%\lib

xcopy /y .\Debug\HawkMemLeak.dll  		%HAWKSDK_HOME%\bin

xcopy /y .\Release\HawkUtil.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Release\HawkLog.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Release\HawkWin32.lib  		%HAWKSDK_HOME%\lib
xcopy /y .\Release\HawkGeometry.lib 	%HAWKSDK_HOME%\lib
xcopy /y .\Release\HawkProfiler.lib 	%HAWKSDK_HOME%\lib
xcopy /y .\Release\HawkGateway.lib  	%HAWKSDK_HOME%\lib
xcopy /y .\Release\HawkRedis.lib  		%HAWKSDK_HOME%\lib

xcopy /y .\HawkUtil\win_bin\*.dll  		%HAWKSDK_HOME%\bin

xcopy /y .\Release\ProtocolGen.exe  	%HAWKSDK_HOME%\bin
xcopy /y .\Release\ProcMonitor.exe  	%HAWKSDK_HOME%\bin
xcopy /y .\Release\LogServer.exe    	%HAWKSDK_HOME%\bin
xcopy /y .\Release\DomainSvr.exe    	%HAWKSDK_HOME%\bin
xcopy /y .\Release\GateServer.exe   	%HAWKSDK_HOME%\bin
xcopy /y .\Release\ProfilerMonitor.exe  %HAWKSDK_HOME%\bin

echo ����HawkSDK���    
echo. & pause  
