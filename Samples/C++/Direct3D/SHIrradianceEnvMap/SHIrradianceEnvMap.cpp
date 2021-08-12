//-----------------------------------------------------------------------------
// File: SHIrradianceEnvMap.cpp
//
// Desc: 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include <dxerr9.h>
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




//-----------------------------------------------------------------------------
// Globals variables and definitions
//-----------------------------------------------------------------------------
//#define DEBUG_VS   // uncomment this line to debug the vertex shaders
//#define DEBUG_PS   // uncomment this line to debug the pixel shaders
#define MAX_LIGHTS 3
const float g_fSqrtPI = ((float)sqrtf(D3DX_PI));


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
    CModelViewerCamera      m_Camera;
    ID3DXMesh*              m_pSphere;

    LPD3DXEFFECT            m_pSimpleLightingEffect;
    LPD3DXEFFECT            m_pIrradianceEnvMapEffect;

    CD3DArcBall             m_LightArcBall[MAX_LIGHTS];
    D3DXVECTOR3             m_vDefaultLightDir[MAX_LIGHTS];
    D3DXVECTOR3             m_vLightDir[MAX_LIGHTS];
    D3DXMATRIXA16           m_mLightRot[MAX_LIGHTS];
    D3DXMATRIXA16           m_mLightRotSnapshot[MAX_LIGHTS];
    float                   m_fLightScale;
    int                     m_nNumActiveLights;
    int                     m_nActiveLight;

    D3DXMATRIXA16           m_mWorld;
    D3DXMATRIXA16           m_mView;
    D3DXMATRIXA16           m_mProj;
    int                     m_nTechnique;

    D3DMATERIAL9            m_mtrl;
    IDirect3DTexture9*      m_pTexture;
    LPD3DXMESH              m_pMesh;

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
    
    HRESULT UpdateLightDir();

    HRESULT RenderLightSphere( D3DXVECTOR3 lightDir, D3DXCOLOR sphereColor );
    HRESULT RenderMeshWithIrradianceEnvMap();
    HRESULT RenderMeshWithSimpleLighting();
    HRESULT RenderText();

public:
    CMyD3DApplication();
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

CMyD3DApplication g_d3dApp;



