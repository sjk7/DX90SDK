//-----------------------------------------------------------------------------
// File: SpectrumWheel.h
//
// Desc: An example of how to create a Windows Media Player visualization using
//       Direct3D graphics
//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef _SPECTRUMWHEEL_H_
#define _SPECTRUMWHEEL_H_

#include "stdafx.h"
#include <d3dx9.h>
#include <comdef.h>
#include "effects.h"
#include "dxerr9.h"
#include "dxutil.h"
#include "d3dfont.h"
#include "d3dvisualization.h"
#include "resource.h"

#define NUM_SECTORS 16
#define NUM_TRACKS 7
#define INNER_RADIUS 0.10f
#define WHEEL_RADIUS 1.0f
#define MIN_LEVEL 50.0f
#define MAX_LEVEL 128.0f
#define VERTS_PER_CUBE 20
#define TRIS_PER_CUBE 10
#define RENDER_PASSES 2
#define RADIANS_PER_SECTOR ((2.0f * D3DX_PI) / NUM_SECTORS)
#define SPACING_PER_SECTOR (D3DX_PI / 100.0f);

#define DISTANCE_PER_TRACK ((WHEEL_RADIUS-INNER_RADIUS) / NUM_TRACKS)
#define SPACING_PER_TRACK (WHEEL_RADIUS / 50.0f)

// A structure for our custom vertex type
struct SPECTRUMVERTEX
{
    FLOAT x, y, z;      // The untransformed, 3D position for the vertex
    DWORD color;        // The vertex color
    FLOAT u, v;         // Texture coordinates
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_SPECTRUMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)


//-----------------------------------------------------------------------------
// Name: class CSpectrumWheel
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CSpectrumWheel 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CSpectrumWheel : public CD3DVisualization
{
public:
    CSpectrumWheel();
    HRESULT GetTitle(TCHAR* strTitle, DWORD dwSize);
    HRESULT GetCapabilities(DWORD * pdwCapabilities);
    

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
    

private:
    VOID    CalculateColors();
    VOID    ShiftColor( DWORD& dwColor, int& offset );
    HRESULT RotateColors();
    
    D3DXVECTOR3 m_vCameraPos;

    SPECTRUMVERTEX m_vertices[RENDER_PASSES][NUM_TRACKS][NUM_SECTORS][VERTS_PER_CUBE];
    FLOAT m_velocities[RENDER_PASSES][NUM_TRACKS][NUM_SECTORS];
    
    LPDIRECT3DVERTEXBUFFER9 m_pVB;
    LPDIRECT3DINDEXBUFFER9 m_pIB;

    D3DXCOLOR m_colorInner;
    D3DXCOLOR m_colorOuter;
    D3DXCOLOR m_colorsOn[NUM_TRACKS];
    
    LPDIRECT3DTEXTURE9 m_pCubeTex;
};

#endif // _SPECTRUMWHEEL_H_