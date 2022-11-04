SET PATH=D:\soft_dev\build_qt_devs\cmake-3.22.0\bin;%PATH%
SET PATH=D:\soft_dev\build_qt_devs\ninja-win;%PATH%
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -G Ninja "-DCMAKE_PREFIX_PATH=D:\qt\qt_5.15.2_x64\bin;D:\soft_dev\build_qt_devs\LLVM-15.0.2-win64" F:\qt_creator\qt-creator-opensource-src-8.0.0-rc1