//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    InitCommonControls();

    if( FAILED( g_d3dApp.Create( hInst ) ) )
    {
        return 0;
    }

    return g_d3dApp.Run();
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
    m_strWindowTitle = _T("SHIrradianceEnvMap");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_pFont = NULL;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;
    m_bShowCursorWhenFullscreen = true;
    m_nTechnique = 0;

    m_vDefaultLightDir[0] = D3DXVECTOR3(0.37f,0.36f,-0.86f);
    m_vDefaultLightDir[1] = D3DXVECTOR3(-0.69f,-0.60f,-0.42f);
    m_vDefaultLightDir[2] = D3DXVECTOR3(-0.85f,0.37f,0.37f);
    for( int i=0; i<MAX_LIGHTS; i++ )
    {
        m_vLightDir[i] = m_vDefaultLightDir[i];
        D3DXMatrixIdentity( &m_mLightRot[i] );
        D3DXMatrixIdentity( &m_mLightRotSnapshot[i] );
    }
    m_nActiveLight = 0;
    m_nNumActiveLights = 3;
    m_fLightScale = 1.0f;

    m_pIrradianceEnvMapEffect = NULL;
    m_pSimpleLightingEffect = NULL;

    m_pMesh = NULL;
    m_bShowHelp = TRUE;
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

    if( FAILED( hr = LoadMesh( TEXT("tiger.x"), &m_pMesh ) ) )
        return hr;

    TCHAR str[MAX_PATH];
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("tiger.bmp") );
    if( FAILED( hr = D3DXCreateTextureFromFile( m_pd3dDevice, str, &m_pTexture ) ) )
        return DXTRACE_ERR( TEXT("D3DXCreateTextureFromFile"), hr );

    // Create a sphere to show the light's position
    hr = D3DXCreateSphere( m_pd3dDevice, 1.0f, 8, 8, &m_pSphere, NULL );
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXCreateSphere"), hr );

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

    // Read the D3DX effect file
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("SHIrradianceEnvMap.fx") );
    hr = D3DXCreateEffectFromFile( m_pd3dDevice, str, NULL, NULL, 
                                   dwShaderFlags, NULL, &m_pIrradianceEnvMapEffect, NULL );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXCreateEffectFromFile"), hr );

    // Read the D3DX effect file
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("SimpleLighting.fx") );
    hr = D3DXCreateEffectFromFile( m_pd3dDevice, str, NULL, NULL, 
                                   dwShaderFlags, NULL, &m_pSimpleLightingEffect, NULL );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXCreateEffectFromFile"), hr );


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
    DXUtil_FindMediaFileCb( str, sizeof(str), strFileName );
    hr = D3DXLoadMeshFromX(str, D3DXMESH_MANAGED, m_pd3dDevice, 
                           NULL, NULL, NULL, 
                           NULL, &pMesh);
    if( FAILED(hr) )
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

        hr = D3DXComputeNormals( pTempMesh, NULL );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("CloneMeshFVF"), hr );

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
    hr = pMesh->OptimizeInplace(D3DXMESHOPT_VERTEXCACHE|D3DXMESHOPT_IGNOREVERTS, rgdwAdjacency, NULL, NULL, NULL);
    delete []rgdwAdjacency;
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("OptimizeInplace"), hr );

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

    if( m_pFont )
    {
        if( FAILED( hr = m_pFont->OnResetDevice() ) )
            return DXTRACE_ERR( TEXT("m_pFont->OnResetDevice"), hr );
    }

    for( int i=0; i<MAX_LIGHTS; i++ )
        m_LightArcBall[i].SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );

    // Turn off lighting since its done in the shaders 
    m_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE );

    // Setup the camera
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI/4, fAspect, 1.0f, 10000.0f );
    D3DXVECTOR3 vFrom( 0.0f, 0.0f, -5.0f );
    D3DXVECTOR3 vAt( 0, 0.0f, 0 );
    m_Camera.SetViewParams( &vFrom, &vAt );
    m_Camera.SetRadius( 5.0f, 1.0f, 20.0f );
    m_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );
    m_Camera.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );
    m_Camera.SetEnablePositionMovement( false );
    m_Camera.SetResetCursorAfterMove( false );

    if( m_pSimpleLightingEffect )
        m_pSimpleLightingEffect->OnResetDevice();
    if( m_pIrradianceEnvMapEffect )
        m_pIrradianceEnvMapEffect->OnResetDevice();

    return S_OK;
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
    D3DXMATRIXA16 mWorldViewProj;

    // Get the projection & view matrix from the camera class
    m_mView  = *m_Camera.GetViewMatrix();       
    m_mProj  = *m_Camera.GetProjMatrix();       
    m_mWorld = *m_Camera.GetWorldMatrix();       

    // Clear the render target
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0x00003F3F, 1.0f, 0L );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Render the light spheres so the user can 
        // visually see the light dir
        for( int i=0; i<m_nNumActiveLights; i++ )
        {
            D3DXCOLOR sphereColor = ( i == m_nActiveLight ) ? D3DXCOLOR(1,1,0,1) : D3DXCOLOR(1,1,1,1);
            RenderLightSphere( m_vLightDir[i], sphereColor );
        }

        mWorldViewProj = m_mWorld * m_mView * m_mProj;
        m_pIrradianceEnvMapEffect->SetMatrix( "mWorldViewProjection", &mWorldViewProj);
        m_pSimpleLightingEffect->SetMatrix( "mWorldViewProjection", &mWorldViewProj);

        switch( m_nTechnique )
        {
            case 0: 
                if( FAILED( hr = RenderMeshWithIrradianceEnvMap() ) ) 
                    return hr;
                break;
            case 1: 
                if( FAILED( hr = RenderMeshWithSimpleLighting() ) )
                    return hr;
                break;
        }

        RenderText();

        // End the scene.
        if( FAILED( hr = m_pd3dDevice->EndScene() ) )
            return DXTRACE_ERR( TEXT("m_pd3dDevice->EndScene"), hr );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderLightSphere()
