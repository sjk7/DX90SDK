//-----------------------------------------------------------------------------
// File: CompiledEffect.cpp
//
// Desc: This sample shows how an ID3DXEffect object can be compiled when the 
//       project is built and loaded directly as a binary file at run time.
//       
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include <dxerr9.h>
#include "resource.h"





//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Main class to run this application. Most functionality is inherited
//       from the CD3DApplication base class.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
private:
    ID3DXFont*              m_pFont;              // Font for drawing text
    bool                    m_bShowHelp;          // If true, it renders the UI control text
    CModelViewerCamera      m_Camera;             // A model viewing camera
    LPD3DXEFFECT            m_pEffect;            // D3DX effect interface

    LPD3DXMESH              m_pMesh;              // Mesh object
    LPDIRECT3DTEXTURE9      m_pMeshTexture;       // Mesh texture

    D3DXHANDLE              m_hTime;                        // Handle to var in the effect 
    D3DXHANDLE              m_hWorld;                       // Handle to var in the effect 
    D3DXHANDLE              m_hMeshTexture;                 // Handle to var in the effect 
    D3DXHANDLE              m_hWorldViewProjection;         // Handle to var in the effect 
    D3DXHANDLE              m_hTechniqueRenderScene;        // Handle to technique in the effect 

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

    HRESULT LoadMesh( TCHAR* strFileName, LPD3DXMESH* ppMesh );
    HRESULT RenderText();

public:
    CMyD3DApplication();
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
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
// Desc: Application constructor.   Paired with ~CMyD3DApplication()
//       Member variables should be initialized to a known state here.  
//       The application window has not yet been created and no Direct3D device 
//       has been created, so any initialization that depends on a window or 
//       Direct3D should be deferred to a later stage. 
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    m_strWindowTitle = _T("CompiledEffect");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;
    m_pFont = NULL;
    m_bShowHelp = TRUE;
    m_pEffect = NULL;

    m_hWorld = NULL;
    m_hMeshTexture = NULL;
    m_hWorldViewProjection = NULL;
    m_hTechniqueRenderScene = NULL;

    m_pMesh = NULL;
    m_pMeshTexture = NULL;
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

    // Load the mesh
    if( FAILED( hr = LoadMesh( TEXT("tiger.x"), &m_pMesh ) ) )
        return DXTRACE_ERR( TEXT("LoadMesh"), hr );

    // Read the D3DX effect file
    TCHAR str[MAX_PATH];
    hr = DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("CompiledEffect.fxo") );
    if( FAILED(hr) )
    {
        MessageBox( m_hWnd, TEXT("Could not locate \"CompiledEffect.fxo\".\n\n")
                            TEXT("This file is created as part of the project build process,\n")
                            TEXT("so the associated project must be compiled inside Visual\n")
                            TEXT("Studio before attempting to run this sample.\n\n") 
                            TEXT("If receiving this error even after compiling the project,\n")
                            TEXT("it's likely there was a problem compiling the effect file.\n")
                            TEXT("Check the build log to verify the custom build step was\n")
                            TEXT("run and to look for possible fxc compile errors."), 
                            TEXT("File Not Found"), MB_OK );
        return D3DAPPERR_MEDIANOTFOUND;
    }

    // Since we are loading a binary file here and this effect has already been compiled,
    // you can not pass compiler flags here (for example to debug the shaders). 
    // To debug the shaders, you must pass these flags to the compiler that generated the
    // binary (for example fxc.exe).  In this sample, there are 2 extra Visual Studio configurations
    // called "Debug Shaders" and "Unicode Debug Shaders" that pass the debug shader flags to fxc.exe.
    hr = D3DXCreateEffectFromFile(m_pd3dDevice, str, NULL, NULL, 0, NULL, &m_pEffect, NULL );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXCreateEffectFromFile"), hr );

    // Setup the camera's projection parameters
    float fAspectRatio = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 0.1f, 1000.0f );
    m_Camera.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );

    // Setup the camera's view parameters
    D3DXVECTOR3 vecEye(0.0f, 0.0f, -5.0f);
    D3DXVECTOR3 vecAt (0.0f, 0.0f, -0.0f);
    m_Camera.SetViewParams( &vecEye, &vecAt );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: LoadMesh()
