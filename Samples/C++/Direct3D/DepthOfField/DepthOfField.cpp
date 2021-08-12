//-----------------------------------------------------------------------------
// File: DepthOfField.cpp
//
// Desc: Example code showing how to do a depth of field effect.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




struct VERTEX 
{
    D3DXVECTOR4 pos;
    DWORD       clr;
    D3DXVECTOR2 tex1;
    D3DXVECTOR2 tex2;
    D3DXVECTOR2 tex3;
    D3DXVECTOR2 tex4;
    D3DXVECTOR2 tex5;
    D3DXVECTOR2 tex6;

    static const DWORD FVF;
};
const DWORD VERTEX::FVF = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX6;


class CMyD3DApplication : public CD3DApplication
{
private:
    CFirstPersonCamera      m_Camera;
    ID3DXFont*              m_pFont;              // Font for drawing text
    float                   m_fAspectRatio;

    VERTEX                  m_Vertex[4];

    LPDIRECT3DTEXTURE9      m_pFullScreenTexture;
    LPD3DXRENDERTOSURFACE   m_pRenderToSurface;
    LPDIRECT3DSURFACE9      m_pFullScreenTextureSurf;

    LPD3DXMESH              m_pScene1Mesh;
    LPDIRECT3DTEXTURE9      m_pScene1MeshTexture;
    LPD3DXMESH              m_pScene2Mesh;
    LPDIRECT3DTEXTURE9      m_pScene2MeshTexture;
    int                     m_nCurrentScene;
    LPD3DXEFFECT            m_pEffect;

    D3DXVECTOR4             m_vFocalPlaneDest;
    D3DXVECTOR4             m_vFocalPlaneSrc;
    D3DXVECTOR4             m_vFocalPlaneCur;
    FLOAT                   m_fChangeTime;
    BOOL                    m_bShowBlurFactor;
    BOOL                    m_bShowUnblurred;
    BOOL                    m_bShowHelp;
    DWORD                   m_dwBackgroundColor;

    D3DVIEWPORT9            m_ViewportFB;
    D3DVIEWPORT9            m_ViewportOffscreen;

    FLOAT                   m_fBlurConst;

    static LPCSTR           m_TechniqueNames[];
    static const DWORD      m_TechniqueCount;
    DWORD                   m_TechniqueIndex;

    D3DXHANDLE              m_hFocalPlane;
    D3DXHANDLE              m_hWorld;
    D3DXHANDLE              m_hWorldView;
    D3DXHANDLE              m_hWorldViewProjection;
    D3DXHANDLE              m_hMeshTexture;
    D3DXHANDLE              m_hTechWorldWithBlurFactor;
    D3DXHANDLE              m_hTechShowBlurFactor;
    D3DXHANDLE              m_hTechShowUnmodified;
    D3DXHANDLE              m_hTech[5];

protected:
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior, D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat );

    void RenderText();
    HRESULT LoadMesh( TCHAR* strFileName, LPD3DXMESH* ppMesh );
    void SetupQuad();
    HRESULT UpdateTechniqueSpecificVariables();

public:
    CMyD3DApplication();
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};



//-----------------------------------------------------------------------------
// Technique list
//-----------------------------------------------------------------------------
LPCSTR  CMyD3DApplication::m_TechniqueNames[]      = {"UsePS20ThirteenLookups",
                                                     "UsePS20SevenLookups",
                                                     "UsePS20SixTexcoords",
                                                     "UsePS11FourTexcoordsNoRings",
                                                     "UsePS11FourTexcoordsWithRings"};