// Desc: Render the light spheres so the user can visually see the light dir
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderLightSphere( D3DXVECTOR3 lightDir, D3DXCOLOR sphereColor )
{
    UINT iPass, cPasses;
    D3DXMATRIX mScale;
    D3DXMATRIX mTrans;
    D3DXMATRIXA16 mWorldViewProj;
    D3DXMATRIXA16 mProj;
    HRESULT hr;

    m_pSimpleLightingEffect->SetTechnique( "SimpleLightingTex0" );

    D3DXCOLOR lightOff(0,0,0,0);
    m_pSimpleLightingEffect->SetVector( "Light1DiffuseColor", (D3DXVECTOR4*)&lightOff );
    m_pSimpleLightingEffect->SetVector( "Light2DiffuseColor", (D3DXVECTOR4*)&lightOff );
    m_pSimpleLightingEffect->SetVector( "Light3DiffuseColor", (D3DXVECTOR4*)&lightOff );
    m_pSimpleLightingEffect->SetVector( "MaterialAmbientColor", (D3DXVECTOR4*)&sphereColor );

    float fObjectRadius = 1.5f;
    D3DXVECTOR3 vL = lightDir * fObjectRadius * 1.2f;
    D3DXMatrixTranslation( &mTrans, vL.x, vL.y, vL.z );
    D3DXMatrixScaling( &mScale, fObjectRadius*0.05f, fObjectRadius*0.05f, fObjectRadius*0.05f );

    mWorldViewProj = mScale * mTrans * m_mView * m_mProj;
    m_pSimpleLightingEffect->SetMatrix( "mWorldViewProjection", &mWorldViewProj);

    if( FAILED( hr = m_pSimpleLightingEffect->Begin(&cPasses, 0) ) )
        return DXTRACE_ERR( TEXT("m_pSimpleLightingEffect->Begin"), hr );
    for (iPass = 0; iPass < cPasses; iPass++)
    {
        hr = m_pSimpleLightingEffect->Pass(iPass);
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("m_pSimpleLightingEffect->Pass"), hr );
        hr = m_pSphere->DrawSubset(0);
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("DrawSubset"), hr );
    }
    m_pSimpleLightingEffect->End();

    D3DXCOLOR mtrlAmbientColor = D3DXCOLOR(0,0,0,0);
    m_pSimpleLightingEffect->SetVector( "MaterialAmbientColor", (D3DXVECTOR4*)&mtrlAmbientColor );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderMeshWithIrradianceEnvMap()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderMeshWithIrradianceEnvMap()
{
    HRESULT hr;
    UINT iPass, cPasses, iChannel;

    D3DXMATRIX mWorldInv;
    D3DXMatrixInverse(&mWorldInv, NULL, &m_mWorld);

    // This sample uses order 3 so they are 3^2 coefficents per channel and 3 channels (R,G,B)
    // If the light had the same intensity for red, green, and blue we could optimize 
    // and just use a single channel (the red channel)
    const int dwOrder = 3;
    float fLight[MAX_LIGHTS][3][dwOrder*dwOrder];  

    D3DXCOLOR lightColor = D3DXCOLOR(1.0f,1.0f,1.0f,1.0f);
    D3DXVECTOR3 lightDirObjectSpace;

    lightColor *= m_fLightScale;

	int i;
    for( i=0; i<m_nNumActiveLights; i++ )
    {
        // Transform the world space light dir into object space
        // Note that if there's multiple objects using PRT in the scene, then
        // for each object you need to either evaulate the lights in object space
        // evaulate the lights in world and rotate the light coefficients 
        // into object space.
        D3DXVec3TransformNormal( &lightDirObjectSpace, &m_vLightDir[i], &mWorldInv );

        // Pass in the light direction, the intensity of each channel, and it returns
        // an array of source radiance coefficients for each channel.  
        // For order 3, there are 9 coefficients per channel.  
        hr = D3DXSHEvalDirectionalLight( 3, &lightDirObjectSpace, 
                                        lightColor.r, lightColor.g, lightColor.b,
                                        fLight[i][0], fLight[i][1], fLight[i][2] );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("D3DXSHEvalDirectionalLight"), hr );
    }

    // For multiple lights, just them sum up using D3DXSHAdd().
    for( i=1; i<m_nNumActiveLights; i++ )
    {
        // D3DXSHAdd will add Order^2 floats.  There are 3 color channels, 
        // so call it 3 times.
        D3DXSHAdd( fLight[0][0], dwOrder, fLight[0][0], fLight[i][0] );
        D3DXSHAdd( fLight[0][1], dwOrder, fLight[0][1], fLight[i][1] );
        D3DXSHAdd( fLight[0][2], dwOrder, fLight[0][2], fLight[i][2] );
    }

    // Lighting environment coefficients
    D3DXVECTOR4 vCoefficients[3];

    // These constants are described in the article by Peter-Pike Sloan titled 
    // "Efficient Evaluation of Irradiance Environment Maps" in the book 
    // "ShaderX 2 - Shader Programming Tips and Tricks" by Wolfgang F. Engel.
    const float fC0 = 1.0f/(2.0f*g_fSqrtPI);
    const float fC1 = (float)sqrt(3.0f)/(3.0f*g_fSqrtPI);
    const float fC2 = (float)sqrt(15.0f)/(8.0f*g_fSqrtPI);
    const float fC3 = (float)sqrt(5.0f)/(16.0f*g_fSqrtPI);
    const float fC4 = 0.5f*fC2;

    for( iChannel=0; iChannel<3; iChannel++ )
    {
        vCoefficients[iChannel].x = -fC1*fLight[0][iChannel][3];
        vCoefficients[iChannel].y = -fC1*fLight[0][iChannel][1];
        vCoefficients[iChannel].z =  fC1*fLight[0][iChannel][2];
        vCoefficients[iChannel].w =  fC0*fLight[0][iChannel][0] - fC3*fLight[0][iChannel][6];
    }

    m_pIrradianceEnvMapEffect->SetVector( "cAr", &vCoefficients[0] );
    m_pIrradianceEnvMapEffect->SetVector( "cAg", &vCoefficients[1] );
    m_pIrradianceEnvMapEffect->SetVector( "cAb", &vCoefficients[2] );

    for( iChannel=0; iChannel<3; iChannel++ )
    {
        vCoefficients[iChannel].x =      fC2*fLight[0][iChannel][4];
        vCoefficients[iChannel].y =     -fC2*fLight[0][iChannel][5];
        vCoefficients[iChannel].z = 3.0f*fC3*fLight[0][iChannel][6];
        vCoefficients[iChannel].w =     -fC2*fLight[0][iChannel][7];
    }

    m_pIrradianceEnvMapEffect->SetVector( "cBr", &vCoefficients[0] );
    m_pIrradianceEnvMapEffect->SetVector( "cBg", &vCoefficients[1] );
    m_pIrradianceEnvMapEffect->SetVector( "cBb", &vCoefficients[2] );

    vCoefficients[0].x = fC4*fLight[0][0][8];
    vCoefficients[0].y = fC4*fLight[0][1][8];
    vCoefficients[0].z = fC4*fLight[0][2][8];
    vCoefficients[0].w = 1.0f;

    m_pIrradianceEnvMapEffect->SetVector( "cC", &vCoefficients[0] );

    m_pIrradianceEnvMapEffect->SetTechnique( "IrradianceEnvironmentMap" );
    m_pIrradianceEnvMapEffect->SetTexture( "MeshTexture", m_pTexture );

    if( FAILED( hr = m_pIrradianceEnvMapEffect->Begin(&cPasses, 0) ) )
        return DXTRACE_ERR( TEXT("m_pEffect->Begin"), hr );
    for (iPass = 0; iPass < cPasses; iPass++)
    {
        if( FAILED( hr = m_pIrradianceEnvMapEffect->Pass(iPass) ) )
            return DXTRACE_ERR( TEXT("m_pEffect->Pass"), hr );

        hr = m_pMesh->DrawSubset(0);
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("m_pMesh->DrawSubset"), hr );
    }

    m_pIrradianceEnvMapEffect->End();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderMeshWithSimpleLighting()
