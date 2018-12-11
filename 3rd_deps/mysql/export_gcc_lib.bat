pexports.exe -v lib_w32\libmysql.dll > lib_w32\libmysql.def
dlltool --dllname lib_w32\libmysql.dll --def lib_w32\libmysql.def --output-lib lib_w32\gcc\libmysql.a

pexports.exe -v lib_w64\libmysql.dll > lib_w64\libmysql.def
dlltool --dllname lib_w64\libmysql.dll --def lib_w64\libmysql.def --output-lib lib_w64\gcc\libmysql.a