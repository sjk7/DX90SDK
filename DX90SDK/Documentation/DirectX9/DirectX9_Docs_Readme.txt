
            Microsoft DirectX 9.0 SDK Readme File
                        September 2003
  (c) Microsoft Corporation, 2003. All rights reserved.

This document provides late-breaking or other information that supplements the Microsoft DirectX 9.0 software development kit (SDK) documentation.

------------------------
How to Use This Document
------------------------

To view the Readme file on-screen in Windows Notepad, maximize the Notepad window. On the Format menu, click Word Wrap. To print the Readme file, open it in Notepad or another word processor, and then use the Print command on the File menu.


---------
CONTENTS
---------

1.  PRINTING ISSUES
 
2.  DIRECTX GRAPHICS

    2.1  High-Level Shader Language Updates
    2.2  Assembly Shader Updates
    2.3  Shader Debugging on Multi-threaded Applications
    2.4  Tutorial #3 Performance
    2.5  CopyRects Removed
    2.6  ID3DXPatchMesh::TessellateAdaptive Method
    2.7  ID3DXBuffer Interface
     

3.  MANAGED CODE
    
    3.1  Viewing Documentation for DirectX for Managed Code
    3.2  Samples
    3.3  New Book


-------------------
1.   PRINTING ISSUES

     If you are trying to print a heading and all of its subtopics, there are several known issues.
     - The pages will have no formatting.
     - If you are using Microsoft Internet Explorer 5.5, content may be missing from your printed pages. 
       Use Internet Explorer 5.0 or 6.0 instead. The recommended printing method is to print one topic at a
	   time. The printed page should then contain all the formatting and content.



2.  DIRECTX GRAPHICS


     2.1  High-Level Shader Language Updates

     The high-level shader language (HLSL) reference material is now broken into a section of its own, in the Shader reference section.


     2.2 Assembly Shader Updates

     The documentation has been revised to show the shader instructions and registers in the TOC. Software only versions, vs_2_sw, vs_3_sw, ps_2_sw, and ps_3_sw are supported for software vertex processing, or the reference rasterizer. 
 

     2.3  Shader Debugging on Multi-threaded Applications
     
     Multi-threaded shaders applications that use a global render function will not work properly with the Visual Studio shader debugger.


     2.4  Tutorial #3 Performance

     You might experience unusually poor performance when running tutorial #3 because of the D3DXMatrixRotation function's use of the sin function. The sin(x) function outputs very rough numbers when the input gets far out of its domain of -2 Pi to 2 Pi. So instead of using D3DXMatrixRotationY( &matWorld, timeGetTime()/150.0f ); in line 156, use the mod function to clamp the range like this:

     D3DXMatrixRotationY( &matWorld, (timeGetTime()%((int)(150.0f * 2.0f * 3.1415926f)))/150.0f );

     The period of rotation is then clamped between -2 Pi and +2 Pi.


     2.5 CopyRects Removed

     IDirect3DDevice9::CopyRects has been removed. Some of the functionality that this method offered is available through two new methods: IDirect3DDevice9::UpdateSurface and IDirect3DDevice9::GetRenderTargetData.

     UpdateSurface enables the transfer of data from a system memory surface to a video memory surface. GetRenderTargetData enables the transfer of data from a render target surface to a system memory surface.  If an application needs to transfer data from a video memory texture (or cubemap surface) to a system memory surface, it can set the D3DUSAGE_DYNAMIC flag when creating the resource, thus enabling locking of the component surfaces.  (Note that many display adapter drivers do not support the creation of dynamic resources.)  Alternatively, the application can proceed indirectly by rendering the texture (or cubemap surface) and applying GetRenderTargetData to the render target.


     2.6 ID3DXPatchMesh::TessellateAdaptive Method
     
     The syntax and parameters given in the documentation for this method are incorrect. The documentation should read as follows:    
     
        ID3DXPatchMesh::TessellateAdaptive Method
        -----------------------------------------
        
        Performs adaptive tessellation based on the z-based adaptive tessellation criterion.

        Syntax
     
            HRESULT TessellateAdaptive(      
            const D3DXVECTOR4* pTrans,
            DWORD dwMaxTessLevel,
            DWORD dwMinTessLevel,
            LPD3DXMESH pMesh
            );
        
        Parameters

            pTrans 
                [in] Specifies a 4-D vector that is dotted with the vertices to get the per-vertex adaptive tessellation amount. Each edge is tessellated to the average value of the tessellation levels for the two vertices it connects.
            dwMaxTessLevel 
                [in] Maximum limit for adaptive tessellation. This is the number of vertices introduced between existing vertices. This integer value can range from 1 to 32, inclusive.
            dwMinTessLevel 
                [in] Minimum limit for adaptive tessellation. This is the number of vertices introduced between existing vertices. This integer value can range from 1 to 32, inclusive.
            pMesh
            [in] Resulting tessellated mesh. See ID3DXMesh.

        Return Value

            If the method succeeds, the return value is D3D_OK.

            If the method fails, the return value can be one of the following:
            
            D3DERR_INVALIDCALL  The method call is invalid. For example, a method's parameter may have an invalid value.
            E_OUTOFMEMORY       Microsoft® Direct3D® could not allocate sufficient memory to complete the call.

        Remarks

            This function will perform more efficiently if the patch mesh has been optimized using ID3DXPatchMesh::Optimize. 


     2.7 ID3DXBuffer Interface
     
     The header given in the documentation for this interface is incorrect. The ID3DXBuffer interface is declared in the d3dx9core.h header file.

     In the Remarks section of the documentation, the LPD3DXBUFFER type definition should read as follows:

        The LPD3DXBUFFER type is defined as a pointer to the ID3DXBuffer interface.

          typedef interface ID3DXBuffer ID3DXBuffer;
          typedef interface ID3DXBuffer *LPD3DXBUFFER;



3.   MANAGED CODE

     3.1  Viewing Documentation for DirectX for Managed Code
     
     Stand-alone help documentation for managed code is now available. Double-click the directx9_m.chm file at DX90SDK\Documentation\DirectX9\. Note that if you copy the .chm file to another location, you should also copy the directx9_m.chi file to the same folder.

     See the Help topic, "Tips and Tricks Using Managed Code," on how to use the documentation with Visual Studio® .NET. The documentation is also available on the Web at http://www.msdn.microsoft.com/library/en-us/directx9_m/directx/directx9m.asp.


     3.2  Samples

     The samples for using DirectX for Managed Code are located at DX90SDK\Samples\C#\ and DX90SDK\Samples\VB.Net\. Samples and applications for managed code can only be properly built using Microsoft Visual Studio® .NET 2003.


     3.3  New Book
     
     A new book will be available by November 2003:
     
          Miller, Tom. Managed DirectX 9 Game and Graphics Programming Kick Start. Indianapolis, IN: Sams Publishing, 2003.
