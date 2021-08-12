//-----------------------------------------------------------------------------
// File: PixelMotionBlur.cpp
//
// Desc: This sample shows how to do a single pass motion blur effect using 
//       floating point textures and multiple render targets.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




//-----------------------------------------------------------------------------
// Globals variables and definitions
//-----------------------------------------------------------------------------
#define NUM_OBJECTS 40
#define NUM_WALLS 250
#define MOVESTYLE_DEFAULT 0

struct SCREEN_VERTEX 
{
    D3DXVECTOR4 pos;
    DWORD       clr;
    D3DXVECTOR2 tex1;

    static const DWORD FVF;
};
const DWORD SCREEN_VERTEX::FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;


struct OBJECT
{
public:
    D3DXVECTOR3         m_vWorldPos;       
    D3DXMATRIXA16       m_mWorld;
    D3DXMATRIXA16       m_mWorldLast;
    LPD3DXMESH          m_pMesh;
    LPDIRECT3DTEXTURE9  m_pMeshTexture;
};



//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Main class to run this application. Most functionality is inherited
//       from the CD3DApplication base class.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
private:
    ID3DXFont*              m_pFont;              // Font for drawing text
    bool                    m_bShowHelp;
    CFirstPersonCamera      m_Camera;
    LPD3DXEFFECT            m_pEffect;

    SCREEN_VERTEX           m_Vertex[4];

    LPD3DXMESH              m_pMesh1;
    LPDIRECT3DTEXTURE9      m_pMeshTexture1;
    LPD3DXMESH              m_pMesh2;
    LPDIRECT3DTEXTURE9      m_pMeshTexture2;
    LPDIRECT3DTEXTURE9      m_pMeshTexture3;

    LPDIRECT3DSURFACE9      m_pOriginalRenderTarget;

    LPDIRECT3DTEXTURE9      m_pFullScreenRenderTarget;
    LPDIRECT3DSURFACE9      m_pFullScreenRenderTargetSurf;

    LPDIRECT3DTEXTURE9      m_pPixelVelocityTexture1;
    LPDIRECT3DSURFACE9      m_pPixelVelocitySurf1;
    LPDIRECT3DTEXTURE9      m_pPixelVelocityTexture2;
    LPDIRECT3DSURFACE9      m_pPixelVelocitySurf2;

    LPDIRECT3DTEXTURE9      m_pLastFrameVelocityTexture;
    LPDIRECT3DSURFACE9      m_pLastFrameVelocitySurf;
    LPDIRECT3DTEXTURE9      m_pCurFrameVelocityTexture;
    LPDIRECT3DSURFACE9      m_pCurFrameVelocitySurf;

    FLOAT                   m_fChangeTime;
    bool                    m_bShowBlurFactor;
    bool                    m_bShowUnblurred;
    DWORD                   m_dwBackgroundColor;

    float                   m_fPixelBlurConst;
    float                   m_fObjectSpeed;
    float                   m_fCameraSpeed;

    OBJECT*                 m_pScene1Object[NUM_OBJECTS];
    OBJECT*                 m_pScene2Object[NUM_WALLS];
    DWORD                   m_dwMoveSytle;
    int                     m_nSleepTime;
    D3DXMATRIX              m_mViewProjectionLast;
    int                     m_nCurrentScene;

    D3DXHANDLE              m_hWorld;
    D3DXHANDLE              m_hWorldLast;
    D3DXHANDLE              m_hMeshTexture;
    D3DXHANDLE              m_hWorldViewProjection;
    D3DXHANDLE              m_hWorldViewProjectionLast;
    D3DXHANDLE              m_hCurFrameVelocityTexture;
    D3DXHANDLE              m_hLastFrameVelocityTexture;
    D3DXHANDLE              m_hTechWorldWithVelocity;
    D3DXHANDLE              m_hPostProcessMotionBlur;

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
    void SetupFullscreenQuad();
    void RenderText();

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
    {
        return 0;
    }

    return d3dApp.Run();
}




