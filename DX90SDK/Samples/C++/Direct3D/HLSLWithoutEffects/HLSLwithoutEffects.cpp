//-----------------------------------------------------------------------------
// File: HLSLwithoutEffects.cpp
//
// Desc: Sample showing a simple vertex shader in D3DX High Level Shader
//       Language (HLSL) without using an ID3DXEffect interface.  Not using the 
//       ID3DXEffect interface is a more difficult method of using HLSL.  See
//       the BasicHLSL sample for a simpler method of using HLSL.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#define STRICT
#include "dxstdafx.h"
#include <math.h>
#include <dxerr9.h>
#include "D3DFile.h"
#include "D3DFont.h"
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 



//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples.
//       CMyD3DApplication adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
    // Font for drawing text
    ID3DXFont*                   m_pFont;

    // Scene
    LPDIRECT3DVERTEXBUFFER9      m_pVB;
    LPDIRECT3DINDEXBUFFER9       m_pIB;
    DWORD                        m_dwNumVertices;
    DWORD                        m_dwNumIndices;
    LPDIRECT3DVERTEXSHADER9      m_pVertexShader;
    LPD3DXCONSTANTTABLE          m_pConstantTable;
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDeclaration;
    DWORD                        m_dwSize;
    BOOL                         m_bShowHelp;

    // Navigation
    CModelViewerCamera           m_Camera;

protected:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior, 
                           D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat );
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    HRESULT RenderText();

public:
    CMyD3DApplication();
};




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    InitCommonControls();
    if( FAILED( d3dApp.Create( hInst ) ) )
        return 0;

    return d3dApp.Run();
}




//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    m_strWindowTitle     = _T("HLSLwithoutEffects");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;

    m_dwCreationWidth    = 640;
    m_dwCreationHeight   = 480;

    m_pFont              = NULL;
    m_pIB                = NULL;
    m_pVB                = NULL;
    m_dwSize             = 64;
    m_dwNumIndices       = (m_dwSize - 1) * (m_dwSize - 1) * 6;
    m_dwNumVertices      = m_dwSize * m_dwSize;
    m_pVertexShader      = NULL;
    m_pConstantTable     = NULL;
    m_pVertexDeclaration = NULL;

    m_bShowHelp          = TRUE;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Paired with FinalCleanup().
