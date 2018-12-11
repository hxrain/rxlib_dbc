pexports.exe -v lib_w32\libpq.dll > lib_w32\libpq.def
dlltool --dllname lib_w32\libpq.dll --def lib_w32\libpq.def --output-lib lib_w32\gcc\libpq.a

pexports.exe -v lib_w64\libpq.dll > lib_w64\libpq.def
dlltool --dllname lib_w64\libpq.dll --def lib_w64\libpq.def --output-lib lib_w64\gcc\libpq.a