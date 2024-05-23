cd thirdparty

copy /Y tiff-4.2.0\libtiff\tif_config.vc.h tiff-4.2.0\libtiff\tif_config.h
copy /Y tiff-4.2.0\libtiff\tiffconf.vc.h tiff-4.2.0\libtiff\tiffconf.h
copy /Y libpng-1.6.21\scripts\pnglibconf.h.prebuilt libpng-1.6.21\pnglibconf.h

cd ../toonz

IF NOT EXIST build mkdir build
cd build

REM Setup for local builds
set MSVCVERSION="Visual Studio 16 2019"
set BOOST_ROOT=C:\boost\boost_1_74_0
set OPENCV_DIR=C:\opencv\451\build
REM set QT_PATH=C:\Qt\5.9.7\msvc2019_64
set QT_PATH=C:\Qt\5.15.2_wintab\msvc2019_64

REM These are effective when running from Actions
IF EXIST C:\local\boost_1_74_0 set BOOST_ROOT=C:\local\boost_1_74_0
IF EXIST C:\tools\opencv set OPENCV_DIR=C:\tools\opencv\build
REM IF EXIST D:\a\tahoma2d\tahoma2d\thirdparty\qt\5.9\msvc2019_64 (
REM 	set QT_PATH=D:\a\tahoma2d\tahoma2d\thirdparty\qt\5.9\msvc2019_64
REM )

set WITH_WINTAB=Y
IF EXIST D:\a\tahoma2d\tahoma2d\thirdparty\qt\5.15.2_wintab\msvc2019_64 (
	set QT_PATH=D:\a\tahoma2d\tahoma2d\thirdparty\qt\5.15.2_wintab\msvc2019_64
)

set WITH_CANON=N
IF EXIST ..\..\thirdparty\canon\Header set WITH_CANON=Y

set WITH_GPHOTO2=N
IF EXIST ..\..\thirdparty\libgphoto2\include set WITH_GPHOTO2=Y

cmake ..\sources -G %MSVCVERSION%  -Ax64 -DQT_PATH=%QT_PATH% -DBOOST_ROOT=%BOOST_ROOT% -DOpenCV_DIR=%OPENCV_DIR% -DWITH_CANON=%WITH_CANON% -DWITH_GPHOTO2=%WITH_GPHOTO2% -DWITH_WINTAB=%WITH_WINTAB%


IF EXIST C:\ProgramData\chocolatey\bin\cl.exe (
 	msbuild /p:CLToolPath=C:\ProgramData\chocolatey\bin /p:UseMultiToolTask=true /p:Configuration=RelWithDebInfo /m /verbosity:minimal ALL_BUILD.vcxproj
) ELSE (
	msbuild /p:Configuration=RelWithDebInfo /m /verbosity:minimal ALL_BUILD.vcxproj
)

cd ../..
