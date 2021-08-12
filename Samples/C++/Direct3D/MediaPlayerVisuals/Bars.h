//-----------------------------------------------------------------------------
// File: Bars.h
//
// Desc: An example of how to create a Windows Media Player visualization using
//       Direct3D graphics
//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _BARS_H_
#define _BARS_H_

#include "stdafx.h"
#include <d3dx9.h>
#include <comdef.h>
#include "effects.h"
#include "dxerr9.h"
#include "dxutil.h"
#include "d3dfont.h"
#include "d3dvisualization.h"
#include "resource.h"

#define BARS_REGKEYNAME     TEXT("Software\\Microsoft\\MediaPlayerVisuals\\Bars")

#define DEFAULT_BAR_WIDTH       5
#define DEFAULT_BAR_SPACING     1


// A structure for our custom vertex type
struct BARVERTEX
{
    FLOAT x, y, z, rhw; // The transformed screen position for the vertex
    DWORD color;        // The vertex color
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_BARVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)




//-----------------------------------------------------------------------------
// Name: class CBars
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CBars 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CBars : public CD3DVisualization
{
public:
    CBars();
    
    HRESULT GetTitle(TCHAR* strTitle, DWORD dwSize);
    HRESULT GetPropSheetPage( PROPSHEETPAGE* pPage );
    HRESULT GetCapabilities(DWORD * pdwCapabilities);
    

    INT_PTR CALLBACK DialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

protected:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();
    HRESULT FrameMove();
    HRESULT Render();

    HRESULT ConfirmDevice(D3DCAPS9*,DWORD,D3DFORMAT,D3DFORMAT);
    int     GetNumOfBars() { return max( 1, m_d3dsdBackBuffer.Width / ( m_dwBarWidth + m_dwBarSpacing ) ); }

    
    DWORD m_dwBarWidth;
    DWORD m_dwBarSpacing;

    int m_nLevels[ SA_BUFFER_SIZE ];

    LPDIRECT3DVERTEXBUFFER9 m_pVBBars;
    LPDIRECT3DINDEXBUFFER9 m_pIBBars;
};

#endif // _BARS_H_