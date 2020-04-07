# Microsoft Developer Studio Generated NMAKE File, Based on PixelMotionBlur.dsp
!IF "$(CFG)" == ""
CFG=PixelMotionBlur - Win32 Release
!MESSAGE No configuration specified. Defaulting to PixelMotionBlur - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "PixelMotionBlur - Win32 Release" && "$(CFG)" != "PixelMotionBlur - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "PixelMotionBlur.mak" CFG="PixelMotionBlur - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PixelMotionBlur - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PixelMotionBlur - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "PixelMotionBlur - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\PixelMotionBlur.exe"


CLEAN :
	-@erase "$(INTDIR)\PixelMotionBlur.obj"
	-@erase "$(INTDIR)\d3dapp.obj"
	-@erase "$(INTDIR)\d3dfile.obj"
	-@erase "$(INTDIR)\d3dfont.obj"
	-@erase "$(INTDIR)\d3dutil.obj"
	-@erase "$(INTDIR)\dxutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\winmain.res"
	-@erase "$(OUTDIR)\PixelMotionBlur.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Fp"$(INTDIR)\PixelMotionBlur.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winmain.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\PixelMotionBlur.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=d3dx8.lib d3d8.lib d3dxof.lib winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\PixelMotionBlur.pdb" /machine:I386 /out:"$(OUTDIR)\PixelMotionBlur.exe" /libpath:"..\..\framework\lib" /stack:0x200000,0x200000
LINK32_OBJS= \
	"$(INTDIR)\d3dapp.obj" \
	"$(INTDIR)\d3dfile.obj" \
	"$(INTDIR)\d3dfont.obj" \
	"$(INTDIR)\d3dutil.obj" \
	"$(INTDIR)\dxutil.obj" \
	"$(INTDIR)\PixelMotionBlur.obj" \
	"$(INTDIR)\winmain.res"

"$(OUTDIR)\PixelMotionBlur.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "PixelMotionBlur - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\PixelMotionBlur.exe"


CLEAN :
	-@erase "$(INTDIR)\PixelMotionBlur.obj"
	-@erase "$(INTDIR)\d3dapp.obj"
	-@erase "$(INTDIR)\d3dfile.obj"
	-@erase "$(INTDIR)\d3dfont.obj"
	-@erase "$(INTDIR)\d3dutil.obj"
	-@erase "$(INTDIR)\dxutil.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\winmain.res"
	-@erase "$(OUTDIR)\PixelMotionBlur.exe"
	-@erase "$(OUTDIR)\PixelMotionBlur.ilk"
	-@erase "$(OUTDIR)\PixelMotionBlur.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "..\..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /Fp"$(INTDIR)\PixelMotionBlur.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winmain.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\PixelMotionBlur.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=d3dx8dt.lib d3d8.lib d3dxof.lib winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\PixelMotionBlur.pdb" /debug /machine:I386 /out:"$(OUTDIR)\PixelMotionBlur.exe" /libpath:"..\..\framework\lib" /stack:0x200000,0x200000
LINK32_OBJS= \
	"$(INTDIR)\d3dapp.obj" \
	"$(INTDIR)\d3dfile.obj" \
	"$(INTDIR)\d3dfont.obj" \
	"$(INTDIR)\d3dutil.obj" \
	"$(INTDIR)\dxutil.obj" \
	"$(INTDIR)\PixelMotionBlur.obj" \
	"$(INTDIR)\winmain.res"

"$(OUTDIR)\PixelMotionBlur.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("PixelMotionBlur.dep")
!INCLUDE "PixelMotionBlur.dep"
!ELSE 
!MESSAGE Warning: cannot find "PixelMotionBlur.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "PixelMotionBlur - Win32 Release" || "$(CFG)" == "PixelMotionBlur - Win32 Debug"
SOURCE=.\winmain.rc

"$(INTDIR)\winmain.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=..\..\common\d3dapp.cpp

"$(INTDIR)\d3dapp.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\d3dfile.cpp

"$(INTDIR)\d3dfile.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\d3dfont.cpp

"$(INTDIR)\d3dfont.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\d3dutil.cpp

"$(INTDIR)\d3dutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\..\common\dxutil.cpp

"$(INTDIR)\dxutil.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\PixelMotionBlur.cpp

"$(INTDIR)\PixelMotionBlur.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