const DWORD CMyD3DApplication::m_TechniqueCount = sizeof(m_TechniqueNames)/sizeof(LPCSTR);


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
    m_strWindowTitle = _T("Depth of Field");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_pFont = NULL;
    m_fAspectRatio = 0.0f;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;

    m_pFullScreenTexture = NULL;
    m_pFullScreenTextureSurf = NULL;
    m_pRenderToSurface = NULL;
    m_pEffect = NULL;

    m_vFocalPlaneDest = D3DXVECTOR4(0.0f, 0.0f, 1.0f, -2.5f);
    m_vFocalPlaneSrc = m_vFocalPlaneDest;
    m_fChangeTime = 0.0f;

    m_pScene1Mesh = NULL;
    m_pScene1MeshTexture = NULL;
    m_pScene2Mesh = NULL;
    m_pScene2MeshTexture = NULL;
    m_nCurrentScene = 1;

    m_bShowBlurFactor = FALSE;
    m_bShowUnblurred = FALSE;
    m_bShowHelp = TRUE;
    m_dwBackgroundColor = 0x00003F3F;

    m_fBlurConst = 4.0f;
    m_TechniqueIndex = 0;

    m_hFocalPlane = NULL;
    m_hWorld = NULL;
    m_hWorldView = NULL;
    m_hWorldViewProjection = NULL;
    m_hMeshTexture = NULL;
    m_hTechWorldWithBlurFactor = NULL;
    m_hTechShowBlurFactor = NULL;
    m_hTechShowUnmodified = NULL;
    ZeroMemory( m_hTech, sizeof(m_hTech) );
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

    // Load the meshs
    if( FAILED( hr = LoadMesh( TEXT("tiger.x"), &m_pScene1Mesh ) ) )
        return hr;
    if( FAILED( hr = LoadMesh( TEXT("sphere.x"), &m_pScene2Mesh ) ) )
        return hr;

    TCHAR str[MAX_PATH];

    // Load the mesh textures
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("tiger.bmp") );
    hr = D3DXCreateTextureFromFile(m_pd3dDevice, str, &m_pScene1MeshTexture);
    if (FAILED(hr))
        m_pScene1MeshTexture = NULL;

    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("earth.bmp") );
    hr = D3DXCreateTextureFromFile(m_pd3dDevice, str, &m_pScene2MeshTexture);
    if (FAILED(hr))
        m_pScene2MeshTexture = NULL;

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

    // Load the D3DX effect file into a ID3DXEffect interface.
    LPD3DXBUFFER pBufferErrors = NULL;
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("DepthOfField.fx") );
    hr = D3DXCreateEffectFromFile(m_pd3dDevice, str, NULL, NULL, dwShaderFlags, NULL, &m_pEffect, &pBufferErrors );
    if( FAILED( hr ) )
        return hr;

    return hr;
}