//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    m_strWindowTitle = _T("Motion Blur");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;
    m_pFont = NULL;

    m_pFullScreenRenderTarget = NULL;
    m_pFullScreenRenderTargetSurf = NULL;
    m_pPixelVelocityTexture1 = NULL;
    m_pPixelVelocitySurf1 = NULL;
    m_pPixelVelocityTexture2 = NULL;
    m_pPixelVelocitySurf2 = NULL;
    m_pLastFrameVelocityTexture = NULL;
    m_pCurFrameVelocityTexture = NULL;
    m_pCurFrameVelocitySurf = NULL;
    m_pLastFrameVelocitySurf = NULL;
    m_pEffect = NULL;
    m_pOriginalRenderTarget = NULL;
    m_nSleepTime = 0;
    m_fPixelBlurConst = 1.0f;
    m_fObjectSpeed = 8.0f;
    m_fCameraSpeed = 10.0f;
    m_nCurrentScene = 1;

    m_fChangeTime = 0.0f;
    m_dwMoveSytle = MOVESTYLE_DEFAULT;
    D3DXMatrixIdentity( &m_mViewProjectionLast );

    m_hWorld = NULL;
    m_hWorldLast = NULL;
    m_hMeshTexture = NULL;
    m_hWorldViewProjection = NULL;
    m_hWorldViewProjectionLast = NULL;
    m_hCurFrameVelocityTexture = NULL;
    m_hLastFrameVelocityTexture = NULL;
    m_hTechWorldWithVelocity = NULL;
    m_hPostProcessMotionBlur = NULL;

    m_pMesh1 = NULL;
    m_pMeshTexture1 = NULL;
    m_pMesh2 = NULL;
    m_pMeshTexture2 = NULL;
    m_pMeshTexture3 = NULL;
    m_bShowBlurFactor = FALSE;
    m_bShowUnblurred = FALSE;
    m_bShowHelp = TRUE;
    m_dwBackgroundColor = 0x00003F3F;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    int iObject;
    for( iObject=0; iObject < NUM_OBJECTS; iObject++ )
    {
        m_pScene1Object[iObject] = new OBJECT;
        ZeroMemory( m_pScene1Object[iObject], sizeof(OBJECT) );
        if( m_pScene1Object[iObject] == NULL )
            return E_OUTOFMEMORY;
    }

    for( iObject=0; iObject < NUM_WALLS; iObject++ )
    {
        m_pScene2Object[iObject] = new OBJECT;
        ZeroMemory( m_pScene2Object[iObject], sizeof(OBJECT) );
        if( m_pScene2Object[iObject] == NULL )
            return E_OUTOFMEMORY;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: This creates all device-dependent managed objects, such as managed
//       textures and managed vertex buffers.
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
        return hr;

    if( FAILED( hr = LoadMesh( TEXT("sphere.x"), &m_pMesh1 ) ) )
        return hr;
    if( FAILED( hr = LoadMesh( TEXT("quad.x"), &m_pMesh2 ) ) )
        return hr;

    // Create the mesh texture from a file
    TCHAR str[MAX_PATH];
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("earth.bmp") );
    hr = D3DXCreateTextureFromFile(m_pd3dDevice, str, &m_pMeshTexture1);
    if( FAILED(hr) )
        m_pMeshTexture1 = NULL;
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("env2.bmp") );
    hr = D3DXCreateTextureFromFile(m_pd3dDevice, str, &m_pMeshTexture2);
    if( FAILED(hr) )
        m_pMeshTexture2 = NULL;
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("seafloor.bmp") );
    hr = D3DXCreateTextureFromFile(m_pd3dDevice, str, &m_pMeshTexture3);
    if( FAILED(hr) )
        m_pMeshTexture3 = NULL;

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

    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("PixelMotionBlur.fx") );
    hr = D3DXCreateEffectFromFile(m_pd3dDevice, str, NULL, NULL, dwShaderFlags, NULL, &m_pEffect, NULL );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    if( FAILED( hr ) )
        return hr;

    int iObject;
    for( iObject=0; iObject < NUM_OBJECTS; iObject++ )
    {
        m_pScene1Object[iObject]->m_pMesh = m_pMesh1;
        m_pScene1Object[iObject]->m_pMeshTexture = m_pMeshTexture1;
    }

    for( iObject=0; iObject < NUM_WALLS; iObject++ )
    {
        m_pScene2Object[iObject]->m_pMesh = m_pMesh2;

        D3DXVECTOR3 vPos;
        D3DXMATRIX mRot;
        D3DXMATRIX mPos;
        D3DXMATRIX mScale;

        if( iObject < NUM_WALLS/5*1 )
        {
            m_pScene2Object[iObject]->m_pMeshTexture = m_pMeshTexture3;

            // Center floor
            vPos.x = 0.0f;
            vPos.y = 0.0f;
            vPos.z = (iObject-NUM_WALLS/5*0) * 1.0f + 10.0f;

            D3DXMatrixRotationX( &mRot, -D3DX_PI/2.0f );
            D3DXMatrixScaling( &mScale, 1.0f, 1.0f, 1.0f );
        }
        else if( iObject < NUM_WALLS/5*2 )
        {
            m_pScene2Object[iObject]->m_pMeshTexture = m_pMeshTexture3;

            // Right floor
            vPos.x = 1.0f;
            vPos.y = 0.0f;
            vPos.z = (iObject-NUM_WALLS/5*1) * 1.0f + 10.0f;

            D3DXMatrixRotationX( &mRot, -D3DX_PI/2.0f );
            D3DXMatrixScaling( &mScale, 1.0f, 1.0f, 1.0f );
        }
        else if( iObject < NUM_WALLS/5*3 )
        {
            m_pScene2Object[iObject]->m_pMeshTexture = m_pMeshTexture3;

            // Left floor
            vPos.x = -1.0f;
            vPos.y = 0.0f;
            vPos.z = (iObject-NUM_WALLS/5*2) * 1.0f + 10.0f;

            D3DXMatrixRotationX( &mRot, -D3DX_PI/2.0f );
            D3DXMatrixScaling( &mScale, 1.0f, 1.0f, 1.0f );
        }
        else if( iObject < NUM_WALLS/5*4 )
        {
            m_pScene2Object[iObject]->m_pMeshTexture = m_pMeshTexture2;

            // Right wall 
            vPos.x = 1.5f;
            vPos.y = 0.5f;
            vPos.z = (iObject-NUM_WALLS/5*3) * 1.0f + 10.0f;

            D3DXMatrixRotationY( &mRot, -D3DX_PI/2.0f );
            D3DXMatrixScaling( &mScale, 1.0f, 1.0f, 1.0f );
        }
        else if( iObject < NUM_WALLS/5*5 )
        {
            m_pScene2Object[iObject]->m_pMeshTexture = m_pMeshTexture2;

            // Left wall 
            vPos.x = -1.5f;
            vPos.y = 0.5f;
            vPos.z = (iObject-NUM_WALLS/5*4) * 1.0f + 10.0f;

            D3DXMatrixRotationY( &mRot, D3DX_PI/2.0f );
            D3DXMatrixScaling( &mScale, 1.0f, 1.0f, 1.0f );
        }

        // Update the current world matrix for this object
        D3DXMatrixTranslation( &mPos, vPos.x, vPos.y, vPos.z );
        m_pScene2Object[iObject]->m_mWorld = mRot * mPos;
        m_pScene2Object[iObject]->m_mWorld = mScale * m_pScene2Object[iObject]->m_mWorld;

        // The walls don't move so just copy the current world matrix
        m_pScene2Object[iObject]->m_mWorldLast = m_pScene2Object[iObject]->m_mWorld;
    }

    // Setup the camera
    float fAspectRatio = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 1.0f, 1000.0f );

    D3DXVECTOR3 vecEye(40.0f, 0.0f, -15.0f);
    D3DXVECTOR3 vecAt (4.0f, 4.0f, -15.0f);
    m_Camera.SetViewParams( &vecEye, &vecAt );

    m_Camera.SetScalers( 0.01f, m_fCameraSpeed );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: LoadMesh()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::LoadMesh( TCHAR* strFileName, LPD3DXMESH* ppMesh )
{
    LPD3DXMESH pMesh = NULL;
    TCHAR str[MAX_PATH];
    HRESULT hr;

    if( ppMesh == NULL )
        return E_INVALIDARG;

    DXUtil_FindMediaFileCb( str, sizeof(str), strFileName );
    hr = D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, 
                           m_pd3dDevice, NULL, NULL, NULL, NULL, &pMesh);
    if( FAILED(hr) || (pMesh == NULL) )
        return hr;

    DWORD *rgdwAdjacency = NULL;

    // Make sure there are normals which are required for lighting
    if( !(pMesh->GetFVF() & D3DFVF_NORMAL) )
    {
        LPD3DXMESH pTempMesh;
        hr = pMesh->CloneMeshFVF( pMesh->GetOptions(), 
                                  pMesh->GetFVF() | D3DFVF_NORMAL, 
                                  m_pd3dDevice, &pTempMesh );
        if( FAILED(hr) )
            return hr;

        D3DXComputeNormals( pTempMesh, NULL );

        SAFE_RELEASE( pMesh );
        pMesh = pTempMesh;
    }

    // Optimze the mesh to make it fast for the user's graphics card
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
// Desc: Restore device-memory objects and state after a device is created or
//       resized.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    HRESULT hr;

    if( m_pFont )
        m_pFont->OnResetDevice();

    m_Camera.SetResetCursorAfterMove( !m_bWindowed );

    if( m_pEffect )
        m_pEffect->OnResetDevice();

    // Create a A8R8G8B8 render target texture.  This will be used to render 
    // the full screen and then rendered to the backbuffer using a quad.
    if(FAILED(hr = D3DXCreateTexture(m_pd3dDevice, 
                                     m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 
                                     1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
                                     D3DPOOL_DEFAULT, &m_pFullScreenRenderTarget)))
        return hr;

    // Create two D3DFMT_G16R16F render targets.  These will be used to store 
    // velocity of each pixel (one for the current frame, and one for last frame).
    if(FAILED(hr = D3DXCreateTexture(m_pd3dDevice, 
                                    m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 
                                    1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, 
                                    D3DPOOL_DEFAULT, &m_pPixelVelocityTexture1)))
        return hr;
    if(FAILED(hr = D3DXCreateTexture(m_pd3dDevice, 
                                    m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 
                                    1, D3DUSAGE_RENDERTARGET, D3DFMT_G16R16F, 
                                    D3DPOOL_DEFAULT, &m_pPixelVelocityTexture2)))
        return hr;

    // Store pointers to surfaces so we can call SetRenderTarget() later
    m_pPixelVelocityTexture1->GetSurfaceLevel(0, &m_pPixelVelocitySurf1);
    m_pPixelVelocityTexture2->GetSurfaceLevel(0, &m_pPixelVelocitySurf2);

    // Setup the current & last pointers that are swapped every frame.
    m_pCurFrameVelocityTexture = m_pPixelVelocityTexture1;
    m_pLastFrameVelocityTexture = m_pPixelVelocityTexture2;
    m_pCurFrameVelocitySurf = m_pPixelVelocitySurf1;
    m_pLastFrameVelocitySurf = m_pPixelVelocitySurf2;

    m_pFullScreenRenderTarget->GetSurfaceLevel(0, &m_pFullScreenRenderTargetSurf);

    SetupFullscreenQuad();

    D3DXCOLOR colorWhite(1.0f, 1.0f, 1.0f, 1.0f);
    D3DXCOLOR colorBlack(0.0f, 0.0f, 0.0f, 1.0f);
    D3DXCOLOR colorAmbient(0.35f, 0.35f, 0.35f, 0);
    D3DXCOLOR colorSpecular(0.5f, 0.5f, 0.5f, 1.0f);
    m_pEffect->SetVector("MaterialAmbientColor", (D3DXVECTOR4*)&colorAmbient);
    m_pEffect->SetVector("MaterialDiffuseColor", (D3DXVECTOR4*)&colorWhite);
    m_pEffect->SetTexture("RenderTargetTexture", m_pFullScreenRenderTarget);

    D3DSURFACE_DESC desc;
    m_pFullScreenRenderTargetSurf->GetDesc(&desc);
    m_pEffect->SetFloat("RenderTargetWidth", (FLOAT)desc.Width );
    m_pEffect->SetFloat("RenderTargetHeight", (FLOAT)desc.Height );

    // 12 is the number of samples in our post-process pass, so we don't want 
    // pixel velocity of more than 12 pixels or else we'll see artifacts
    float fVelocityCapInPixels = 3.0f;
    float fVelocityCapNonHomogeneous = fVelocityCapInPixels * 2 / m_d3dsdBackBuffer.Width;
    float fVelocityCapSqNonHomogeneous = fVelocityCapNonHomogeneous * fVelocityCapNonHomogeneous;

    m_pEffect->SetFloat("VelocityCapSq", fVelocityCapSqNonHomogeneous );
    m_pEffect->SetFloat("ConvertToNonHomogeneous", 1.0f / m_d3dsdBackBuffer.Width );

    m_hTechWorldWithVelocity    = m_pEffect->GetTechniqueByName("WorldWithVelocity");
    m_hPostProcessMotionBlur    = m_pEffect->GetTechniqueByName("PostProcessMotionBlur");
    m_hWorld                    = m_pEffect->GetParameterByName( NULL, "mWorld" );
    m_hWorldLast                = m_pEffect->GetParameterByName( NULL, "mWorldLast" );
    m_hWorldViewProjection      = m_pEffect->GetParameterByName( NULL, "mWorldViewProjection" );
    m_hWorldViewProjectionLast  = m_pEffect->GetParameterByName( NULL, "mWorldViewProjectionLast" );
    m_hMeshTexture              = m_pEffect->GetParameterByName( NULL, "MeshTexture" );
    m_hCurFrameVelocityTexture  = m_pEffect->GetParameterByName( NULL, "CurFrameVelocityTexture" );
    m_hLastFrameVelocityTexture = m_pEffect->GetParameterByName( NULL, "LastFrameVelocityTexture" );

    // Turn off lighting since its done in the shaders 
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE );

    // Save a pointer to the orignal render target to restore it later
    if( FAILED( hr = m_pd3dDevice->GetRenderTarget( 0, &m_pOriginalRenderTarget ) ) )
        return hr;

    // Clear each RT
    m_pd3dDevice->SetRenderTarget( 0, m_pFullScreenRenderTargetSurf );
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
    m_pd3dDevice->SetRenderTarget( 0, m_pLastFrameVelocitySurf );
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
    m_pd3dDevice->SetRenderTarget( 0, m_pCurFrameVelocitySurf );
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

    // Restore the orignal RT
    m_pd3dDevice->SetRenderTarget( 0, m_pOriginalRenderTarget );
    m_pd3dDevice->SetRenderTarget( 1, NULL );

    return hr;
}