// Desc: Render the scene with standard diffuse (N.L) lighting
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderMeshWithSimpleLighting()
{
    HRESULT hr;
    UINT iPass, cPasses;
    D3DXVECTOR3 lightDirObjectSpace;
    D3DXCOLOR lightOn(1,1,1,1);
    D3DXCOLOR lightOff(0,0,0,0);

    lightOn *= m_fLightScale;

    if( m_pMesh == NULL )
        return S_OK;

    m_pSimpleLightingEffect->SetTechnique( "SimpleLightingTex1" );
    m_pSimpleLightingEffect->SetTexture( "MeshTexture", m_pTexture );

    D3DXMATRIX mWorldInv;
    D3DXMatrixInverse(&mWorldInv, NULL, &m_mWorld);

    // Light 1
    D3DXVec3TransformNormal( &lightDirObjectSpace, &m_vLightDir[0], &mWorldInv );
    m_pSimpleLightingEffect->SetFloatArray( "Light1Dir", (float*)&lightDirObjectSpace.x, 3 );
    m_pSimpleLightingEffect->SetVector( "Light1DiffuseColor", (D3DXVECTOR4*)&lightOn );

    // Light 2
    D3DXVec3TransformNormal( &lightDirObjectSpace, &m_vLightDir[1], &mWorldInv );
    m_pSimpleLightingEffect->SetFloatArray( "Light2Dir", (float*)&lightDirObjectSpace.x, 3 );
    m_pSimpleLightingEffect->SetVector( "Light2DiffuseColor", (m_nNumActiveLights > 1) ? (D3DXVECTOR4*)&lightOn : (D3DXVECTOR4*)&lightOff );

    // Light 3
    D3DXVec3TransformNormal( &lightDirObjectSpace, &m_vLightDir[2], &mWorldInv );
    m_pSimpleLightingEffect->SetFloatArray( "Light3Dir", (float*)&lightDirObjectSpace.x, 3 );
    m_pSimpleLightingEffect->SetVector( "Light3DiffuseColor", (m_nNumActiveLights > 2) ? (D3DXVECTOR4*)&lightOn : (D3DXVECTOR4*)&lightOff );

    // Render the scene using simple N dot L lighting (up to 3 lights)
    if( FAILED( hr = m_pSimpleLightingEffect->Begin(&cPasses, 0) ) )
        return DXTRACE_ERR( TEXT("m_pSimpleLightingEffect->Begin"), hr );
    for (iPass = 0; iPass < cPasses; iPass++)
    {
        hr = m_pSimpleLightingEffect->Pass(iPass);
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("m_pSimpleLightingEffect->Pass"), hr );

        DWORD dwAttribs = 0;
        m_pMesh->GetAttributeTable( NULL, &dwAttribs );
        for( DWORD i=0; i<dwAttribs; i++ )
        {
            hr = m_pMesh->DrawSubset(i);
            if( FAILED( hr ) )
                return DXTRACE_ERR( TEXT("m_pMesh->DrawSubset"), hr );
        }
    }
    m_pSimpleLightingEffect->End();
    
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
        return S_FALSE;

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

    TCHAR sz[256];
    switch( m_nTechnique )
    {
        case 0: lstrcpy( sz, TEXT("Technique: RenderMeshWithIrradianceEnvMap") ); break;
        case 1: lstrcpy( sz, TEXT("Technique: RenderMeshWithSimpleLighting") ); break;
    }
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    _sntprintf( sz, 256, TEXT("Number of lights: %d, Light scale: %0.2f, Light dir: <%0.2f,%0.2f,%0.2f>"), m_nNumActiveLights, m_fLightScale, m_vLightDir[m_nActiveLight].x, m_vLightDir[m_nActiveLight].y, m_vLightDir[m_nActiveLight].z  );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    switch( m_nTechnique )
    {
        case 0: 
            lstrcpy( sz, TEXT("This is SHIrrEnvMap lighting.  Press 'N' to change to simple N.L") ); 
            break;
        case 1: 
            lstrcpy( sz, TEXT("This is simple N.L lighting.  Press 'N' to change to SHIrrEnvMap") ); 
            break;
    }
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
                                      _T("Rotate camera: Middle mouse button\n")
                                      _T("Rotate active light: Right mouse button\n")
                                      _T("Zoom camera: Mouse wheel scroll\n"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );

        SetRect( &rc, 250, iLineY, 0, 0 ); 
        hr = m_pFont->DrawText( NULL, _T("Change # of active lights: L\n")                                      
                                      _T("Change active light: K\n")
                                      _T("Change light scale: U,I,O\n"),                               
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );

        SetRect( &rc, 450, iLineY, 0, 0 );
        hr = m_pFont->DrawText( NULL, _T("Hide help: F1\n")
                                      _T("Change Device: F2\n")
                                      _T("View readme: F9\n")
                                      _T("Quit: ESC"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );
    }
    else
    {
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        hr = m_pFont->DrawText( NULL, _T("Press F1 for help"), 
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
    if( m_pSimpleLightingEffect )
        m_pSimpleLightingEffect->OnLostDevice();
    if( m_pIrradianceEnvMapEffect )
        m_pIrradianceEnvMapEffect->OnLostDevice();

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
    SAFE_RELEASE( m_pMesh );
    SAFE_RELEASE( m_pSphere );
    SAFE_RELEASE( m_pTexture );
    SAFE_RELEASE( m_pSimpleLightingEffect );
    SAFE_RELEASE( m_pIrradianceEnvMapEffect );

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

    // If device doesn't support 1.1 vertex shaders in HW, switch to SWVP.
    if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
    {
        if( (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
        {
            return E_FAIL;
        }
    }

    // No fallback, so need ps1.1
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(1,1) )
        return E_FAIL;

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
                {
                    m_bShowHelp = !m_bShowHelp;
                    break;
                }
            }
        }

        case WM_KEYDOWN:
            switch( wParam )
            {
                case 'N':
                    m_nTechnique++;
                    m_nTechnique %= 2;
                    break;

                case 'K':
                    if( !m_LightArcBall[m_nActiveLight].IsBeingDragged() )
                    {
                        m_nActiveLight++;
                        m_nActiveLight %= m_nNumActiveLights;
                    }
                    break;

                case 'L':
                    if( !m_LightArcBall[m_nActiveLight].IsBeingDragged() )
                    {
                        m_nNumActiveLights %= MAX_LIGHTS;
                        m_nNumActiveLights++;
                        m_nActiveLight %= m_nNumActiveLights;
                        UpdateLightDir();
                    }
                    break;

                case 'I': m_fLightScale *= 1.25f; break;
                case 'U': m_fLightScale *= 0.8f; break;
                case 'O': m_fLightScale = 1.0f; break;

            }
            break;

        case WM_RBUTTONDOWN:
        {
            int iMouseX = GET_X_LPARAM(lParam);
            int iMouseY = GET_Y_LPARAM(lParam);
            m_LightArcBall[m_nActiveLight].OnBegin( iMouseX, iMouseY );
            SetCapture(hWnd);
            return TRUE;
        }

        case WM_MOUSEMOVE:
        {
            int iMouseX = GET_X_LPARAM(lParam);
            int iMouseY = GET_Y_LPARAM(lParam);
            if( m_LightArcBall[m_nActiveLight].IsBeingDragged() )
                m_LightArcBall[m_nActiveLight].OnMove( iMouseX, iMouseY );

            if( m_Camera.IsBeingDragged() )
            {
                // Evaluate the lights in terms of SH coefficients 
                // and set the shader constants, and call this function
                // whenever the lights or object rotates and need to be reevaluated
                UpdateLightDir();
            }
            return TRUE;
        }

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        {
            if( uMsg == WM_RBUTTONUP )
            {
                m_LightArcBall[m_nActiveLight].OnEnd();
                ReleaseCapture();
            }

            UpdateLightDir();
            return TRUE;
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}



//-----------------------------------------------------------------------------
// Name: UpdateLightDir()
// Desc: Make the arcball respond correctly when the camera has be rotated.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::UpdateLightDir()
{
    D3DXMATRIX mInvView;
    D3DXMatrixInverse(&mInvView, NULL, &m_mView);
    mInvView._41 = mInvView._42 = mInvView._43 = 0;

    D3DXMATRIX mLastRotInv;
    D3DXMatrixInverse(&mLastRotInv, NULL, &m_mLightRotSnapshot[m_nActiveLight]);

    D3DXMATRIX mRot = *m_LightArcBall[m_nActiveLight].GetRotationMatrix();
    m_mLightRotSnapshot[m_nActiveLight] = mRot;

    // Accumulate the delta of the arcball's rotation in view space.
    // Note that per-frame delta rotations could be problematic over long periods of time.
    m_mLightRot[m_nActiveLight] *= m_mView * mLastRotInv * mRot * mInvView;

    // Since we're accumulating delta rotations, we need to orthonormalize 
    // the matrix to prevent eventual matrix skew
    D3DXVECTOR3* pXBasis = (D3DXVECTOR3*) &m_mLightRot[m_nActiveLight]._11;
    D3DXVECTOR3* pYBasis = (D3DXVECTOR3*) &m_mLightRot[m_nActiveLight]._21;
    D3DXVECTOR3* pZBasis = (D3DXVECTOR3*) &m_mLightRot[m_nActiveLight]._31;
    D3DXVec3Normalize( pXBasis, pXBasis );
    D3DXVec3Cross( pYBasis, pZBasis, pXBasis );
    D3DXVec3Normalize( pYBasis, pYBasis );
    D3DXVec3Cross( pZBasis, pXBasis, pYBasis );

    // Transform the default direction vector by the light's rotation matrix
    D3DXVec3TransformNormal( &m_vLightDir[m_nActiveLight], &m_vDefaultLightDir[m_nActiveLight], &m_mLightRot[m_nActiveLight] );

    return S_OK;
}