//       The window has been created and the IDirect3D9 interface has been
//       created, but the device has not been created yet.  Here you can
//       perform application-related initialization and cleanup that does
//       not depend on a device.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Paired with DeleteDeviceObjects()
//       The device has been created.  Resources that are not lost on
//       Reset() can be created here -- resources in D3DPOOL_MANAGED,
//       D3DPOOL_SCRATCH, or D3DPOOL_SYSTEMMEM.  Image surfaces created via
//       CreateImageSurface are never lost and can be created here.  Vertex
//       shaders and pixel shaders can also be created here as they are not
//       lost on Reset().
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    HRESULT hr;

    // Initialize the font
    HDC hDC = GetDC( NULL );
    int nHeight = -MulDiv( 9, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    ReleaseDC( NULL, hDC );
    if( FAILED( hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("Arial"), &m_pFont ) ) )
        return DXTRACE_ERR( TEXT("D3DXCreateFont"), hr );

    // Create vertex shader
    TCHAR        tszPath[512];
    LPD3DXBUFFER pCode;

    D3DVERTEXELEMENT9 decl[] =
    {
        { 0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        D3DDECL_END()
    };

    if( FAILED( hr = m_pd3dDevice->CreateVertexDeclaration( decl, &m_pVertexDeclaration ) ) )
    {
        return DXTRACE_ERR( TEXT("CreateVertexDeclaration"), hr );
    }

    // Find the vertex shader file
    if( FAILED( hr = DXUtil_FindMediaFileCb( tszPath, 
        sizeof(tszPath), _T("HLSLwithoutEffects.vsh") ) ) )
    {
        return DXTRACE_ERR( TEXT("DXUtil_FindMediaFileCb"), hr );
    }

    // Define DEBUG_VS and/or DEBUG_PS to debug vertex and/or pixel shaders with the shader debugger.  
    // Debugging vertex shaders requires either REF or software vertex processing, and debugging 
    // pixel shaders requires REF.  The D3DXSHADER_FORCE_*_SOFTWARE_NOOPT flag improves the debug 
    // experience in the shader debugger.  It enables source level debugging, prevents instruction 
    // reordering, prevents dead code elimination, and forces the compiler to compile against the next 
    // higher available software target, which ensures that the unoptimized shaders do not exceed 
    // the shader model limitations.  Setting these flags will cause slower rendering since the shaders 
    // will be unoptimized and forced into software.  See the DirectX documentation for more information 
    // about using the shader debugger.
    DWORD dwShaderFlags = 0;
    #ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
    #ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif

    // Assemble the vertex shader from the file
    if( FAILED( hr = D3DXCompileShaderFromFile( tszPath, NULL, NULL, "Ripple",
                                                "vs_1_1", dwShaderFlags, &pCode,
                                                NULL, &m_pConstantTable ) ) )
    {
        return DXTRACE_ERR( TEXT("D3DXCompileShaderFromFile"), hr );
    }

    // Create the vertex shader
    hr = m_pd3dDevice->CreateVertexShader( (DWORD*)pCode->GetBufferPointer(),
                                            &m_pVertexShader );
    pCode->Release();
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("CreateVertexShader"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Paired with InvalidateDeviceObjects()
//       The device exists, but may have just been Reset().  Resources in
//       D3DPOOL_DEFAULT and any other device state that persists during
//       rendering should be set here.  Render states, matrices, textures,
//       etc., that don't change during rendering can be set once here to
//       avoid redundant state setting during Render() or FrameMove().
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    HRESULT hr;

    // Restore font
    if( m_pFont )
        m_pFont->OnResetDevice();

    // Setup render states
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

    // Create and initialize index buffer
    WORD* pIndices;

    if( FAILED( hr = m_pd3dDevice->CreateIndexBuffer( m_dwNumIndices * sizeof(WORD),
                                                      0, D3DFMT_INDEX16,
                                                      D3DPOOL_DEFAULT, &m_pIB, NULL ) ) )
        return DXTRACE_ERR( TEXT("CreateIndexBuffer"), hr );

    if( FAILED( hr = m_pIB->Lock( 0, 0, (void**)&pIndices, 0 ) ) )
        return DXTRACE_ERR( TEXT("Lock"), hr );

    DWORD y;
    for( y = 1; y < m_dwSize; y++ )
    {
        for( DWORD x = 1; x < m_dwSize; x++ )
        {
            *pIndices++ = (WORD)( (y-1)*m_dwSize + (x-1) );
            *pIndices++ = (WORD)( (y-0)*m_dwSize + (x-1) );
            *pIndices++ = (WORD)( (y-1)*m_dwSize + (x-0) );

            *pIndices++ = (WORD)( (y-1)*m_dwSize + (x-0) );
            *pIndices++ = (WORD)( (y-0)*m_dwSize + (x-1) );
            *pIndices++ = (WORD)( (y-0)*m_dwSize + (x-0) );
        }
    }

    if( FAILED( hr = m_pIB->Unlock() ) )
        return DXTRACE_ERR( TEXT("Unlock"), hr );

    // Create and initialize vertex buffer
    if( FAILED( hr = m_pd3dDevice->CreateVertexBuffer( m_dwNumVertices * sizeof(D3DXVECTOR2),
                                                       0, 0,
                                                       D3DPOOL_DEFAULT, &m_pVB, NULL ) ) )
        return DXTRACE_ERR( TEXT("CreateVertexBuffer"), hr );

    D3DXVECTOR2 *pVertices;
    if( FAILED( hr = m_pVB->Lock( 0, 0, (void**)&pVertices, 0 ) ) )
        return DXTRACE_ERR( TEXT("Lock"), hr );

    for( y = 0; y < m_dwSize; y++ )
    {
        for( DWORD x = 0; x < m_dwSize; x++ )
        {
            *pVertices++ = D3DXVECTOR2( ((float)x / (float)(m_dwSize-1) - 0.5f) * D3DX_PI,
                                        ((float)y / (float)(m_dwSize-1) - 0.5f) * D3DX_PI );
        }
    }

    if( FAILED( hr = m_pVB->Unlock() ) )
        return DXTRACE_ERR( TEXT("Unlock"), hr );

    // Setup the camera
    FLOAT fAspectRatio = (FLOAT)m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI / 3, fAspectRatio, 0.1f, 100.0f );
    m_Camera.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );
    m_Camera.SetViewQuat( D3DXQUATERNION(-0.275f, 0.3f, 0.0f, 0.9f) );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Update the camera
    m_Camera.FrameMove( m_fElapsedTime );

    // Set up the vertex shader constants
    D3DXMATRIXA16 mWorldViewProj;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;

    mWorld = *m_Camera.GetWorldMatrix();
    mView  = *m_Camera.GetViewMatrix();
    mProj  = *m_Camera.GetProjMatrix();

    mWorldViewProj = mWorld * mView * mProj;

    m_pConstantTable->SetMatrix( m_pd3dDevice, "mWorldViewProj", &mWorldViewProj );
    m_pConstantTable->SetFloat( m_pd3dDevice, "fTime", m_fTime );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
    HRESULT hr;

    // Clear the backbuffer
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         0x00003F3F, 1.0f, 0L );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        m_pd3dDevice->SetVertexDeclaration( m_pVertexDeclaration );
        m_pd3dDevice->SetVertexShader( m_pVertexShader );
        m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(D3DXVECTOR2) );
        m_pd3dDevice->SetIndices( m_pIB );

        hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, m_dwNumVertices,
                                            0, m_dwNumIndices/3 );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawIndexedPrimitive"), hr );

        hr = RenderText();
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("RenderText"), hr );

        // End the scene.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderText()
