# Microsoft Developer Studio Project File - Name="billboard" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=billboard - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "billboard.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "billboard.mak" CFG="billboard - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "billboard - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "billboard - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "billboard - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\..\common" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX"dxstdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 d3dx9.lib d3d9.lib d3dxof.lib comctl32.lib winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /stack:0x200000,0x200000 /subsystem:windows /machine:I386 /libpath:"..\..\framework\lib" /ignore:4078 /ignore:4089
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "billboard - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "..\..\common" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX"dxstdafx.h" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 d3dx9dt.lib d3d9.lib d3dxof.lib comctl32.lib winmm.lib dxguid.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /stack:0x200000,0x200000 /subsystem:windows /debug /machine:I386 /libpath:"..\..\framework\lib" /ignore:4089 /ignore:4078

!ENDIF 

# Begin Target

# Name "billboard - Win32 Release"
# Name "billboard - Win32 Debug"
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\common\directx.ico
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\winmain.rc
# End Source File
# End Group
# Begin Group "Common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\common\d3dapp.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dapp.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d3denumeration.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d3denumeration.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dfile.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dfile.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dfont.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dfont.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dsettings.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dsettings.h
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\d3dutil.h
# End Source File
# Begin Source File

SOURCE=..\..\Common\dxstdafx.cpp

!IF  "$(CFG)" == "billboard - Win32 Release"

!ELSEIF  "$(CFG)" == "billboard - Win32 Debug"

# ADD CPP /Yc"dxstdafx.h"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\Common\dxstdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\common\dxutil.cpp
# End Source File
# Begin Source File

SOURCE=..\..\common\dxutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\billboard.cpp
# ADD CPP /YX
# End Source File
# Begin Source File

SOURCE=.\readme.txt
# End Source File
# End Target
# End Project