// Desc: Load the mesh and ensure the mesh has normals, and optimize
//       the mesh for this graphics card's vertex cache so the triangle list
//       inside in the mesh will cache miss less which improves perf. 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::LoadMesh( TCHAR* strFileName, LPD3DXMESH* ppMesh )
{
    LPD3DXMESH pMesh = NULL;
    TCHAR str[MAX_PATH];
    HRESULT hr;

    if( ppMesh == NULL )
        return E_INVALIDARG;

    // Load the mesh with D3DX and get back a ID3DXMesh*.  For this
    // sample we'll ignore the X file's embedded materials since we know 
    // exactly the model we're loading.  See the mesh samples such as
    // "OptimizedMesh" for a more generic mesh loading example.
    hr = DXUtil_FindMediaFileCb( str, sizeof(str), strFileName );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DXUtil_FindMediaFileCb"), D3DAPPERR_MEDIANOTFOUND );
    hr = D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, 
                           m_pd3dDevice, NULL, NULL, NULL, NULL, &pMesh);
    if( FAILED(hr) || (pMesh == NULL) )
        return DXTRACE_ERR( TEXT("D3DXLoadMeshFromX"), hr );

    DWORD *rgdwAdjacency = NULL;

    // Make sure there are normals which are required for lighting
    if( !(pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
        LPD3DXMESH pTempMesh;
        hr = pMesh->CloneMeshFVF( pMesh->GetOptions(), 
                                  pMesh->GetFVF() | D3DFVF_NORMAL, 
                                  m_pd3dDevice, &pTempMesh );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("CloneMeshFVF"), hr );

        D3DXComputeNormals( pTempMesh, NULL );

        SAFE_RELEASE( pMesh );
        pMesh = pTempMesh;
    }

    // Optimize the mesh for this graphics card's vertex cache 
    // so when rendering the mesh's triangle list the vertices will 
    // cache hit more often so it won't have to re-execute the vertex shader 
    // on those vertices so it will improves perf.     
    rgdwAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
    if( rgdwAdjacency == NULL )
        return E_OUTOFMEMORY;
    pMesh->ConvertPointRepsToAdjacency(NULL, rgdwAdjacency);
    pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE, rgdwAdjacency, NULL, NULL, NULL);
    delete []rgdwAdjacency;

    *ppMesh = pMesh;

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
    TCHAR str[MAX_PATH];

    if( m_pFont )
        m_pFont->OnResetDevice();
    if( m_pEffect )
        m_pEffect->OnResetDevice();

    // Create the mesh texture from a file
    hr = DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("tiger.bmp") );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DXUtil_FindMediaFileCb"), D3DAPPERR_MEDIANOTFOUND );
    hr =  D3DXCreateTextureFromFileEx( m_pd3dDevice, str, D3DX_DEFAULT, D3DX_DEFAULT, 
                                       D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_DEFAULT, 
                                       D3DX_DEFAULT, D3DX_DEFAULT, 0, 
                                       NULL, NULL, &m_pMeshTexture );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXCreateTextureFromFileEx"), hr );


    // Set effect variables as needed
    D3DXCOLOR colorMtrlDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    D3DXCOLOR colorMtrlAmbient(0.35f, 0.35f, 0.35f, 0);
    m_pEffect->SetVector("g_MaterialAmbientColor", (D3DXVECTOR4*)&colorMtrlAmbient);
    m_pEffect->SetVector("g_MaterialDiffuseColor", (D3DXVECTOR4*)&colorMtrlDiffuse);

    // To read or write to D3DX effect variables we can use the string name 
    // instead of using handles, however it improves perf to use handles since then 
    // D3DX won't have to spend time doing string compares
    m_hTechniqueRenderScene         = m_pEffect->GetTechniqueByName( "RenderScene" );
    m_hTime                         = m_pEffect->GetParameterByName( NULL, "g_fTime" );
    m_hWorld                        = m_pEffect->GetParameterByName( NULL, "g_mWorld" );
    m_hWorldViewProjection          = m_pEffect->GetParameterByName( NULL, "g_mWorldViewProjection" );
    m_hMeshTexture                  = m_pEffect->GetParameterByName( NULL, "g_MeshTexture" );

    return hr;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Update the camera's postion based on user input 
    m_Camera.FrameMove( m_fElapsedTime );

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
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mWorldViewProjection;
    UINT iPass, cPasses;
   
    // Clear the render target and the zbuffer 
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00003F3F, 1.0f, 0);

    // Render the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Set world drawing technique
        m_pEffect->SetTechnique( m_hTechniqueRenderScene );

        // Get the projection & view matrix from the camera class
        mWorld = *m_Camera.GetWorldMatrix();       
        mProj = *m_Camera.GetProjMatrix();       
        mView = *m_Camera.GetViewMatrix();

        mWorldViewProjection = mWorld * mView * mProj;

        // Update the effect's variables 
        m_pEffect->SetMatrix( m_hWorldViewProjection, &mWorldViewProjection );
        m_pEffect->SetMatrix( m_hWorld, &mWorld );
        m_pEffect->SetFloat( m_hTime, m_fTime );
        m_pEffect->SetTexture( m_hMeshTexture, m_pMeshTexture);

        hr = m_pEffect->Begin(&cPasses, 0);
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("Begin"), hr );
        for (iPass = 0; iPass < cPasses; iPass++)
        {
            hr = m_pEffect->Pass(iPass);
            if( FAILED( hr ) )
                return DXTRACE_ERR( TEXT("Pass"), hr );

            hr = m_pMesh->DrawSubset(0);
            if( FAILED( hr ) )
                return DXTRACE_ERR( TEXT("DrawSubset"), hr );
        }
        m_pEffect->End();

        hr = RenderText();
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("RenderText"), hr );

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
    hr = m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    TCHAR sz[100];
    _sntprintf( sz, 100, TEXT("AppStat1=%0.1f AppStat2=%0.4f"), m_fTime, sin(m_fTime) ); sz[99] = 0;
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    // Draw help
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
        hr = m_pFont->DrawText( NULL, _T("Press F1 to help"), 
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
    if( m_pEffect )
        m_pEffect->OnLostDevice();

    SAFE_RELEASE(m_pMeshTexture);

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
    SAFE_RELEASE(m_pEffect);
    SAFE_RELEASE(m_pFont);
    SAFE_RELEASE(m_pMesh);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Paired with OneTimeSceneInit()
//       Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device initialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
                                          D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
    // Debugging vertex shaders requires either REF or software vertex processing 
    // and debugging pixel shaders requires REF.  For this sample DEBUG_SHADERS 
    // is defined in the Visual Studio build configuration 
    #ifdef DEBUG_SHADERS
        if( pCaps->DeviceType != D3DDEVTYPE_REF )
            return E_FAIL;
    #endif

    if( dwBehavior & D3DCREATE_PUREDEVICE )
        return E_FAIL;

    // No fallback, so need ps1.1
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(1,1) )
        return E_FAIL;

    // If device doesn't support 1.1 vertex shaders in HW, switch to SWVP.
    if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
    {
        if( (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
        {
            return E_FAIL;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Overrrides the main WndProc, so the sample can do custom message
//       handling (e.g. processing mouse, keyboard, or menu commands).
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
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

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}