// Desc: Draw the help & statistics for running sample
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderText()
{
    HRESULT hr;
    RECT rc;

    if( NULL == m_pFont )
        return S_OK;

    // Output statistics
    int iLineY = 0;
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    if( m_bShowHelp )
    {
        iLineY = m_d3dsdBackBuffer.Height-15*6;
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        hr = m_pFont->DrawText( NULL, _T("Controls:"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );

        SetRect( &rc, 20, iLineY, 0, 0 ); 
        hr = m_pFont->DrawText( NULL, _T("Rotate model: Left mouse button\n")
                                    _T("Rotate camera: Right mouse button\n")
                                    _T("Zoom camera: Mouse wheel scroll\n")
                                    _T("Hide help: F1\n"),                                      
                                    -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );

        SetRect( &rc, 250, iLineY, 0, 0 );
        hr = m_pFont->DrawText( NULL, _T("Change Device: F2\n")
                                    _T("View readme: F9\n")
                                    _T("Quit: ESC"),
                                    -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );
    }
    else
    {
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        hr = m_pFont->DrawText( NULL,
                                _T("Press F1 for help"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Invalidates device objects.  Paired with RestoreDeviceObjects()
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    if( m_pFont )
        m_pFont->OnLostDevice();

    SAFE_RELEASE( m_pIB );
    SAFE_RELEASE( m_pVB );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Paired with InitDeviceObjects()
//       Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.  
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
    SAFE_RELEASE( m_pFont );
    SAFE_RELEASE( m_pVertexShader );
    SAFE_RELEASE( m_pConstantTable );
    SAFE_RELEASE( m_pVertexDeclaration );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device initialization, this code checks the device
//       for some minimum set of capabilities.  In this sample we need
//       at least the support of vertex shader 1_0.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
                                          D3DFORMAT adapterFormat,
                                          D3DFORMAT backBufferFormat )
{
    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  
    #ifdef DEBUG_VS
        if( pCaps->DeviceType != D3DDEVTYPE_REF && 
            (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING) == 0 )
            return E_FAIL;
    #endif
    #ifdef DEBUG_PS
        if( pCaps->DeviceType != D3DDEVTYPE_REF )
            return E_FAIL;
    #endif

    if( (dwBehavior & D3DCREATE_HARDWARE_VERTEXPROCESSING ) ||
        (dwBehavior & D3DCREATE_MIXED_VERTEXPROCESSING ) )
    {
        if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,0) )
            return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    // Handle camera movement
    m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    switch( uMsg )
    {
        case WM_COMMAND:
            {
                switch( LOWORD(wParam) )
                {
                case IDM_TOGGLEHELP:
                    m_bShowHelp = !m_bShowHelp;
                    break;
                }
            }
    }

    // Pass remaining messages to default handler
    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}