//-----------------------------------------------------------------------------
// Name: LoadMesh()
// Desc: Loads a mesh, ensures it has normals, and optimizes it for the 
//       vertex cache size of the current card.
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
// Name: SetupQuad()
// Desc: Sets up a quad to render the fullscreen render target to the backbuffer
//       so it can run a fullscreen pixel shader pass that blurs based
//       on the depth of the objects.  It set the texcoords based on the blur factor
//-----------------------------------------------------------------------------
void CMyD3DApplication::SetupQuad()
{
    D3DSURFACE_DESC desc;
    m_pFullScreenTextureSurf->GetDesc(&desc);

    FLOAT fWidth5 = (FLOAT)m_d3dsdBackBuffer.Width - 0.5f;
    FLOAT fHeight5 = (FLOAT)m_d3dsdBackBuffer.Height - 0.5f;

    FLOAT fHalf = m_fBlurConst;
    FLOAT fOffOne = fHalf * 0.5f;
    FLOAT fOffTwo = fOffOne * sqrtf(3.0f);

    FLOAT fTexWidth1 = (FLOAT)m_d3dsdBackBuffer.Width / (FLOAT)desc.Width;
    FLOAT fTexHeight1 = (FLOAT)m_d3dsdBackBuffer.Height / (FLOAT)desc.Height;

    FLOAT fWidthMod = 1.0f / (FLOAT)desc.Width ;
    FLOAT fHeightMod = 1.0f / (FLOAT)desc.Height;

    // Create vertex buffer.  
    // m_Vertex[0].tex1 == full texture coverage
    // m_Vertex[0].tex2 == full texture coverage, but shifted y by -fHalf*fHeightMod
    // m_Vertex[0].tex3 == full texture coverage, but shifted x by -fOffTwo*fWidthMod & y by -fOffOne*fHeightMod
    // m_Vertex[0].tex4 == full texture coverage, but shifted x by +fOffTwo*fWidthMod & y by -fOffOne*fHeightMod
    // m_Vertex[0].tex5 == full texture coverage, but shifted x by -fOffTwo*fWidthMod & y by +fOffOne*fHeightMod
    // m_Vertex[0].tex6 == full texture coverage, but shifted x by +fOffTwo*fWidthMod & y by +fOffOne*fHeightMod
    m_Vertex[0].pos = D3DXVECTOR4(fWidth5, -0.5f, 0.0f, 1.0f);
    m_Vertex[0].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[0].tex1 = D3DXVECTOR2(fTexWidth1, 0.0f);
    m_Vertex[0].tex2 = D3DXVECTOR2(fTexWidth1, 0.0f - fHalf*fHeightMod);
    m_Vertex[0].tex3 = D3DXVECTOR2(fTexWidth1 - fOffTwo*fWidthMod, 0.0f - fOffOne*fHeightMod);
    m_Vertex[0].tex4 = D3DXVECTOR2(fTexWidth1 + fOffTwo*fWidthMod, 0.0f - fOffOne*fHeightMod);
    m_Vertex[0].tex5 = D3DXVECTOR2(fTexWidth1 - fOffTwo*fWidthMod, 0.0f + fOffOne*fHeightMod);
    m_Vertex[0].tex6 = D3DXVECTOR2(fTexWidth1 + fOffTwo*fWidthMod, 0.0f + fOffOne*fHeightMod);

    m_Vertex[1].pos = D3DXVECTOR4(fWidth5, fHeight5, 0.0f, 1.0f);
    m_Vertex[1].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[1].tex1 = D3DXVECTOR2(fTexWidth1, fTexHeight1);
    m_Vertex[1].tex2 = D3DXVECTOR2(fTexWidth1, fTexHeight1 - fHalf*fHeightMod);
    m_Vertex[1].tex3 = D3DXVECTOR2(fTexWidth1 - fOffTwo*fWidthMod, fTexHeight1 - fOffOne*fHeightMod);
    m_Vertex[1].tex4 = D3DXVECTOR2(fTexWidth1 + fOffTwo*fWidthMod, fTexHeight1 - fOffOne*fHeightMod);
    m_Vertex[1].tex5 = D3DXVECTOR2(fTexWidth1 - fOffTwo*fWidthMod, fTexHeight1 + fOffOne*fHeightMod);
    m_Vertex[1].tex6 = D3DXVECTOR2(fTexWidth1 + fOffTwo*fWidthMod, fTexHeight1 + fOffOne*fHeightMod);

    m_Vertex[2].pos = D3DXVECTOR4(-0.5f, -0.5f, 0.0f, 1.0f);
    m_Vertex[2].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[2].tex1 = D3DXVECTOR2(0.0f, 0.0f);
    m_Vertex[2].tex2 = D3DXVECTOR2(0.0f, 0.0f - fHalf*fHeightMod);
    m_Vertex[2].tex3 = D3DXVECTOR2(0.0f - fOffTwo*fWidthMod, 0.0f - fOffOne*fHeightMod);
    m_Vertex[2].tex4 = D3DXVECTOR2(0.0f + fOffTwo*fWidthMod, 0.0f - fOffOne*fHeightMod);
    m_Vertex[2].tex5 = D3DXVECTOR2(0.0f - fOffTwo*fWidthMod, 0.0f + fOffOne*fHeightMod);
    m_Vertex[2].tex6 = D3DXVECTOR2(0.0f + fOffTwo*fWidthMod, 0.0f + fOffOne*fHeightMod);

    m_Vertex[3].pos = D3DXVECTOR4(-0.5f, fHeight5, 0.0f, 1.0f);
    m_Vertex[3].clr = D3DXCOLOR(0.5f, 0.5f, 0.5f, 0.66666f);
    m_Vertex[3].tex1 = D3DXVECTOR2(0.0f, fTexHeight1);
    m_Vertex[3].tex2 = D3DXVECTOR2(0.0f, fTexHeight1 - fHalf*fHeightMod);
    m_Vertex[3].tex3 = D3DXVECTOR2(0.0f - fOffTwo*fWidthMod, fTexHeight1 - fOffOne*fHeightMod);
    m_Vertex[3].tex4 = D3DXVECTOR2(0.0f + fOffTwo*fWidthMod, fTexHeight1 - fOffOne*fHeightMod);
    m_Vertex[3].tex5 = D3DXVECTOR2(0.0f + fOffTwo*fWidthMod, fTexHeight1 + fOffOne*fHeightMod);
    m_Vertex[3].tex6 = D3DXVECTOR2(0.0f - fOffTwo*fWidthMod, fTexHeight1 + fOffOne*fHeightMod);
}




