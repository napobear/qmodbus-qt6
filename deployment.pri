win32 {
	dlls.path = $$PREFIX
	qt6platforms.path = $${dlls.path}/platforms

	MAIN_DLLS=Qt6Core Qt6Gui Qt6Widgets Qt6SerialPort libpng16-16
	for(dll, MAIN_DLLS): dlls.files += $$[QT_INSTALL_BINS]/$${dll}.dll

	dlls.files += /usr/x86_64-w64-mingw32/lib/zlib1.dll
	dlls.files += /usr/lib/gcc/x86_64-w64-mingw32/6.3-win32/libstdc++-6.dll
	dlls.files += /usr/lib/gcc/x86_64-w64-mingw32/6.3-win32/libgcc_s_seh-1.dll

	# add required Qt plugin DLLs
	qt6platforms.files += $$[QT_INSTALL_PLUGINS]/platforms/qwindows.dll

	INSTALLS += dlls qt6platforms
}

unix {
	isEmpty(PREFIX) {
		PREFIX = /usr/local/bin
	}
}

target.path = $$PREFIX

INSTALLS += target