//-----------------------------------------------------------------------------
// Name: SetupFullscreenQuad()
// Desc: Sets up a full screen quad.  First we render to a fullscreen render 
//       target texture, and then we render that texture using this quad to 
//       apply a pixel shader on every pixel of the scene.
//-----------------------------------------------------------------------------
void CMyD3DApplication::SetupFullscreenQuad()
{
    D3DSURFACE_DESC desc;

    m_pFullScreenRenderTargetSurf->GetDesc(&desc);

    // Ensure that we're directly mapping texels to pixels by offset by 0.5
    // For more info see the doc page titled "Directly Mapping Texels to Pixels"
    FLOAT fWidth5 = (FLOAT)m_d3dsdBackBuffer.Width - 0.5f;
    FLOAT fHeight5 = (FLOAT)m_d3dsdBackBuffer.Height - 0.5f;

    FLOAT fTexWidth1 = (FLOAT)m_d3dsdBackBuffer.Width / (FLOAT)desc.Width;
    FLOAT fTexHeight1 = (FLOAT)m_d3dsdBackBuffer.Height / (FLOAT)desc.Height;

    // Fill in the vertex values
    m_Vertex[0].pos = D3DXVECTOR4(fWidth5, -0.5f, 0.0f, 1.0f);
    m_Vertex[0].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[0].tex1 = D3DXVECTOR2(fTexWidth1, 0.0f);

    m_Vertex[1].pos = D3DXVECTOR4(fWidth5, fHeight5, 0.0f, 1.0f);
    m_Vertex[1].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[1].tex1 = D3DXVECTOR2(fTexWidth1, fTexHeight1);

    m_Vertex[2].pos = D3DXVECTOR4(-0.5f, -0.5f, 0.0f, 1.0f);
    m_Vertex[2].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[2].tex1 = D3DXVECTOR2(0.0f, 0.0f);

    m_Vertex[3].pos = D3DXVECTOR4(-0.5f, fHeight5, 0.0f, 1.0f);
    m_Vertex[3].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[3].tex1 = D3DXVECTOR2(0.0f, fTexHeight1);
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Swap the current frame's per-pixel velocity texture with  
    // last frame's per-pixel velocity texture
    LPDIRECT3DTEXTURE9 pTempTex = m_pCurFrameVelocityTexture;
    m_pCurFrameVelocityTexture = m_pLastFrameVelocityTexture;
    m_pLastFrameVelocityTexture = pTempTex;

    LPDIRECT3DSURFACE9 pTempSurf = m_pCurFrameVelocitySurf;
    m_pCurFrameVelocitySurf = m_pLastFrameVelocitySurf;
    m_pLastFrameVelocitySurf = pTempSurf;

    m_pEffect->SetFloat("PixelBlurConst", m_fPixelBlurConst);

    // Update the camera's postion based on user input 
    m_Camera.FrameMove( m_fElapsedTime );

    if( m_nCurrentScene == 1 )
    {
        // Move the objects around based on some simple function
        int iObject;
        for( iObject=0; iObject < NUM_OBJECTS; iObject++ )
        {
            D3DXVECTOR3 vPos;

            float fRadius = 7.0f;
            if( iObject >= 30 && iObject < 41 )
            {
                vPos.x = cosf(m_fObjectSpeed*0.125f*m_fTime + 2*D3DX_PI/10*(iObject-30)) * fRadius;
                vPos.y = 10.0f;
                vPos.z = sinf(m_fObjectSpeed*0.125f*m_fTime + 2*D3DX_PI/10*(iObject-30)) * fRadius - 25.0f;
            }
            else if( iObject >= 20 && iObject < 31 )
            {
                vPos.x = cosf(m_fObjectSpeed*0.25f*m_fTime + 2*D3DX_PI/10*(iObject-20)) * fRadius;
                vPos.y = 10.0f;
                vPos.z = sinf(m_fObjectSpeed*0.25f*m_fTime + 2*D3DX_PI/10*(iObject-20)) * fRadius - 5.0f;
            }
            else if( iObject >= 10 && iObject < 21 )
            {
                vPos.x = cosf(m_fObjectSpeed*0.5f*m_fTime + 2*D3DX_PI/10*(iObject-10)) * fRadius;
                vPos.y = 0.0f;
                vPos.z = sinf(m_fObjectSpeed*0.5f*m_fTime + 2*D3DX_PI/10*(iObject-10)) * fRadius - 25.0f;
            }
            else
            {
                vPos.x = cosf(m_fObjectSpeed*m_fTime + 2*D3DX_PI/10*iObject) * fRadius;
                vPos.y = 0.0f;
                vPos.z = sinf(m_fObjectSpeed*m_fTime + 2*D3DX_PI/10*iObject) * fRadius - 5.0f;
            }

            m_pScene1Object[iObject]->m_vWorldPos = vPos;

            // Store the last world matrix so that we can tell the effect file
            // what it was when we render this object
            m_pScene1Object[iObject]->m_mWorldLast = m_pScene1Object[iObject]->m_mWorld;

            // Update the current world matrix for this object
            D3DXMatrixTranslation( &m_pScene1Object[iObject]->m_mWorld, m_pScene1Object[iObject]->m_vWorldPos.x, m_pScene1Object[iObject]->m_vWorldPos.y, m_pScene1Object[iObject]->m_vWorldPos.z );
        }
    }

    // Sleep to simulate time spent doing complex 
    // graphics, AI, physics, networking, etc
    if( m_nSleepTime > 0 )
        Sleep(m_nSleepTime);

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
    LPDIRECT3DSURFACE9 pOriginalRenderTarget = NULL;
    D3DXMATRIXA16 mProj;
    D3DXMATRIXA16 mView;
    D3DXMATRIXA16 mViewProjection;
    D3DXMATRIXA16 mWorldViewProjection;
    D3DXMATRIXA16 mWorldViewProjectionLast;
    D3DXMATRIXA16 mWorld;
    D3DXVECTOR4 vEyePt;
    UINT iPass, cPasses;

    // Save a pointer to the current render target in the swap chain
    if( FAILED( hr = m_pd3dDevice->GetRenderTarget( 0, &pOriginalRenderTarget ) ) )
        return hr;

    m_pEffect->SetTexture( m_hCurFrameVelocityTexture, m_pCurFrameVelocityTexture);
    m_pEffect->SetTexture( m_hLastFrameVelocityTexture, m_pLastFrameVelocityTexture);

    // Clear the velocity render target to 0
    m_pd3dDevice->SetRenderTarget( 0, m_pCurFrameVelocitySurf );
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

    // Clear the full screen render target to the background color
    m_pd3dDevice->SetRenderTarget( 0, m_pFullScreenRenderTargetSurf );
    m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, m_dwBackgroundColor, 1.0f, 0);

    // Set the render target 0 to be the fullscreen render target
    m_pd3dDevice->SetRenderTarget( 0, m_pFullScreenRenderTargetSurf );

    // Set render target 1.  The pixel shader can then output to both
    // render targets in 1 pass if multiple render targets are supported
    m_pd3dDevice->SetRenderTarget( 1, m_pCurFrameVelocitySurf );

    // Turn on Z for this pass
    m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

    // For the first pass we'll draw the screen to the full screen render target
    // and to update the velocity render target with the velocity of each pixel
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Set world drawing technique
        m_pEffect->SetTechnique( m_hTechWorldWithVelocity );
    
        // Get the projection & view matrix from the camera class
        mProj = *m_Camera.GetProjMatrix();       
        mView = *m_Camera.GetViewMatrix();
    
        mViewProjection = mView * mProj;
    
        if( m_nCurrentScene == 1 )
        {
            // For each object, tell the effect about the object's current world matrix
            // and its last frame's world matrix and render the object. 
            // The vertex shader can then use both these matricies to calculate how 
            // much each vertex has moved.  The pixel shader then interpolates this 
            // vertex velocity for each pixel
            for( int iObject=0; iObject < NUM_OBJECTS; iObject++ )
            {
                mWorldViewProjection = m_pScene1Object[iObject]->m_mWorld * mViewProjection;
                mWorldViewProjectionLast = m_pScene1Object[iObject]->m_mWorldLast * m_mViewProjectionLast;
    
                // Tell the effect where the camera is now
                m_pEffect->SetMatrix( m_hWorldViewProjection, &mWorldViewProjection);
                m_pEffect->SetMatrix( m_hWorld, &m_pScene1Object[iObject]->m_mWorld );
    
                // Tell the effect where the camera was last frame
                m_pEffect->SetMatrix( m_hWorldViewProjectionLast, &mWorldViewProjectionLast);
    
                // Tell the effect the current mesh's texture
                m_pEffect->SetTexture( m_hMeshTexture, m_pScene1Object[iObject]->m_pMeshTexture);
    
                m_pEffect->Begin(&cPasses, 0);
                for (iPass = 0; iPass < cPasses; iPass++)
                {
                    m_pEffect->Pass(iPass);
                    m_pScene1Object[iObject]->m_pMesh->DrawSubset(0);
                }
                m_pEffect->End();
            }
        }
        else if( m_nCurrentScene == 2 )
        {
            for( int iObject=0; iObject < NUM_WALLS; iObject++ )
            {
                mWorldViewProjection = m_pScene2Object[iObject]->m_mWorld * mViewProjection;
                mWorldViewProjectionLast = m_pScene2Object[iObject]->m_mWorldLast * m_mViewProjectionLast;
    
                // Tell the effect where the camera is now
                m_pEffect->SetMatrix( m_hWorldViewProjection, &mWorldViewProjection);
                m_pEffect->SetMatrix( m_hWorld, &m_pScene2Object[iObject]->m_mWorld );
    
                // Tell the effect where the camera was last frame
                m_pEffect->SetMatrix( m_hWorldViewProjectionLast, &mWorldViewProjectionLast);
    
                // Tell the effect the current mesh's texture
                m_pEffect->SetTexture( m_hMeshTexture, m_pScene2Object[iObject]->m_pMeshTexture);
    
                m_pEffect->Begin(&cPasses, 0);
                for (iPass = 0; iPass < cPasses; iPass++)
                {
                    m_pEffect->Pass(iPass);
                    m_pScene2Object[iObject]->m_pMesh->DrawSubset(0);
                }
                m_pEffect->End();
            }
        }
    
        m_pd3dDevice->EndScene();
    }

    // Remember the current view projection matrix for next frame
    m_mViewProjectionLast = mViewProjection;

    // Now that we have the scene rendered into m_pFullScreenRenderTargetSurf
    // and the pixel velocity rendered into m_pCurFrameVelocitySurf 
    // we can do a final pass to render this into the backbuffer and use
    // a pixel shader to blur the pixels based on the last frame's & current frame's 
    // per pixel velocity to achieve a motion blur effect
    m_pd3dDevice->SetRenderTarget( 0, pOriginalRenderTarget );
    m_pd3dDevice->SetRenderTarget( 1, NULL );
    m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

    // Clear the render target
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L );

    // Above we rendered to a fullscreen render target texture, and now we 
    // render that texture using a quad to apply a pixel shader on every pixel of the scene.
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        m_pEffect->SetTechnique( m_hPostProcessMotionBlur );

        m_pEffect->Begin(&cPasses, 0);
        for (iPass = 0; iPass < cPasses; iPass++)
        {
            m_pEffect->Pass(iPass);
            m_pd3dDevice->SetFVF(SCREEN_VERTEX::FVF);
            m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_Vertex, sizeof(SCREEN_VERTEX));
        }
        m_pEffect->End();

        RenderText();

        m_pd3dDevice->EndScene();
    }

    SAFE_RELEASE(pOriginalRenderTarget);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderText()
