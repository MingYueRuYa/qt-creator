md qtcreator_build
cd qtcreator_build

SET PATH=D:\soft_dev\build_qt_devs\cmake-3.22.0\bin;%PATH%
SET PATH=D:\soft_dev\build_qt_devs\ninja-win;%PATH%
SET PATH=C:\python3.10.6_amd64;%PATH%

cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja "-DCMAKE_PREFIX_PATH=D:\qt\qt_5.15.2_x64\bin;D:\soft_dev\build_qt_devs\LLVM-15.0.2-win64" F:\qt_creator\qt-creator-opensource-src-8.0.0-rc1

REM 编译
REM cmake --build . 
REM cmake --install . --prefix c:\custom_qt_creator --component Dependencies 
REM 在执行安装脚本的时候
REM F:\qt_creator\qt-creator-opensource-src-8.0.0-rc1\qtcreator_build>cmake --install . --prefix c:\custom_qt_creator --component Dependencies
REM -- Install configuration: "RelWithDebInfo"
REM 'C:/python3.10.6_amd64/python.exe' 'F:/qt_creator/qt-creator-opensource-src-8.0.0-rc1/scripts/deployqt.py' 'c:\custom_qt_creator/bin/qtcreator' 'D:/qt/qt_5.15.2_x64/bin/bin/qmake.exe'
REM Cannot find Qt Creator binary.