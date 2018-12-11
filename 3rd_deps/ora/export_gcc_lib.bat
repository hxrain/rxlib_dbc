pexports.exe -v lib_w32\oci.dll > lib_w32\oci.def
dlltool --dllname lib_w32\oci.dll --def lib_w32\oci.def --output-lib lib_w32\gcc\liboci.a

pexports.exe -v lib_w64\oci.dll > lib_w64\oci.def
dlltool --dllname lib_w64\oci.dll --def lib_w64\oci.def --output-lib lib_w64\gcc\liboci.a