//-----------------------------------------------------------------------------
// Name: UpdateTechniqueSpecificVariables()
// Desc: Certain parameters need to be specified for specific techniques
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::UpdateTechniqueSpecificVariables()
{
    LPCSTR strInputArrayName, strOutputArrayName;
    int nNumKernelEntries;
    HRESULT hr;
    D3DXHANDLE hAnnotation;

    // Create the post-process quad and set the texcoords based on the blur factor
    SetupQuad();

    // Get the handle to the current technique
    D3DXHANDLE hTech = m_pEffect->GetTechniqueByName(m_TechniqueNames[m_TechniqueIndex]);   
    if(hTech == NULL)
        return S_FALSE; // This will happen if the technique doesn't have this annotation

    // Get the value of the annotation int named "NumKernelEntries" inside the technique
    hAnnotation = m_pEffect->GetAnnotationByName(hTech, "NumKernelEntries");
    if( hAnnotation == NULL )
        return S_FALSE; // This will happen if the technique doesn't have this annotation
    hr = m_pEffect->GetInt(hAnnotation, &nNumKernelEntries);
    if( FAILED(hr) || nNumKernelEntries <= 0 )
        return hr; // This shouldn't happen unless the effect file has been modified

    // Get the value of the annotation string named "KernelInputArray" inside the technique
    hAnnotation = m_pEffect->GetAnnotationByName(hTech, "KernelInputArray");
    if( hAnnotation == NULL )
        return S_FALSE; // This will happen if the technique doesn't have this annotation
    if( FAILED( hr = m_pEffect->GetString( hAnnotation, &strInputArrayName) ) )
        return hr;

    // Get the value of the annotation string named "KernelOutputArray" inside the technique
    hAnnotation = m_pEffect->GetAnnotationByName(hTech, "KernelOutputArray");
    if( hAnnotation == NULL )
        return S_FALSE; // This will happen if the technique doesn't have this annotation
    if( FAILED( hr = m_pEffect->GetString( hAnnotation, &strOutputArrayName) ) )
        return hr;

    // Create a array to store the input array
    D3DXVECTOR2* aKernel = new D3DXVECTOR2[nNumKernelEntries];
    if (aKernel == NULL)
        return E_OUTOFMEMORY;

    // Get the input array
    m_pEffect->GetValue(strInputArrayName, aKernel, sizeof(D3DXVECTOR2) * nNumKernelEntries);

    // Get the size of the texture
    D3DSURFACE_DESC desc;
    m_pFullScreenTextureSurf->GetDesc(&desc);

    // Calculate the scale factor to convert the input array to screen space
    FLOAT fWidthMod = m_fBlurConst / (FLOAT)desc.Width ;
    FLOAT fHeightMod = m_fBlurConst / (FLOAT)desc.Height;

    // Scale the effect's kernel from pixel space to tex coord space
    // In pixel space 1 unit = one pixel and in tex coord 1 unit = width/height of texture
    for( int iEntry = 0; iEntry < nNumKernelEntries; iEntry++ )
    {
        aKernel[iEntry].x *= fWidthMod;
        aKernel[iEntry].y *= fHeightMod;
    }

    // Pass the updated array values to the effect file
    m_pEffect->SetValue(strOutputArrayName, aKernel, sizeof(D3DXVECTOR2) * nNumKernelEntries);

    SAFE_DELETE_ARRAY( aKernel );

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
    if( m_pEffect )
        m_pEffect->OnResetDevice();

    // Setup the camera with view & projection matrix
    D3DXVECTOR3 vecEye(1.3f, 1.1f, -3.3f);
    D3DXVECTOR3 vecAt (0.75f, 0.9f, -2.5f);
    m_Camera.SetViewParams( &vecEye, &vecAt );
    m_fAspectRatio = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DXToRadian(60.0f), m_fAspectRatio, 0.5f, 100.0f );
    m_Camera.SetResetCursorAfterMove( !m_bWindowed );

    m_pd3dDevice->GetViewport(&m_ViewportFB);

    // Backbuffer viewport is identical to frontbuffer, except starting at 0, 0
    m_ViewportOffscreen = m_ViewportFB;
    m_ViewportOffscreen.X = 0;
    m_ViewportOffscreen.Y = 0;

    // Create fullscreen renders target texture
    if(FAILED(hr = D3DXCreateTexture(m_pd3dDevice, m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pFullScreenTexture)) &&
       FAILED(hr = D3DXCreateTexture(m_pd3dDevice, m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pFullScreenTexture)))
    {
        return hr;
    }

    D3DSURFACE_DESC desc;
    m_pFullScreenTexture->GetSurfaceLevel(0, &m_pFullScreenTextureSurf);
    m_pFullScreenTextureSurf->GetDesc(&desc);

    // Create a ID3DXRenderToSurface to help render to a texture on cards 
    // that don't support render targets
    if(FAILED(hr = D3DXCreateRenderToSurface(m_pd3dDevice, desc.Width, desc.Height, 
                        desc.Format, TRUE, D3DFMT_D16, &m_pRenderToSurface)))   
        return hr;

    // clear the surface alpha to 0 so that it does not bleed into a "blurry" background
    //   this is possible because of the avoidance of blurring in a non-blurred texel
    if(SUCCEEDED(m_pRenderToSurface->BeginScene(m_pFullScreenTextureSurf, NULL)))
    {
        m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0x00, 1.0f, 0);
        m_pRenderToSurface->EndScene( 0 );
    }

    D3DXCOLOR colorWhite(1.0f, 1.0f, 1.0f, 1.0f);
    D3DXCOLOR colorBlack(0.0f, 0.0f, 0.0f, 1.0f);
    D3DXCOLOR colorAmbient(0.25f, 0.25f, 0.25f, 1.0f);

    // Get D3DXHANDLEs to the parameters/techniques that are set every frame so 
    // D3DX doesn't spend time doing string compares.  Doing this likely won't affect
    // the perf of this simple sample but it should be done in complex engine.
    m_hFocalPlane               = m_pEffect->GetParameterByName( NULL, "vFocalPlane" );
    m_hWorld                    = m_pEffect->GetParameterByName( NULL, "mWorld" );
    m_hWorldView                = m_pEffect->GetParameterByName( NULL, "mWorldView" );
    m_hWorldViewProjection      = m_pEffect->GetParameterByName( NULL, "mWorldViewProjection" );
    m_hMeshTexture              = m_pEffect->GetParameterByName( NULL, "MeshTexture" );
    m_hTechWorldWithBlurFactor  = m_pEffect->GetTechniqueByName("WorldWithBlurFactor");
    m_hTechShowBlurFactor       = m_pEffect->GetTechniqueByName("ShowBlurFactor");
    m_hTechShowUnmodified       = m_pEffect->GetTechniqueByName("ShowUnmodified");
    for( int i=0; i<5; i++ )
        m_hTech[i] = m_pEffect->GetTechniqueByName( m_TechniqueNames[i] );

    // Set the vars in the effect that doesn't change each frame
    m_pEffect->SetVector("MaterialAmbientColor", (D3DXVECTOR4*)&colorAmbient);
    m_pEffect->SetVector("MaterialDiffuseColor", (D3DXVECTOR4*)&colorWhite);
    m_pEffect->SetTexture("RenderTargetTexture", m_pFullScreenTexture);

    // Check if the current technique is valid for the new device/settings
    // Start from the current technique, increment until we find one we can use.
    DWORD OriginalTechnique = m_TechniqueIndex;
    do
    {
        if( m_TechniqueIndex == m_TechniqueCount )
            m_TechniqueIndex = 0;

        D3DXHANDLE hTech = m_pEffect->GetTechniqueByName( m_TechniqueNames[m_TechniqueIndex] );
        if( SUCCEEDED( m_pEffect->ValidateTechnique( hTech ) ) )
            break;

        m_TechniqueIndex++;
    } 
    while( OriginalTechnique != m_TechniqueIndex );

    if( FAILED( hr = UpdateTechniqueSpecificVariables() ) )
        return hr;

    return hr;
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

    SAFE_RELEASE(m_pFullScreenTextureSurf);
    SAFE_RELEASE(m_pFullScreenTexture);
    SAFE_RELEASE(m_pRenderToSurface);

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

    SAFE_RELEASE(m_pFullScreenTextureSurf);
    SAFE_RELEASE(m_pFullScreenTexture);
    SAFE_RELEASE(m_pRenderToSurface);

    SAFE_RELEASE(m_pScene1Mesh);
    SAFE_RELEASE(m_pScene1MeshTexture);
    SAFE_RELEASE(m_pScene2Mesh);
    SAFE_RELEASE(m_pScene2MeshTexture);

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
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Simple static scene, so just update the camera 
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
    UINT iPass, cPasses;

    // First render the world on the rendertarget m_pFullScreenTexture. 
    if( SUCCEEDED( m_pRenderToSurface->BeginScene(m_pFullScreenTextureSurf, &m_ViewportOffscreen) ) )
    {
        m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, m_dwBackgroundColor, 1.0f, 0);

        // Get the view & projection matrix from camera
        D3DXMATRIXA16 matWorld;
        D3DXMATRIXA16 matView = *m_Camera.GetViewMatrix();
        D3DXMATRIXA16 matProj = *m_Camera.GetProjMatrix();
        D3DXMATRIXA16 matViewProj = matView * matProj;

        // Update focal plane
        FLOAT fLerp = (m_fTime - m_fChangeTime) / 2.0f;
        fLerp = min(fLerp, 1.0f);
        D3DXVec4Lerp(&m_vFocalPlaneCur, &m_vFocalPlaneSrc, &m_vFocalPlaneDest, fLerp);
        m_pEffect->SetVector( m_hFocalPlane, &m_vFocalPlaneCur);

        // Set world render technique
        m_pEffect->SetTechnique( m_hTechWorldWithBlurFactor );

        // Set the mesh texture 
        LPD3DXMESH pSceneMesh;
        int nNumObjectsInScene;
        if( m_nCurrentScene == 1 )
        {
            m_pEffect->SetTexture( m_hMeshTexture, m_pScene1MeshTexture);
            pSceneMesh = m_pScene1Mesh;
            nNumObjectsInScene = 25;
        }
        else
        {
            m_pEffect->SetTexture( m_hMeshTexture, m_pScene2MeshTexture);
            pSceneMesh = m_pScene2Mesh;
            nNumObjectsInScene = 3;
        }

        static const D3DXVECTOR3 mScene2WorldPos[3] = { D3DXVECTOR3(-0.5f,-0.5f,-0.5f), 
                                                        D3DXVECTOR3( 1.0f, 1.0f, 2.0f), 
                                                        D3DXVECTOR3( 3.0f, 3.0f, 5.0f) };

        for(int iObject=0; iObject < nNumObjectsInScene; iObject++)
        {
            // setup the world matrix for the current world
            if( m_nCurrentScene == 1 )
            {
                D3DXMatrixTranslation( &matWorld, -(iObject % 5)*1.0f, 0.0f, (iObject / 5)*3.0f );
            }
            else
            {
                D3DXMATRIXA16 matRot, matPos;
                D3DXMatrixRotationY(&matRot, m_fTime * 0.66666f);
                D3DXMatrixTranslation( &matPos, mScene2WorldPos[iObject].x, mScene2WorldPos[iObject].y, mScene2WorldPos[iObject].z );
                matWorld = matRot * matPos;
            }

            // Update effect vars
            D3DXMATRIXA16 matWorldViewProj = matWorld * matViewProj;
            D3DXMATRIXA16 matWorldView = matWorld * matView;
            m_pEffect->SetMatrix( m_hWorld, &matWorld);
            m_pEffect->SetMatrix( m_hWorldView, &matWorldView);
            m_pEffect->SetMatrix( m_hWorldViewProjection, &matWorldViewProj);

            // Draw the mesh on the rendertarget
            m_pEffect->Begin(&cPasses, 0);
            for (iPass = 0; iPass < cPasses; iPass++)
            {
                m_pEffect->Pass(iPass);
                pSceneMesh->DrawSubset(0);
            }
            m_pEffect->End();
        }

        m_pRenderToSurface->EndScene( 0 );
    }

    // Clear the backbuffer 
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L );

    // Begin the scene, rendering to the backbuffer
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        m_pd3dDevice->SetViewport(&m_ViewportFB);

        // Set the post process technique
        if (m_bShowBlurFactor)
            m_pEffect->SetTechnique( m_hTechShowBlurFactor );
        else if (m_bShowUnblurred)
            m_pEffect->SetTechnique( m_hTechShowUnmodified );
        else
            m_pEffect->SetTechnique( m_hTech[m_TechniqueIndex] );

        // Render the fullscreen quad on to the backbuffer
        m_pEffect->Begin(&cPasses, 0);
        for (iPass = 0; iPass < cPasses; iPass++)
        {
            m_pEffect->Pass(iPass);
            m_pd3dDevice->SetFVF(VERTEX::FVF);
            m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, m_Vertex, sizeof(VERTEX));
        }
        m_pEffect->End();

        // Render the text
        RenderText();

        // End the scene.
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderText()
// Desc: Draw the help & statistics for running sample
//-----------------------------------------------------------------------------
void CMyD3DApplication::RenderText()
{
    RECT rc;
    TCHAR str[256];

    if( NULL == m_pFont )
        return;

    // Output statistics
    int iLineY = 0;
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );

    if (m_bShowBlurFactor)
        lstrcpy( str, TEXT("Technique: ShowBlurFactor") );
    else if (m_bShowUnblurred)
        lstrcpy( str, TEXT("Technique: ShowUnmodified") );
    else
    {
        TCHAR *tszTechName;
#ifdef UNICODE
        TCHAR tszBuf[MAX_PATH];
        tszTechName = tszBuf;
        MultiByteToWideChar( CP_ACP, 0, m_TechniqueNames[m_TechniqueIndex], -1, tszTechName, MAX_PATH );
        tszBuf[MAX_PATH - 1] = _T('\0');
#else
        tszTechName = (TCHAR *)m_TechniqueNames[m_TechniqueIndex];
#endif
        _sntprintf( str, 256, TEXT("Technique: %s"), tszTechName );
    }
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, str, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

    _sntprintf( str, 256, TEXT("Blur Factor: %0.1f  Focal Plane: (%0.1f,%0.1f,%0.1f,%0.1f)"), m_fBlurConst, m_vFocalPlaneCur.x, m_vFocalPlaneCur.y, m_vFocalPlaneCur.z, m_vFocalPlaneCur.w );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    m_pFont->DrawText( NULL, str, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

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
                                 _T("Change Technique: Plus,Minus\n"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );

        SetRect( &rc, 250, iLineY, 0, 0 ); 
        m_pFont->DrawText( NULL, _T("Show Blur Factor (Alpha): I\n")                                      
                                      _T("Show Unblurred: U\n")                                      
                                      _T("Change Blur Factor: Y,H,N\n")
                                      _T("Change Focal Plane: 1,2,3\n")
                                      _T("Change Blur Factor: Insert,Delete,R\n"),
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

    if( pCaps->PixelShaderVersion < D3DPS_VERSION(1,1) )
        return E_FAIL;

    // If device doesn't support 1.1 vertex shaders in HW, switch to SWVP.
    if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
    {
        if( (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
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
    // Pass messages to camera class
    m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    switch( uMsg )
    {
        case WM_COMMAND:
        {
            D3DXVECTOR4 vFocalPlane;
            m_pEffect->GetVector( m_hFocalPlane, &vFocalPlane);

            switch( LOWORD(wParam) )
            {
            case IDM_OPTIONS_SHOWBLURFACTOR:
                m_bShowBlurFactor = !m_bShowBlurFactor;
                if (m_bShowBlurFactor)
                    m_bShowUnblurred = FALSE;
                CheckMenuItem( GetMenu(hWnd), IDM_OPTIONS_SHOWBLURFACTOR , m_bShowBlurFactor ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem( GetMenu(hWnd), IDM_OPTIONS_SHOWUNBLURRED , m_bShowUnblurred ? MF_CHECKED : MF_UNCHECKED);
                break;

            case IDM_OPTIONS_SHOWUNBLURRED:
                m_bShowUnblurred = !m_bShowUnblurred;
                if (m_bShowUnblurred)
                    m_bShowBlurFactor = FALSE;
                CheckMenuItem( GetMenu(hWnd), IDM_OPTIONS_SHOWBLURFACTOR , m_bShowBlurFactor ? MF_CHECKED : MF_UNCHECKED);
                CheckMenuItem( GetMenu(hWnd), IDM_OPTIONS_SHOWUNBLURRED , m_bShowUnblurred ? MF_CHECKED : MF_UNCHECKED);
                break;

            case IDM_OPTIONS_NEXTTECHNIQUE:
            {
                DWORD OriginalTechnique = m_TechniqueIndex;
                do
                {
                    m_TechniqueIndex++;

                    if (m_TechniqueIndex == m_TechniqueCount)
                    {
                        m_TechniqueIndex = 0;
                    }

                    D3DXHANDLE hTech = m_pEffect->GetTechniqueByName(m_TechniqueNames[m_TechniqueIndex]);
                    if (SUCCEEDED(m_pEffect->ValidateTechnique(hTech)))
                    {
                        break;
                    }
                } while(OriginalTechnique != m_TechniqueIndex);

                UpdateTechniqueSpecificVariables();
                break;
            }

            case IDM_OPTIONS_PREVTECHNIQUE:
            {
                DWORD OriginalTechnique = m_TechniqueIndex;
                do
                {
                    if (m_TechniqueIndex == 0)
                    {
                        m_TechniqueIndex = m_TechniqueCount;
                    }

                    m_TechniqueIndex--;

                    if (SUCCEEDED(m_pEffect->ValidateTechnique(m_TechniqueNames[m_TechniqueIndex])))
                    {
                        break;
                    }
                } while(OriginalTechnique != m_TechniqueIndex);

                UpdateTechniqueSpecificVariables();
                break;
            }

            case IDM_OPTIONS_FOCALPLANE1:
                m_fChangeTime = m_fTime;
                m_vFocalPlaneDest = D3DXVECTOR4(0.0f, 0.0f, 1.0f, -2.5f);
                m_vFocalPlaneSrc = vFocalPlane;
                break;

            case IDM_OPTIONS_FOCALPLANE2:
                m_fChangeTime = m_fTime;
                m_vFocalPlaneDest = D3DXVECTOR4(0.0f, 0.0f, 1.0f, -5.0f);
                m_vFocalPlaneSrc = vFocalPlane;
                break;

            case IDM_OPTIONS_FOCALPLANE3:
                m_fChangeTime = m_fTime;
                m_vFocalPlaneDest = D3DXVECTOR4(0.0f, 0.0f, 1.0f, -8.0f);
                m_vFocalPlaneSrc = vFocalPlane;
                break;

            case IDM_OPTIONS_INCREASEBLUR:
                m_fBlurConst += 0.25f;
                UpdateTechniqueSpecificVariables();
                break;

            case IDM_OPTIONS_DECREASEBLUR:
                m_fBlurConst -= 0.25f;
                if( m_fBlurConst < 0 )  // bottoms at 0
                    m_fBlurConst = 0;
                UpdateTechniqueSpecificVariables();
                break;

            case IDM_OPTIONS_RESETBLUR:
                m_fBlurConst = 4.0f;
                UpdateTechniqueSpecificVariables();
                break;

            case IDM_OPTIONS_CHANGESCENE:
            {
                m_nCurrentScene %= 2;
                m_nCurrentScene++;

                switch( m_nCurrentScene )
                {
                    case 1:
                    {
                        D3DXVECTOR3 vecEye(0.75f, 0.8f, -2.3f);
                        D3DXVECTOR3 vecAt (0.2f, 0.75f, -1.5f);
                        m_Camera.SetViewParams( &vecEye, &vecAt );
                        break;
                    }

                    case 2:
                    {
                        D3DXVECTOR3 vecEye(0.0f, 0.0f, -3.0f);
                        D3DXVECTOR3 vecAt (0.0f, 0.0f, 0.0f);
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