// Desc: Draw the help & statistics for running sample
//-----------------------------------------------------------------------------
void CMyD3DApplication::RenderText()
{
    RECT rc;

    if( NULL == m_pFont )
        return;

    // Output statistics
    int iLineY = 0;
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );

    TCHAR sz[100];
    _stprintf( sz, _T("Blur=%0.2f Object Speed=%0.1f Camera Speed=%0.1f"), m_fPixelBlurConst, m_fObjectSpeed, m_fCameraSpeed );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

    if( m_nSleepTime == 0 )
        lstrcpy( sz, _T("Not sleeping between frames") );
    else
        _stprintf( sz, _T("Sleeping %dms per frame"), m_nSleepTime );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, _T("Press 'End' to toggle blur and 'P' to change scene."), 
                        -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

    // Draw help
    if( m_bShowHelp )
    {
        iLineY = m_d3dsdBackBuffer.Height-15*6;
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        m_pFont->DrawText( NULL, _T("Controls:"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        SetRect( &rc, 20, iLineY, 0, 0 ); 
        m_pFont->DrawText( NULL, _T("Look: Left drag mouse\n")
                                      _T("Move: A,W,S,D or Arrow Keys\n")
                                      _T("Strafe up/down: Q,E or PgUp,PgDn\n")
                                      _T("Change Scene: P\n")
                                      _T("Framerate adjust: 0,1,2,3,4,5\n"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );

        SetRect( &rc, 250, iLineY, 0, 0 ); 
        m_pFont->DrawText( NULL, _T("Change Blur Factor: Insert,Delete,R\n")                                      
                                      _T("Turn Blur On/Off: Delete\n")                                      
                                      _T("Change Object Speed: Y,U,I\n")
                                      _T("Change Camera Speed: H,J,K\n")
                                      _T("Reset camera: Home\n"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );

        SetRect( &rc, 500, iLineY, 0, 0 );
        m_pFont->DrawText( NULL, _T("Hide help: F1\n")
                                      _T("Change Device: F2\n")
                                      _T("View readme: F9\n")
                                      _T("Quit: ESC"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
    }
    else
    {
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        m_pFont->DrawText( NULL, _T("Press F1 for help"), 
                           -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    }
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the device-dependent objects are about to be lost.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    if( m_pFont )
        m_pFont->OnLostDevice();
    if( m_pEffect )
        m_pEffect->OnLostDevice();

    SAFE_RELEASE(m_pFullScreenRenderTargetSurf);
    SAFE_RELEASE(m_pFullScreenRenderTarget);
    SAFE_RELEASE(m_pPixelVelocitySurf1);
    SAFE_RELEASE(m_pPixelVelocityTexture1);
    SAFE_RELEASE(m_pPixelVelocitySurf2);
    SAFE_RELEASE(m_pPixelVelocityTexture2);
    SAFE_RELEASE(m_pOriginalRenderTarget);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
    SAFE_RELEASE( m_pFont );
    SAFE_RELEASE(m_pEffect);

    SAFE_RELEASE(m_pFullScreenRenderTargetSurf);
    SAFE_RELEASE(m_pFullScreenRenderTarget);

    SAFE_RELEASE(m_pMesh1);
    SAFE_RELEASE(m_pMeshTexture1);
    SAFE_RELEASE(m_pMesh2);
    SAFE_RELEASE(m_pMeshTexture2);
    SAFE_RELEASE(m_pMeshTexture3);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    int iObject;
    for( iObject=0; iObject < NUM_OBJECTS; iObject++ )
        SAFE_DELETE( m_pScene1Object[iObject] );

    for( iObject=0; iObject < NUM_WALLS; iObject++ )
        SAFE_DELETE( m_pScene2Object[iObject] );

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

    if( dwBehavior & D3DCREATE_PUREDEVICE )
        return E_FAIL;

    // No fallback, so need ps2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(2,0) )
        return E_FAIL;

    // No fallback, so need multiple render targets
    if( pCaps->NumSimultaneousRTs < 2 )
        return E_FAIL;

    // If device doesn't support 1.1 vertex shaders in HW, switch to SWVP.
    if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
    {
        if( (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
        {
            return E_FAIL;
        }
    }

    // No fallback, so need to support render target
    if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    adapterFormat, D3DUSAGE_RENDERTARGET, 
                    D3DRTYPE_TEXTURE, backBufferFormat ) ) )
    {
        return E_FAIL;
    }

    // No fallback, so need to support D3DFMT_G16R16F render target
    if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    adapterFormat, D3DUSAGE_RENDERTARGET, 
                    D3DRTYPE_TEXTURE, D3DFMT_G16R16F ) ) )
    {
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
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
                case IDM_OPTIONS_FPS_SETTING_0:
                    m_nSleepTime = 0;
                    break;
                case IDM_OPTIONS_FPS_SETTING_1:
                    m_nSleepTime = 10;
                    break;
                case IDM_OPTIONS_FPS_SETTING_2:
                    m_nSleepTime = 20;
                    break;
                case IDM_OPTIONS_FPS_SETTING_3:
                    m_nSleepTime = 40;
                    break;
                case IDM_OPTIONS_FPS_SETTING_4:
                    m_nSleepTime = 80;
                    break;
                case IDM_OPTIONS_FPS_SETTING_5:
                    m_nSleepTime = 160;
                    break;

                case IDM_OPTIONS_INCREASEBLUR:
                    m_fPixelBlurConst += 0.1f;
                    break;
                case IDM_OPTIONS_DECREASEBLUR:
                    m_fPixelBlurConst -= 0.1f;
                    break;
                case IDM_OPTIONS_RESETBLUR:
                    m_fPixelBlurConst = 1.0f;
                    break;
                case IDM_OPTIONS_TOGGLEBLUR:
                {
                    static float s_fRemember = 0.0f;
                    if( m_fPixelBlurConst == 0.0f )
                    {
                        m_fPixelBlurConst = s_fRemember;
                    }
                    else
                    {
                        s_fRemember = m_fPixelBlurConst;
                        m_fPixelBlurConst = 0.0f;
                    }
                    break;
                }

                case IDM_OPTIONS_INCREASEOBJSPEED:
                    m_fObjectSpeed += 0.1f;
                    break;
                case IDM_OPTIONS_DECREASEOBJSPEED:
                    m_fObjectSpeed -= 0.1f;
                    break;
                case IDM_OPTIONS_RESETOBJSPEED:
                    m_fObjectSpeed = 8.0f;
                    break;

                case IDM_OPTIONS_INCREASECAMSPEED:
                    m_fCameraSpeed += 1.0f;
                    m_Camera.SetScalers( 0.01f, m_fCameraSpeed );
                    break;
                case IDM_OPTIONS_DECREASECAMSPEED:
                    m_fCameraSpeed -= 1.0f;
                    m_Camera.SetScalers( 0.01f, m_fCameraSpeed );
                    break;
                case IDM_OPTIONS_RESETCAMSPEED:
                    m_fCameraSpeed = 25.0f;
                    m_Camera.SetScalers( 0.01f, m_fCameraSpeed );
                    break;

                case IDM_OPTIONS_CHANGESCENE:
                {
                    m_nCurrentScene %= 2;
                    m_nCurrentScene++;

                    switch( m_nCurrentScene )
                    {
                        case 1:
                        {
                            D3DXVECTOR3 vecEye(40.0f, 0.0f, -15.0f);
                            D3DXVECTOR3 vecAt (4.0f, 4.0f, -15.0f);
                            m_Camera.SetViewParams( &vecEye, &vecAt );
                            break;
                        }

                        case 2:
                        {
                            D3DXVECTOR3 vecEye(0.125f, 1.25f, 3.0f);
                            D3DXVECTOR3 vecAt (0.125f, 1.25f, 4.0f);
                            m_Camera.SetViewParams( &vecEye, &vecAt );
                            break;
                        }
                    }
                    break;
                }

                case IDM_TOGGLEHELP:
                    m_bShowHelp = !m_bShowHelp;
                    break;
            }
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}


