//-----------------------------------------------------------------------------
// File: HDRLighting.cpp
//
// Desc: This sample demonstrates some high dynamic range lighting effects 
//       using floating point textures.
//
// The algorithms described in this sample are based very closely on the 
// lighting effects implemented in Masaki Kawase's Rthdribl sample and the tone 
// mapping process described in the whitepaper "Tone Reproduction for Digital 
// Images"
//
// Real-Time High Dynamic Range Image-Based Lighting (Rthdribl)
// Masaki Kawase
// http://www.daionet.gr.jp/~masa/rthdribl/ 
//
// "Photographic Tone Reproduction for Digital Images"
// Erik Reinhard, Mike Stark, Peter Shirley and Jim Ferwerda
// http://www.cs.utah.edu/~reinhard/cdrom/ 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include "GlareDefD3D.h"
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




//-----------------------------------------------------------------------------
// Defines, constants, and custom types
//-----------------------------------------------------------------------------
#define MAX_SAMPLES           16      // Maximum number of texture grabs

#define NUM_LIGHTS            2       // Number of lights in the scene

#define EMISSIVE_COEFFICIENT  39.78f  // Emissive color multiplier for each lumen
                                      // of light intensity                                    
#define NUM_TONEMAP_TEXTURES  4       // Number of stages in the 4x4 down-scaling 
                                      // of average luminance textures
#define NUM_STAR_TEXTURES     12      // Number of textures used for the star
                                      // post-processing effect
#define NUM_BLOOM_TEXTURES    3       // Number of textures used for the bloom
                                      // post-processing effect
                                    

// Texture coordinate rectangle
struct CoordRect
{
    float fLeftU, fTopV;
    float fRightU, fBottomV;
};


// World vertex format
struct WorldVertex
{
    D3DXVECTOR3 p; // position
    D3DXVECTOR3 n; // normal
    D3DXVECTOR2 t; // texture coordinate

    static const DWORD FVF;
};
const DWORD WorldVertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;


// Screen quad vertex format
struct ScreenVertex
{
    D3DXVECTOR4 p; // position
    D3DXVECTOR2 t; // texture coordinate

    static const DWORD FVF;
};
const DWORD ScreenVertex::FVF = D3DFVF_XYZRHW | D3DFVF_TEX1;






//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
public:
    CMyD3DApplication();

   
protected:
    // CD3DApplication overrides
    HRESULT ConfirmDevice(D3DCAPS9* pdtdc, DWORD dwBehavior, D3DFORMAT dtdfDisplay, D3DFORMAT dtdfBackBuffer);
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FrameMove();
    HRESULT Render();
    HRESULT FinalCleanup();
    
    
    // Scene geometry initialization routines
    HRESULT BuildWorldMesh();
    HRESULT BuildColumn( WorldVertex* &pV, float x, float y, float z, float width );
    HRESULT LoadMesh( TCHAR* strFileName, LPD3DXMESH* ppMesh );


    // Post-processing source textures creation
    HRESULT Scene_To_SceneScaled();
    HRESULT SceneScaled_To_BrightPass();
    HRESULT BrightPass_To_StarSource();
    HRESULT StarSource_To_BloomSource();
    

    // Post-processing helper functions
    HRESULT GetTextureRect( PDIRECT3DTEXTURE9 pTexture, RECT* pRect );
    HRESULT GetTextureCoords( PDIRECT3DTEXTURE9 pTexSrc, RECT* pRectSrc, PDIRECT3DTEXTURE9 pTexDest, RECT* pRectDest, CoordRect* pCoords );
    

    // Sample offset calculation. These offsets are passed to corresponding
    // pixel shaders.
    HRESULT GetSampleOffsets_GaussBlur5x5(DWORD dwD3DTexWidth, DWORD dwD3DTexHeight, D3DXVECTOR2* avTexCoordOffset, D3DXVECTOR4* avSampleWeights, FLOAT fMultiplier = 1.0f );
    HRESULT GetSampleOffsets_Bloom(DWORD dwD3DTexSize, float afTexCoordOffset[15], D3DXVECTOR4* avColorWeight, float fDeviation, FLOAT fMultiplier=1.0f);    
    HRESULT GetSampleOffsets_Star(DWORD dwD3DTexSize, float afTexCoordOffset[15], D3DXVECTOR4* avColorWeight, float fDeviation);    
    HRESULT GetSampleOffsets_DownScale4x4( DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 avSampleOffsets[] );
    HRESULT GetSampleOffsets_DownScale2x2( DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 avSampleOffsets[] );

    
    // Tone mapping and post-process lighting effects
    HRESULT MeasureLuminance();
    HRESULT CalculateAdaptation();
    HRESULT RenderStar();
    HRESULT RenderBloom();
    
    
    // Methods to control scene lights
    HRESULT AdjustLight(UINT iLight, bool bIncrement); 
    HRESULT RefreshLights();

    HRESULT RenderScene();
    HRESULT RenderText();
    HRESULT ClearTexture( LPDIRECT3DTEXTURE9 pTexture );

    VOID    ResetOptions();
    VOID    DrawFullScreenQuad(CoordRect c) { DrawFullScreenQuad( c.fLeftU, c.fTopV, c.fRightU, c.fBottomV ); }
    VOID    DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV);
    
    LRESULT MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    static inline float GaussianDistribution( float x, float y, float rho );
 
private:
    ID3DXFont*  m_pFont;                      // Font for drawing text
    CFirstPersonCamera  m_Camera;             // First-person camera tied to input

    PDIRECT3DTEXTURE9 m_pTexScene;            // HDR render target containing the scene
    PDIRECT3DTEXTURE9 m_pTexSceneScaled;      // Scaled copy of the HDR scene
    PDIRECT3DTEXTURE9 m_pTexBrightPass;       // Bright-pass filtered copy of the scene
    PDIRECT3DTEXTURE9 m_pTexAdaptedLuminanceCur;  // The luminance that the user is currenly adapted to
    PDIRECT3DTEXTURE9 m_pTexAdaptedLuminanceLast; // The luminance that the user is currenly adapted to
    PDIRECT3DTEXTURE9 m_pTexStarSource;       // Star effect source texture
    PDIRECT3DTEXTURE9 m_pTexBloomSource;      // Bloom effect source texture
    
    PDIRECT3DTEXTURE9 m_pTexWall;     // Stone texture for the room walls
    PDIRECT3DTEXTURE9 m_pTexFloor;    // Concrete texture for the room floor
    PDIRECT3DTEXTURE9 m_pTexCeiling;  // Plaster texture for the room ceiling
    PDIRECT3DTEXTURE9 m_pTexPainting; // Texture for the paintings on the wall
    

    PDIRECT3DTEXTURE9 m_apTexBloom[NUM_BLOOM_TEXTURES];     // Blooming effect working textures
    PDIRECT3DTEXTURE9 m_apTexStar[NUM_STAR_TEXTURES];       // Star effect working textures
    PDIRECT3DTEXTURE9 m_apTexToneMap[NUM_TONEMAP_TEXTURES]; // Log average luminance samples 
                                                            // from the HDR render target
    
    LPD3DXEFFECT      m_pEffect;          // Effect file
    
    LPD3DXMESH        m_pWorldMesh;       // Mesh to contain world objects
    LPD3DXMESH        m_pmeshSphere;      // Representation of point light
    
    CGlareDef         m_GlareDef;         // Glare defintion
    EGLARELIBTYPE     m_eGlareType;       // Enumerated glare type

    D3DXVECTOR4       m_avLightPosition[NUM_LIGHTS];    // Light positions in world space
    D3DXVECTOR4       m_avLightIntensity[NUM_LIGHTS];   // Light floating point intensities
    int               m_nLightLogIntensity[NUM_LIGHTS]; // Light intensities on a log scale
    int               m_nLightMantissa[NUM_LIGHTS];     // Mantissa of the light intensity

    DWORD m_dwCropWidth;    // Width of the cropped scene texture
    DWORD m_dwCropHeight;   // Height of the cropped scene texture
   
    float       m_fKeyValue;              // Middle gray key value for tone mapping
    
    bool        m_bToneMap;               // True when scene is to be tone mapped            
    bool        m_bDetailedStats;         // True when state variables should be rendered
    bool        m_bDrawHelp;              // True when help instructions are to be drawn
    bool        m_bBlueShift;             // True when blue shift is to be factored in
    bool        m_bAdaptationInvalid;     // True when adaptation level needs refreshing
};




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
    CMyD3DApplication d3dApp;

    InitCommonControls();

    if(FAILED(d3dApp.Create(hInst)))
        return 0;

    return d3dApp.Run();
}




//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CMyD3DApplication::CMyD3DApplication()
{
    m_strWindowTitle = TEXT("HDRLighting");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;
   

    m_dwCropWidth           = 0;
    m_dwCropHeight          = 0;

    m_pFont                 = NULL;
    m_pWorldMesh            = NULL;
    m_pmeshSphere           = NULL;
    m_pEffect               = NULL;
    
    m_pTexScene             = NULL;  
    m_pTexSceneScaled       = NULL;
    m_pTexWall              = NULL;  
    m_pTexFloor             = NULL;    
    m_pTexCeiling           = NULL;  
    m_pTexPainting          = NULL; 
    m_pTexAdaptedLuminanceCur  = NULL;
    m_pTexAdaptedLuminanceLast = NULL;

    m_pTexBrightPass        = NULL;
    m_pTexBloomSource       = NULL;
    m_pTexStarSource        = NULL;
    
    m_bAdaptationInvalid    = FALSE;
    
    ZeroMemory( m_apTexStar, sizeof(m_apTexStar) );
    ZeroMemory( m_apTexToneMap, sizeof(m_apTexToneMap) );
    ZeroMemory( m_apTexBloom, sizeof(m_apTexBloom) );
                                     
    ResetOptions();
}




//-----------------------------------------------------------------------------
// Name: ResetOptions()
// Desc: Reset all user-controlled options to default values
//-----------------------------------------------------------------------------
void CMyD3DApplication::ResetOptions()
{
    m_bDrawHelp         = TRUE;
    m_bDetailedStats    = TRUE;
    m_bToneMap          = TRUE;
    m_bBlueShift        = TRUE;
    m_fKeyValue         = 0.18f;
    
    m_nLightMantissa[0] = 8;
    m_nLightLogIntensity[0] = 0;

    m_nLightMantissa[1] = 8;
    m_nLightLogIntensity[1] = -1;

    m_eGlareType = GLT_DEFAULT;
    m_GlareDef.Initialize( m_eGlareType );

    RefreshLights();
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called only once to initialize application objects which persist
//       through device changes.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    // Initialize the camera
    D3DXVECTOR3   vFromPt(7.5f, 1.8f, 2);
    D3DXVECTOR3   vLookatPt(7.5f, 1.5f, 10.0f);
    m_Camera.SetViewParams( &vFromPt, &vLookatPt );

    // Set options for the first-person camera
    m_Camera.SetScalers( 0.01f, 15.0f );
    m_Camera.SetDrag( true );
    m_Camera.SetEnableYAxisMovement(false);

    D3DXVECTOR3 vMin = D3DXVECTOR3(1.5f,0.0f,1.5f);
    D3DXVECTOR3 vMax = D3DXVECTOR3(13.5f,10.0f,18.5f);
    m_Camera.SetClipToBoundary( TRUE, &vMin, &vMax ); 

    // Set light positions in world space
    m_avLightPosition[0] = D3DXVECTOR4(  4.0f,  2.0f,  18.0f, 1.0f );
    m_avLightPosition[1] = D3DXVECTOR4(  11.0f, 2.0f,  18.0f, 1.0f );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: AdjustLight
// Desc: Increment or decrement the light at the given index
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::AdjustLight(UINT iLight, bool bIncrement)
{
    if( iLight >= NUM_LIGHTS )
        return E_INVALIDARG;

    if( bIncrement && m_nLightLogIntensity[iLight] < 7 )
    {
        m_nLightMantissa[iLight]++;
        if( m_nLightMantissa[iLight] > 9 )
        {
            m_nLightMantissa[iLight] = 1;
            m_nLightLogIntensity[iLight]++;
        }
    }
    
    if( !bIncrement && m_nLightLogIntensity[iLight] > -4 )
    {
        m_nLightMantissa[iLight]--;
        if( m_nLightMantissa[iLight] < 1 )
        {
            m_nLightMantissa[iLight] = 9;
            m_nLightLogIntensity[iLight]--;
        }
    }

    RefreshLights();
    return S_OK;
}
 


   
//-----------------------------------------------------------------------------
// Name: RefreshLights
// Desc: Set the light intensities to match the current log luminance
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RefreshLights()
{
    for( int i=0; i < NUM_LIGHTS; i++ )
    {
        m_avLightIntensity[i].x = m_nLightMantissa[i] * (float) pow(10, m_nLightLogIntensity[i]);
        m_avLightIntensity[i].y = m_nLightMantissa[i] * (float) pow(10, m_nLightLogIntensity[i]);
        m_avLightIntensity[i].z = m_nLightMantissa[i] * (float) pow(10, m_nLightLogIntensity[i]);
        m_avLightIntensity[i].w = 1.0f;
    }
    
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    HRESULT hr;

    // Initialize the font
    HDC hDC = GetDC( NULL );
    int nHeight = -MulDiv( 9, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    ReleaseDC( NULL, hDC );
    
    hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("Arial"), &m_pFont );
    if( FAILED(hr) )
        return hr;

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

    // Load the effect file
    TCHAR Path[MAX_PATH];
    DXUtil_FindMediaFileCch(Path, sizeof(Path), TEXT("HDRLighting.fx"));
    hr = D3DXCreateEffectFromFile(m_pd3dDevice, Path, NULL, NULL, dwShaderFlags, NULL, &m_pEffect, NULL);
    if(FAILED(hr))
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Restores scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    HRESULT hr;
    int i=0; // loop variable

 
    // Crop the scene texture so width and height are evenly divisible by 8.
    // This cropped version of the scene will be used for post processing effects,
    // and keeping everything evenly divisible allows precise control over
    // sampling points within the shaders.
    m_dwCropWidth = m_d3dsdBackBuffer.Width - m_d3dsdBackBuffer.Width % 8;
    m_dwCropHeight = m_d3dsdBackBuffer.Height - m_d3dsdBackBuffer.Height % 8;

    // Restore the font
    if( m_pFont )
        m_pFont->OnResetDevice();
    
    // Restore the effect
    if( m_pEffect )
        m_pEffect->OnResetDevice();
    

    // Create the HDR scene texture
    hr = m_pd3dDevice->CreateTexture(m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 
                                     1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, 
                                     D3DPOOL_DEFAULT, &m_pTexScene, NULL);
    if( FAILED(hr) )
        return hr;


    // Scaled version of the HDR scene texture
    hr = m_pd3dDevice->CreateTexture(m_dwCropWidth / 4, m_dwCropHeight / 4, 
                                     1, D3DUSAGE_RENDERTARGET, 
                                     D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, 
                                     &m_pTexSceneScaled, NULL);
    if( FAILED(hr) )
        return hr;
    

    // Create the bright-pass filter texture. 
    // Texture has a black border of single texel thickness to fake border 
    // addressing using clamp addressing
    hr = m_pd3dDevice->CreateTexture(m_dwCropWidth / 4 + 2, m_dwCropHeight / 4 + 2, 
                                     1, D3DUSAGE_RENDERTARGET, 
                                     D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, 
                                     &m_pTexBrightPass, NULL);
    if( FAILED(hr) )
        return hr;

    
    
    // Create a texture to be used as the source for the star effect
    // Texture has a black border of single texel thickness to fake border 
    // addressing using clamp addressing
    hr = m_pd3dDevice->CreateTexture(m_dwCropWidth / 4 + 2, m_dwCropHeight / 4 + 2, 
                                     1, D3DUSAGE_RENDERTARGET, 
                                     D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, 
                                     &m_pTexStarSource, NULL);
    if( FAILED(hr) )
        return hr;

    
    
    // Create a texture to be used as the source for the bloom effect
    // Texture has a black border of single texel thickness to fake border 
    // addressing using clamp addressing
    hr = m_pd3dDevice->CreateTexture(m_dwCropWidth / 8 + 2, m_dwCropHeight / 8 + 2, 
                                     1, D3DUSAGE_RENDERTARGET, 
                                     D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, 
                                     &m_pTexBloomSource, NULL);
    if( FAILED(hr) )
        return hr;


    

    // Create a 2 textures to hold the luminance that the user is currently adapted
    // to. This allows for a simple simulation of light adaptation.
    hr = m_pd3dDevice->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET,
                                     D3DFMT_R16F, D3DPOOL_DEFAULT,
                                     &m_pTexAdaptedLuminanceCur, NULL);
    if( FAILED(hr) )
        return hr;
    hr = m_pd3dDevice->CreateTexture(1, 1, 1, D3DUSAGE_RENDERTARGET,
                                     D3DFMT_R16F, D3DPOOL_DEFAULT,
                                     &m_pTexAdaptedLuminanceLast, NULL);
    if( FAILED(hr) )
        return hr;


    // For each scale stage, create a texture to hold the intermediate results
    // of the luminance calculation
    for(i=0; i < NUM_TONEMAP_TEXTURES; i++)
    {
        int iSampleLen = 1 << (2*i);

        hr = m_pd3dDevice->CreateTexture(iSampleLen, iSampleLen, 1, D3DUSAGE_RENDERTARGET, 
                                         D3DFMT_R16F, D3DPOOL_DEFAULT, 
                                         &m_apTexToneMap[i], NULL);
        if( FAILED(hr) )
            return hr;
    }


    // Create the temporary blooming effect textures
    // Texture has a black border of single texel thickness to fake border 
    // addressing using clamp addressing
    for( i=1; i < NUM_BLOOM_TEXTURES; i++ )
    {
        hr = m_pd3dDevice->CreateTexture(m_dwCropWidth/8 + 2, m_dwCropHeight/8 + 2, 
                                        1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
                                        D3DPOOL_DEFAULT, &m_apTexBloom[i], NULL);
        if( FAILED(hr) )
            return hr;
    }

    // Create the final blooming effect texture
    hr = m_pd3dDevice->CreateTexture( m_dwCropWidth/8, m_dwCropHeight/8,
                                      1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, 
                                      D3DPOOL_DEFAULT, &m_apTexBloom[0], NULL);
    if( FAILED(hr) )
        return hr;
                              

    // Create the star effect textures
    for( i=0; i < NUM_STAR_TEXTURES; i++ )
    {
        hr = m_pd3dDevice->CreateTexture( m_dwCropWidth /4, m_dwCropHeight / 4,
                                          1, D3DUSAGE_RENDERTARGET,
                                          D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT,
                                          &m_apTexStar[i], NULL );

        if( FAILED(hr) )
            return hr;
    }

    // Create a texture to paint the walls
    TCHAR Path[MAX_PATH];
    DXUtil_FindMediaFileCb(Path, sizeof(Path), TEXT("env2.bmp"));

    hr = D3DXCreateTextureFromFile( m_pd3dDevice, Path, &m_pTexWall );
    if( FAILED(hr) )
        return hr;


    // Create a texture to paint the floor
    DXUtil_FindMediaFileCb(Path, sizeof(Path), TEXT("ground2.bmp"));

    hr = D3DXCreateTextureFromFile( m_pd3dDevice, Path, &m_pTexFloor );
    if( FAILED(hr) )
        return hr;


    // Create a texture to paint the ceiling
    DXUtil_FindMediaFileCb(Path, sizeof(Path), TEXT("seafloor.bmp"));

    hr = D3DXCreateTextureFromFile( m_pd3dDevice, Path, &m_pTexCeiling );
    if( FAILED(hr) )
        return hr;


    // Create a texture for the paintings
    DXUtil_FindMediaFileCb(Path, sizeof(Path), TEXT("env3.bmp"));

    hr = D3DXCreateTextureFromFile( m_pd3dDevice, Path, &m_pTexPainting );
    if( FAILED(hr) )
        return hr;


    // Textures with borders must be cleared since scissor rect testing will
    // be used to avoid rendering on top of the border
    ClearTexture( m_pTexAdaptedLuminanceCur );
    ClearTexture( m_pTexAdaptedLuminanceLast );
    ClearTexture( m_pTexBloomSource );
    ClearTexture( m_pTexBrightPass );
    ClearTexture( m_pTexStarSource );

    for( i=0; i < NUM_BLOOM_TEXTURES; i++ )
    {
        ClearTexture( m_apTexBloom[i] );
    }
   
    
    // Build the world object
    hr = BuildWorldMesh();
    if( FAILED(hr) )
        return hr;


    // Create sphere mesh to represent the light
    hr = LoadMesh(TEXT("sphere0.x"), &m_pmeshSphere);
    if( FAILED(hr) )
        return hr;

    // Set camera parameters
    FLOAT fAspect = ((FLOAT)m_dwCropWidth) / m_dwCropHeight;
    m_Camera.SetProjParams( D3DX_PI/4, fAspect, 0.2f, 30.0f );
    m_Camera.SetResetCursorAfterMove( true );

    // Set effect file variables
    D3DXMATRIX mProjection = *m_Camera.GetProjMatrix();
    m_pEffect->SetMatrix("g_mProjection", &mProjection);
    m_pEffect->SetFloat( "g_fBloomScale", 1.0f );
    m_pEffect->SetFloat( "g_fStarScale", 0.5f );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ClearTexture()
// Desc: Helper function for RestoreDeviceObjects to clear a texture surface
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ClearTexture( LPDIRECT3DTEXTURE9 pTexture )
{
    HRESULT hr = S_OK;
    PDIRECT3DSURFACE9 pSurface = NULL;

    hr = pTexture->GetSurfaceLevel( 0, &pSurface );
    if( SUCCEEDED(hr) )
        m_pd3dDevice->ColorFill( pSurface, NULL, D3DCOLOR_ARGB(0, 0, 0, 0) );

    SAFE_RELEASE( pSurface );
    return hr;
}




//-----------------------------------------------------------------------------
// Name: SetTextureCoords()
// Desc: Helper function for BuildWorldMesh to set texture coordinates
//       for vertices
//-----------------------------------------------------------------------------
void SetTextureCoords( WorldVertex* pVertex, float u, float v )
{
    (pVertex++)->t = D3DXVECTOR2( 0.0f, 0.0f );
    (pVertex++)->t = D3DXVECTOR2( u,    0.0f );
    (pVertex++)->t = D3DXVECTOR2( u,    v    );
    (pVertex++)->t = D3DXVECTOR2( 0.0f, v    );
}




//-----------------------------------------------------------------------------
// Name: BuildColumn()
// Desc: Helper function for BuildWorldMesh to add column quads to the scene 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::BuildColumn( WorldVertex* &pV, float x, float y, float z, float width )
{
    float w = width / 2;

    SetTextureCoords( pV, 1.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( x - w, y,    z - w );
    (pV++)->p = D3DXVECTOR3( x + w, y,    z - w );
    (pV++)->p = D3DXVECTOR3( x + w, 0.0f, z - w );
    (pV++)->p = D3DXVECTOR3( x - w, 0.0f, z - w );

    SetTextureCoords( pV, 1.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( x + w, y,    z - w );
    (pV++)->p = D3DXVECTOR3( x + w, y,    z + w );
    (pV++)->p = D3DXVECTOR3( x + w, 0.0f, z + w );
    (pV++)->p = D3DXVECTOR3( x + w, 0.0f, z - w );

    SetTextureCoords( pV, 1.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( x + w, y,    z + w );
    (pV++)->p = D3DXVECTOR3( x - w, y,    z + w );
    (pV++)->p = D3DXVECTOR3( x - w, 0.0f, z + w );
    (pV++)->p = D3DXVECTOR3( x + w, 0.0f, z + w );

    SetTextureCoords( pV, 1.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( x - w, y,    z + w );
    (pV++)->p = D3DXVECTOR3( x - w, y,    z - w );
    (pV++)->p = D3DXVECTOR3( x - w, 0.0f, z - w );
    (pV++)->p = D3DXVECTOR3( x - w, 0.0f, z + w );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: BuildWorldMesh()
// Desc: Creates the wall, floor, ceiling, columns, and painting mesh
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::BuildWorldMesh()
{
    HRESULT hr;
    UINT i; // Loop variable
    
    const FLOAT fWidth  = 15.0f;
    const FLOAT fDepth  = 20.0f;
    const FLOAT fHeight = 3.0f;

    // Create the room  
    LPD3DXMESH pWorldMeshTemp = NULL;
    hr = D3DXCreateMeshFVF( 48, 96, 0, WorldVertex::FVF, m_pd3dDevice, &pWorldMeshTemp );
    if( FAILED(hr) )
        goto LCleanReturn;

    WorldVertex* pVertex;
    hr = pWorldMeshTemp->LockVertexBuffer(0, (PVOID*)&pVertex);
    if( FAILED(hr) )
        goto LCleanReturn;

    WorldVertex* pV;
    pV = pVertex;

    // Front wall
    SetTextureCoords( pV, 7.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( 0.0f,   fHeight, fDepth );   
    (pV++)->p = D3DXVECTOR3( fWidth, fHeight, fDepth );  
    (pV++)->p = D3DXVECTOR3( fWidth, 0.0f,    fDepth );  
    (pV++)->p = D3DXVECTOR3( 0.0f,   0.0f,    fDepth );  

    // Right wall
    SetTextureCoords( pV, 10.5f, 2.0f );
    (pV++)->p = D3DXVECTOR3( fWidth, fHeight, fDepth );
    (pV++)->p = D3DXVECTOR3( fWidth, fHeight, 0.0f   );
    (pV++)->p = D3DXVECTOR3( fWidth, 0.0f,    0.0f   );
    (pV++)->p = D3DXVECTOR3( fWidth, 0.0f,    fDepth );

    // Back wall
    SetTextureCoords( pV, 7.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( fWidth, fHeight, 0.0f   );
    (pV++)->p = D3DXVECTOR3( 0.0f,   fHeight, 0.0f   );
    (pV++)->p = D3DXVECTOR3( 0.0f,   0.0f,    0.0f   );
    (pV++)->p = D3DXVECTOR3( fWidth, 0.0f,    0.0f   );

    // Left wall
    SetTextureCoords( pV, 10.5f, 2.0f );
    (pV++)->p = D3DXVECTOR3( 0.0f,   fHeight, 0.0f   );
    (pV++)->p = D3DXVECTOR3( 0.0f,   fHeight, fDepth );
    (pV++)->p = D3DXVECTOR3( 0.0f,   0.0f,    fDepth );
    (pV++)->p = D3DXVECTOR3( 0.0f,   0.0f,    0.0f   );

    BuildColumn( pV, 4.0f, fHeight, 7.0f, 0.75f );
    BuildColumn( pV, 4.0f, fHeight, 13.0f, 0.75f );
    BuildColumn( pV, 11.0f, fHeight, 7.0f, 0.75f );
    BuildColumn( pV, 11.0f, fHeight, 13.0f, 0.75f );

    // Floor
    SetTextureCoords( pV, 7.0f, 7.0f );
    (pV++)->p = D3DXVECTOR3( 0.0f,   0.0f,    fDepth );
    (pV++)->p = D3DXVECTOR3( fWidth, 0.0f,    fDepth );
    (pV++)->p = D3DXVECTOR3( fWidth, 0.0f,    0.0f   );
    (pV++)->p = D3DXVECTOR3( 0.0f,   0.0f,    0.0f   );

    // Ceiling
    SetTextureCoords( pV, 7.0f, 2.0f );
    (pV++)->p = D3DXVECTOR3( 0.0f,   fHeight, 0.0f   );
    (pV++)->p = D3DXVECTOR3( fWidth, fHeight, 0.0f   );
    (pV++)->p = D3DXVECTOR3( fWidth, fHeight, fDepth );
    (pV++)->p = D3DXVECTOR3( 0.0f,   fHeight, fDepth );

    // Painting 1
    SetTextureCoords( pV, 1.0f, 1.0f );
    (pV++)->p = D3DXVECTOR3( 2.0f,   fHeight - 0.5f, fDepth - 0.01f );
    (pV++)->p = D3DXVECTOR3( 6.0f,   fHeight - 0.5f, fDepth - 0.01f );
    (pV++)->p = D3DXVECTOR3( 6.0f,   fHeight - 2.5f, fDepth - 0.01f );
    (pV++)->p = D3DXVECTOR3( 2.0f,   fHeight - 2.5f, fDepth - 0.01f );

    // Painting 2
    SetTextureCoords( pV, 1.0f, 1.0f );
    (pV++)->p = D3DXVECTOR3(  9.0f,   fHeight - 0.5f, fDepth - 0.01f );
    (pV++)->p = D3DXVECTOR3( 13.0f,   fHeight - 0.5f, fDepth - 0.01f );
    (pV++)->p = D3DXVECTOR3( 13.0f,   fHeight - 2.5f, fDepth - 0.01f );
    (pV++)->p = D3DXVECTOR3(  9.0f,   fHeight - 2.5f, fDepth - 0.01f );

    pWorldMeshTemp->UnlockVertexBuffer();

    // Retrieve the indices
    WORD* pIndex;
    hr = pWorldMeshTemp->LockIndexBuffer(0, (PVOID*)&pIndex);
    if(FAILED(hr))
        goto LCleanReturn;

    for( i=0; i < pWorldMeshTemp->GetNumFaces()/2; i++ )
    {
        *pIndex++ = (i*4) + 0;
        *pIndex++ = (i*4) + 1;
        *pIndex++ = (i*4) + 2;
        *pIndex++ = (i*4) + 0;
        *pIndex++ = (i*4) + 2;
        *pIndex++ = (i*4) + 3;
    }

    pWorldMeshTemp->UnlockIndexBuffer();
    
    // Set attribute groups to draw floor, ceiling, walls, and paintings
    // separately, with different shader constants. These group numbers
    // will be used during the calls to DrawSubset().
    DWORD* pAttribs;
    hr = pWorldMeshTemp->LockAttributeBuffer(0, &pAttribs);
    if( FAILED(hr) )
        goto LCleanReturn;

    for( i=0; i < 40; i++ )
        *pAttribs++ = 0;

    for( i=0; i < 2; i++ )
        *pAttribs++ = 1;

    for( i=0; i < 2; i++ )
        *pAttribs++ = 2;

    for( i=0; i < 4; i++ )
        *pAttribs++ = 3;

    pWorldMeshTemp->UnlockAttributeBuffer();
    D3DXComputeNormals( pWorldMeshTemp, NULL );

    // Optimize the mesh
    hr = pWorldMeshTemp->CloneMeshFVF( D3DXMESH_VB_WRITEONLY | D3DXMESH_IB_WRITEONLY, 
                                       WorldVertex::FVF, m_pd3dDevice, &m_pWorldMesh );
    if( FAILED(hr) )
        goto LCleanReturn;

    hr = S_OK;

LCleanReturn:
    SAFE_RELEASE( pWorldMeshTemp );
    return hr;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Adjust the camera position based on user input
    m_Camera.FrameMove(m_fElapsedTime);

    // Set the flag to refresh the user's simulated adaption level.
    // Frame move is not called when the scene is paused or single-stepped. 
    // If the scene is paused, the user's adaptation level needs to remain
    // unchanged.
    m_bAdaptationInvalid = true;

    // Calculate the position of the lights in view space
    D3DXVECTOR4 avLightViewPosition[NUM_LIGHTS]; 
    for(int iLight=0; iLight < NUM_LIGHTS; iLight++)
    {
        D3DXMATRIX mView = *m_Camera.GetViewMatrix();
        D3DXVec4Transform(&avLightViewPosition[iLight], &m_avLightPosition[iLight], &mView);
    }

    // Set frame shader constants
    m_pEffect->SetBool("g_bEnableToneMap", m_bToneMap);
    m_pEffect->SetBool("g_bEnableBlueShift", m_bBlueShift);
    m_pEffect->SetValue("g_avLightPositionView", avLightViewPosition, sizeof(D3DXVECTOR4) * NUM_LIGHTS);
    m_pEffect->SetValue("g_avLightIntensity", m_avLightIntensity, sizeof(D3DXVECTOR4) * NUM_LIGHTS);
    
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetTextureRect()
// Desc: Get the dimensions of the texture
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetTextureRect( PDIRECT3DTEXTURE9 pTexture, RECT* pRect )
{
    HRESULT hr = S_OK;

    if( pTexture == NULL || pRect == NULL )
        return E_INVALIDARG;

    D3DSURFACE_DESC desc;
    hr = pTexture->GetLevelDesc( 0, &desc );
    if( FAILED(hr) )
        return hr;

    pRect->left = 0;
    pRect->top = 0;
    pRect->right = desc.Width;
    pRect->bottom = desc.Height;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetTextureCoords()
// Desc: Get the texture coordinates to use when rendering into the destination
//       texture, given the source and destination rectangles
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetTextureCoords( PDIRECT3DTEXTURE9 pTexSrc, RECT* pRectSrc, 
                          PDIRECT3DTEXTURE9 pTexDest, RECT* pRectDest, CoordRect* pCoords )
{
    HRESULT hr = S_OK;
    D3DSURFACE_DESC desc;
    float tU, tV;

    // Validate arguments
    if( pTexSrc == NULL || pTexDest == NULL || pCoords == NULL )
        return E_INVALIDARG;

    // Start with a default mapping of the complete source surface to complete 
    // destination surface
    pCoords->fLeftU = 0.0f;
    pCoords->fTopV = 0.0f;
    pCoords->fRightU = 1.0f; 
    pCoords->fBottomV = 1.0f;

    // If not using the complete source surface, adjust the coordinates
    if( pRectSrc != NULL )
    {
        // Get destination texture description
        hr = pTexSrc->GetLevelDesc( 0, &desc );
        if( FAILED(hr) )
            return hr;

        // These delta values are the distance between source texel centers in 
        // texture address space
        tU = 1.0f / desc.Width;
        tV = 1.0f / desc.Height;

        pCoords->fLeftU += pRectSrc->left * tU;
        pCoords->fTopV += pRectSrc->top * tV;
        pCoords->fRightU -= (desc.Width - pRectSrc->right) * tU;
        pCoords->fBottomV -= (desc.Height - pRectSrc->bottom) * tV;
    }

    // If not drawing to the complete destination surface, adjust the coordinates
    if( pRectDest != NULL )
    {
        // Get source texture description
        hr = pTexDest->GetLevelDesc( 0, &desc );
        if( FAILED(hr) )
            return hr;

        // These delta values are the distance between source texel centers in 
        // texture address space
        tU = 1.0f / desc.Width;
        tV = 1.0f / desc.Height;

        pCoords->fLeftU -= pRectDest->left * tU;
        pCoords->fTopV -= pRectDest->top * tV;
        pCoords->fRightU += (desc.Width - pRectDest->right) * tU;
        pCoords->fBottomV += (desc.Height - pRectDest->bottom) * tV;
    }

    return S_OK;
}
  



//-----------------------------------------------------------------------------
// Name: Scene_To_SceneScaled()
// Desc: Scale down m_pTexScene by 1/4 x 1/4 and place the result in 
//       m_pTexSceneScaled
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Scene_To_SceneScaled()
{
    HRESULT hr = S_OK;
    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];
    
   
    // Get the new render target surface
    PDIRECT3DSURFACE9 pSurfScaledScene = NULL;
    hr = m_pTexSceneScaled->GetSurfaceLevel( 0, &pSurfScaledScene );
    if( FAILED(hr) )
        goto LCleanReturn;


    // Create a 1/4 x 1/4 scale copy of the HDR texture. Since bloom textures
    // are 1/8 x 1/8 scale, border texels of the HDR texture will be discarded 
    // to keep the dimensions evenly divisible by 8; this allows for precise 
    // control over sampling inside pixel shaders.
    m_pEffect->SetTechnique("DownScale4x4");

    // Place the rectangle in the center of the back buffer surface
    RECT rectSrc;
    rectSrc.left = (m_d3dsdBackBuffer.Width - m_dwCropWidth) / 2;
    rectSrc.top = (m_d3dsdBackBuffer.Height - m_dwCropHeight) / 2;
    rectSrc.right = rectSrc.left + m_dwCropWidth;
    rectSrc.bottom = rectSrc.top + m_dwCropHeight;

    // Get the texture coordinates for the render target
    CoordRect coords;
    GetTextureCoords( m_pTexScene, &rectSrc, m_pTexSceneScaled, NULL, &coords );

    // Get the sample offsets used within the pixel shader
    GetSampleOffsets_DownScale4x4( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, avSampleOffsets );
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    
    m_pd3dDevice->SetRenderTarget( 0, pSurfScaledScene );
    m_pd3dDevice->SetTexture( 0, m_pTexScene );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
   
    UINT uiPassCount, uiPass;       
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad
        DrawFullScreenQuad( coords );
    }

    m_pEffect->End();
    

    hr = S_OK;
LCleanReturn:
    SAFE_RELEASE( pSurfScaledScene );
    return hr;
}




//-----------------------------------------------------------------------------
// Name: SceneScaled_To_BrightPass
// Desc: Run the bright-pass filter on m_pTexSceneScaled and place the result
//       in m_pTexBrightPass
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::SceneScaled_To_BrightPass()
{
    HRESULT hr = S_OK;

    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];
    D3DXVECTOR4 avSampleWeights[MAX_SAMPLES];

    
    // Get the new render target surface
    PDIRECT3DSURFACE9 pSurfBrightPass;
    hr = m_pTexBrightPass->GetSurfaceLevel( 0, &pSurfBrightPass );
    if( FAILED(hr) )
        goto LCleanReturn;

    
    D3DSURFACE_DESC desc;
    m_pTexSceneScaled->GetLevelDesc( 0, &desc );

    // Get the offsets to be used within the GaussBlur5x5 pixel shader
    hr = GetSampleOffsets_GaussBlur5x5( desc.Width, desc.Height, avSampleOffsets, avSampleWeights );
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets) );
    m_pEffect->SetValue("g_avSampleWeights", avSampleWeights, sizeof(avSampleWeights) );
    

    // Get the rectangle describing the sampled portion of the source texture.
    // Decrease the rectangle to adjust for the single pixel black border.
    RECT rectSrc;
    GetTextureRect( m_pTexSceneScaled, &rectSrc );
    InflateRect( &rectSrc, -1, -1 );

    // Get the destination rectangle.
    // Decrease the rectangle to adjust for the single pixel black border.
    RECT rectDest;
    GetTextureRect( m_pTexBrightPass, &rectDest );
    InflateRect( &rectDest, -1, -1 );

    // Get the correct texture coordinates to apply to the rendered quad in order 
    // to sample from the source rectangle and render into the destination rectangle
    CoordRect coords;
    GetTextureCoords( m_pTexSceneScaled, &rectSrc, m_pTexBrightPass, &rectDest, &coords );

    // The bright-pass filter removes everything from the scene except lights and
    // bright reflections
    m_pEffect->SetTechnique("BrightPassFilter");

    m_pd3dDevice->SetRenderTarget( 0, pSurfBrightPass );
    m_pd3dDevice->SetTexture( 0, m_pTexSceneScaled );
    m_pd3dDevice->SetTexture( 1, m_pTexAdaptedLuminanceCur );
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
    m_pd3dDevice->SetScissorRect( &rectDest );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
       
    UINT uiPass, uiPassCount;
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;
    
    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad( coords );
    }
    
    m_pEffect->End();
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

    hr = S_OK;
LCleanReturn:
    SAFE_RELEASE( pSurfBrightPass );
    return hr;
}




//-----------------------------------------------------------------------------
// Name: BrightPass_To_StarSource
// Desc: Perform a 5x5 gaussian blur on m_pTexBrightPass and place the result
//       in m_pTexStarSource. The bright-pass filtered image is blurred before
//       being used for star operations to avoid aliasing artifacts.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::BrightPass_To_StarSource()
{
    HRESULT hr = S_OK;

    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];
    D3DXVECTOR4 avSampleWeights[MAX_SAMPLES];

    // Get the new render target surface
    PDIRECT3DSURFACE9 pSurfStarSource;
    hr = m_pTexStarSource->GetSurfaceLevel( 0, &pSurfStarSource );
    if( FAILED(hr) )
        goto LCleanReturn;

    
    // Get the destination rectangle.
    // Decrease the rectangle to adjust for the single pixel black border.
    RECT rectDest;
    GetTextureRect( m_pTexStarSource, &rectDest );
    InflateRect( &rectDest, -1, -1 );

    // Get the correct texture coordinates to apply to the rendered quad in order 
    // to sample from the source rectangle and render into the destination rectangle
    CoordRect coords;
    GetTextureCoords( m_pTexBrightPass, NULL, m_pTexStarSource, &rectDest, &coords );

    // Get the sample offsets used within the pixel shader
    D3DSURFACE_DESC desc;
    hr = m_pTexBrightPass->GetLevelDesc( 0, &desc );
    if( FAILED(hr) )
        return hr;

    
    GetSampleOffsets_GaussBlur5x5( desc.Width, desc.Height, avSampleOffsets, avSampleWeights );
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    m_pEffect->SetValue("g_avSampleWeights", avSampleWeights, sizeof(avSampleWeights));
    
    // The gaussian blur smooths out rough edges to avoid aliasing effects
    // when the star effect is run
    m_pEffect->SetTechnique("GaussBlur5x5");

    m_pd3dDevice->SetRenderTarget( 0, pSurfStarSource );
    m_pd3dDevice->SetTexture( 0, m_pTexBrightPass );
    m_pd3dDevice->SetScissorRect( &rectDest );
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
   
    UINT uiPassCount, uiPass;       
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad
        DrawFullScreenQuad( coords );
    }

    m_pEffect->End();
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

  
    hr = S_OK;
LCleanReturn:
    SAFE_RELEASE( pSurfStarSource);
    return hr;
}




//-----------------------------------------------------------------------------
// Name: StarSource_To_BloomSource
// Desc: Scale down m_pTexStarSource by 1/2 x 1/2 and place the result in 
//       m_pTexBloomSource
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::StarSource_To_BloomSource()
{
    HRESULT hr = S_OK;

    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];
    
    // Get the new render target surface
    PDIRECT3DSURFACE9 pSurfBloomSource;
    hr = m_pTexBloomSource->GetSurfaceLevel( 0, &pSurfBloomSource );
    if( FAILED(hr) )
        goto LCleanReturn;

    
    // Get the rectangle describing the sampled portion of the source texture.
    // Decrease the rectangle to adjust for the single pixel black border.
    RECT rectSrc;
    GetTextureRect( m_pTexStarSource, &rectSrc );
    InflateRect( &rectSrc, -1, -1 );

    // Get the destination rectangle.
    // Decrease the rectangle to adjust for the single pixel black border.
    RECT rectDest;
    GetTextureRect( m_pTexBloomSource, &rectDest );
    InflateRect( &rectDest, -1, -1 );

    // Get the correct texture coordinates to apply to the rendered quad in order 
    // to sample from the source rectangle and render into the destination rectangle
    CoordRect coords;
    GetTextureCoords( m_pTexStarSource, &rectSrc, m_pTexBloomSource, &rectDest, &coords );

    // Get the sample offsets used within the pixel shader
    D3DSURFACE_DESC desc;
    hr = m_pTexBrightPass->GetLevelDesc( 0, &desc );
    if( FAILED(hr) )
        return hr;

    GetSampleOffsets_DownScale2x2( desc.Width, desc.Height, avSampleOffsets );
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));

    // Create an exact 1/2 x 1/2 copy of the source texture
    m_pEffect->SetTechnique("DownScale2x2");

    m_pd3dDevice->SetRenderTarget( 0, pSurfBloomSource );
    m_pd3dDevice->SetTexture( 0, m_pTexStarSource );
    m_pd3dDevice->SetScissorRect( &rectDest );
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
   
    UINT uiPassCount, uiPass;       
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad
        DrawFullScreenQuad( coords );
    }

    m_pEffect->End();
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );


    
    hr = S_OK;
LCleanReturn:
    SAFE_RELEASE( pSurfBloomSource);
    return hr;
}




//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale4x4
// Desc: Get the texture coordinate offsets to be used inside the DownScale4x4
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetSampleOffsets_DownScale4x4( DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 avSampleOffsets[] )
{
    if( NULL == avSampleOffsets )
        return E_INVALIDARG;

    float tU = 1.0f / dwWidth;
    float tV = 1.0f / dwHeight;

    // Sample from the 16 surrounding points. Since the center point will be in
    // the exact center of 16 texels, a 0.5f offset is needed to specify a texel
    // center.
    int index=0;
    for( int y=0; y < 4; y++ )
    {
        for( int x=0; x < 4; x++ )
        {
            avSampleOffsets[ index ].x = (x - 1.5f) * tU;
            avSampleOffsets[ index ].y = (y - 1.5f) * tV;
                                                      
            index++;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_DownScale2x2
// Desc: Get the texture coordinate offsets to be used inside the DownScale2x2
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetSampleOffsets_DownScale2x2( DWORD dwWidth, DWORD dwHeight, D3DXVECTOR2 avSampleOffsets[] )
{
    if( NULL == avSampleOffsets )
        return E_INVALIDARG;

    float tU = 1.0f / dwWidth;
    float tV = 1.0f / dwHeight;

    // Sample from the 4 surrounding points. Since the center point will be in
    // the exact center of 4 texels, a 0.5f offset is needed to specify a texel
    // center.
    int index=0;
    for( int y=0; y < 2; y++ )
    {
        for( int x=0; x < 2; x++ )
        {
            avSampleOffsets[ index ].x = (x - 0.5f) * tU;
            avSampleOffsets[ index ].y = (y - 0.5f) * tV;
                                                      
            index++;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_GaussBlur5x5
// Desc: Get the texture coordinate offsets to be used inside the GaussBlur5x5
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetSampleOffsets_GaussBlur5x5(DWORD dwD3DTexWidth,
                                                         DWORD dwD3DTexHeight,
                                                         D3DXVECTOR2* avTexCoordOffset,
                                                         D3DXVECTOR4* avSampleWeight,
                                                         FLOAT fMultiplier )
{
    float tu = 1.0f / (float)dwD3DTexWidth ;
    float tv = 1.0f / (float)dwD3DTexHeight ;

    D3DXVECTOR4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );
    
    float totalWeight = 0.0f;
    int index=0;
    for( int x = -2; x <= 2; x++ )
    {
        for( int y = -2; y <= 2; y++ )
        {
            // Exclude pixels with a block distance greater than 2. This will
            // create a kernel which approximates a 5x5 kernel using only 13
            // sample points instead of 25; this is necessary since 2.0 shaders
            // only support 16 texture grabs.
            if( abs(x) + abs(y) > 2 )
                continue;

            // Get the unscaled Gaussian intensity for this offset
            avTexCoordOffset[index] = D3DXVECTOR2( x * tu, y * tv );
            avSampleWeight[index] = vWhite * GaussianDistribution( (float)x, (float)y, 1.0f );
            totalWeight += avSampleWeight[index].x;

            index++;
        }
    }

    // Divide the current weight by the total weight of all the samples; Gaussian
    // blur kernels add to 1.0f to ensure that the intensity of the image isn't
    // changed when the blur occurs. An optional multiplier variable is used to
    // add or remove image intensity during the blur.
    for( int i=0; i < index; i++ )
    {
        avSampleWeight[i] /= totalWeight;
        avSampleWeight[i] *= fMultiplier;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_Bloom
// Desc: Get the texture coordinate offsets to be used inside the Bloom
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetSampleOffsets_Bloom( DWORD dwD3DTexSize,
                                                   float afTexCoordOffset[15],
                                                   D3DXVECTOR4* avColorWeight,
                                                   float fDeviation,
                                                   float fMultiplier )
{
    int i=0;
    float tu = 1.0f / (float)dwD3DTexSize;

    // Fill the center texel
    float weight = fMultiplier * GaussianDistribution( 0, 0, fDeviation );
    avColorWeight[0] = D3DXVECTOR4( weight, weight, weight, 1.0f );

    afTexCoordOffset[0] = 0.0f;
    
    // Fill the first half
    for( i=1; i < 8; i++ )
    {
        // Get the Gaussian intensity for this offset
        weight = fMultiplier * GaussianDistribution( (float)i, 0, fDeviation );
        afTexCoordOffset[i] = i * tu;

        avColorWeight[i] = D3DXVECTOR4( weight, weight, weight, 1.0f );
    }

    // Mirror to the second half
    for( i=8; i < 15; i++ )
    {
        avColorWeight[i] = avColorWeight[i-7];
        afTexCoordOffset[i] = -afTexCoordOffset[i-7];
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetSampleOffsets_Bloom
// Desc: Get the texture coordinate offsets to be used inside the Bloom
//       pixel shader.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GetSampleOffsets_Star(DWORD dwD3DTexSize,
                                                float afTexCoordOffset[15],
                                                D3DXVECTOR4* avColorWeight,
                                                float fDeviation)
{
    int i=0;
    float tu = 1.0f / (float)dwD3DTexSize;

    // Fill the center texel
    float weight = 1.0f * GaussianDistribution( 0, 0, fDeviation );
    avColorWeight[0] = D3DXVECTOR4( weight, weight, weight, 1.0f );

    afTexCoordOffset[0] = 0.0f;
    
    // Fill the first half
    for( i=1; i < 8; i++ )
    {
        // Get the Gaussian intensity for this offset
        weight = 1.0f * GaussianDistribution( (float)i, 0, fDeviation );
        afTexCoordOffset[i] = i * tu;

        avColorWeight[i] = D3DXVECTOR4( weight, weight, weight, 1.0f );
    }

    // Mirror to the second half
    for( i=8; i < 15; i++ )
    {
        avColorWeight[i] = avColorWeight[i-7];
        afTexCoordOffset[i] = -afTexCoordOffset[i-7];
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GaussianDistribution
// Desc: Helper function for GetSampleOffsets function to compute the 
//       2 parameter Gaussian distrubution using the given standard deviation
//       rho
//-----------------------------------------------------------------------------
float CMyD3DApplication::GaussianDistribution( float x, float y, float rho )
{
    float g = 1.0f / sqrtf( 2.0f * D3DX_PI * rho * rho );
    g *= expf( -(x*x + y*y)/(2*rho*rho) );

    return g;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
    HRESULT hr = S_OK;

    PDIRECT3DSURFACE9 pSurfLDR; // Low dynamic range surface for final output
    PDIRECT3DSURFACE9 pSurfHDR; // High dynamic range surface to store 
                                // intermediate floating point color values
    
    // Setup HDR render target
    m_pd3dDevice->GetRenderTarget(0, &pSurfLDR);
    m_pTexScene->GetSurfaceLevel(0, &pSurfHDR);
    
    
    // Clear the viewport
    m_pd3dDevice->SetRenderTarget(0, pSurfHDR);
    m_pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 0), 1.0f, 0L);

    // Begin the scene
    if(SUCCEEDED(m_pd3dDevice->BeginScene()))
    {

        // Render the HDR Scene
        RenderScene();

        // Create a scaled copy of the scene
        Scene_To_SceneScaled();

        // Setup tone mapping technique
        if( m_bToneMap )
            MeasureLuminance();
      
        // If FrameMove has been called, the user's adaptation level has also changed
        // and should be updated
        if( m_bAdaptationInvalid )
        {
            // Clear the update flag
            m_bAdaptationInvalid = false;

            // Calculate the current luminance adaptation level
            CalculateAdaptation();
        }

        // Now that luminance information has been gathered, the scene can be bright-pass filtered
        // to remove everything except bright lights and reflections.
        SceneScaled_To_BrightPass();

        // Blur the bright-pass filtered image to create the source texture for the star effect
        BrightPass_To_StarSource();

        // Scale-down the source texture for the star effect to create the source texture
        // for the bloom effect
        StarSource_To_BloomSource();

        // Render post-process lighting effects
        RenderBloom();
        RenderStar();
              
        // Draw the high dynamic range scene texture to the low dynamic range
        // back buffer. As part of this final pass, the scene will be tone-mapped
        // using the user's current adapted luminance, blue shift will occur
        // if the scene is determined to be very dark, and the post-process lighting
        // effect textures will be added to the scene.
        UINT uiPassCount, uiPass;
        
        m_pEffect->SetTechnique("FinalScenePass");
        m_pEffect->SetFloat("g_fMiddleGray", m_fKeyValue);
        
        m_pd3dDevice->SetRenderTarget(0, pSurfLDR);
        m_pd3dDevice->SetTexture( 0, m_pTexScene );
        m_pd3dDevice->SetTexture( 1, m_apTexBloom[0] );
        m_pd3dDevice->SetTexture( 2, m_apTexStar[0] );
        m_pd3dDevice->SetTexture( 3, m_pTexAdaptedLuminanceCur );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( 2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        m_pd3dDevice->SetSamplerState( 2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( 3, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 3, D3DSAMP_MINFILTER, D3DTEXF_POINT );
        
   
        hr = m_pEffect->Begin(&uiPassCount, 0);
        if( FAILED(hr) )
            return hr;

        for (uiPass = 0; uiPass < uiPassCount; uiPass++)
        {
            m_pEffect->Pass(uiPass);

            
            DrawFullScreenQuad( 0.0f, 0.0f, 1.0f, 1.0f );
        }

        m_pEffect->End();
               
        // Draw scene statistics & help 
        RenderText();

        // End the scene.
        m_pd3dDevice->EndScene();
    }

    // Release surfaces
    SAFE_RELEASE(pSurfHDR);
    SAFE_RELEASE(pSurfLDR);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderScene()
// Desc: Render the world objects and lights
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderScene()
{
    HRESULT hr = S_OK;

    UINT uiPassCount, uiPass;
    D3DXMATRIXA16 mWorld;
    D3DXMATRIXA16 mTrans;
    D3DXMATRIXA16 mRotate;
    D3DXMATRIXA16 mObjectToView;

    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP );
    
    D3DXMATRIX mView = *m_Camera.GetViewMatrix();
    
    m_pEffect->SetTechnique("RenderScene");
    m_pEffect->SetMatrix("g_mObjectToView", &mView);
    
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        return hr;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Turn off emissive lighting
        D3DXVECTOR4 vNull( 0.0f, 0.0f, 0.0f, 0.0f );
        m_pEffect->SetVector("g_vEmissive", &vNull); 
        
        // Enable texture
        m_pEffect->SetBool("g_bEnableTexture", true);
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    
    
        // Render walls and columns
        m_pEffect->SetFloat("g_fPhongExponent", 5.0f);
        m_pEffect->SetFloat("g_fPhongCoefficient", 1.0f);
        m_pEffect->SetFloat("g_fDiffuseCoefficient", 0.5f);
        m_pd3dDevice->SetTexture(0, m_pTexWall );
        m_pWorldMesh->DrawSubset(0);

        // Render floor
        m_pEffect->SetFloat("g_fPhongExponent", 50.0f);
        m_pEffect->SetFloat("g_fPhongCoefficient", 3.0f);
        m_pEffect->SetFloat("g_fDiffuseCoefficient", 1.0f);
        m_pd3dDevice->SetTexture(0, m_pTexFloor );
        m_pWorldMesh->DrawSubset(1);

        // Render ceiling
        m_pEffect->SetFloat("g_fPhongExponent", 5.0f);
        m_pEffect->SetFloat("g_fPhongCoefficient", 0.3f);
        m_pEffect->SetFloat("g_fDiffuseCoefficient", 0.3f);
        m_pd3dDevice->SetTexture(0, m_pTexCeiling );
        m_pWorldMesh->DrawSubset(2);

        // Render paintings
        m_pEffect->SetFloat("g_fPhongExponent", 5.0f);
        m_pEffect->SetFloat("g_fPhongCoefficient", 0.3f);
        m_pEffect->SetFloat("g_fDiffuseCoefficient", 1.0f);
        m_pd3dDevice->SetTexture(0, m_pTexPainting );
        m_pWorldMesh->DrawSubset(3);

        // Draw the light spheres.
        m_pEffect->SetFloat("g_fPhongExponent", 5.0f);
        m_pEffect->SetFloat("g_fPhongCoefficient", 1.0f);
        m_pEffect->SetFloat("g_fDiffuseCoefficient", 1.0f);
        m_pEffect->SetBool("g_bEnableTexture", false);
        
        for(int iLight=0; iLight < NUM_LIGHTS; iLight++)
        {  
            // Just position the point light -- no need to orient it
            D3DXMATRIXA16 mScale;
            D3DXMatrixScaling( &mScale, 0.05f, 0.05f, 0.05f );
            
            mView = *m_Camera.GetViewMatrix();
            D3DXMatrixTranslation(&mWorld, m_avLightPosition[iLight].x, m_avLightPosition[iLight].y, m_avLightPosition[iLight].z);
            mWorld = mScale * mWorld;
            mObjectToView = mWorld * mView;
            m_pEffect->SetMatrix("g_mObjectToView", &mObjectToView);

            // A light which illuminates objects at 80 lum/sr should be drawn
            // at 3183 lumens/meter^2/steradian, which equates to a multiplier
            // of 39.78 per lumen.
            D3DXVECTOR4 vEmissive = EMISSIVE_COEFFICIENT * m_avLightIntensity[iLight];
            m_pEffect->SetVector("g_vEmissive", &vEmissive );    
    
            m_pmeshSphere->DrawSubset(0);
        }
    }

    m_pEffect->End();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MeasureLuminance()
// Desc: Measure the average log luminance in the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::MeasureLuminance()
{
    HRESULT hr = S_OK;
    UINT uiPassCount, uiPass;
    int i, x, y, index;
    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];

    
    DWORD dwCurTexture = NUM_TONEMAP_TEXTURES-1;

    // Sample log average luminance
    PDIRECT3DSURFACE9 apSurfToneMap[NUM_TONEMAP_TEXTURES] = {0};

    // Retrieve the tonemap surfaces
    for( i=0; i < NUM_TONEMAP_TEXTURES; i++ )
    {
        hr = m_apTexToneMap[i]->GetSurfaceLevel( 0, &apSurfToneMap[i] );
        if( FAILED(hr) )
            goto LCleanReturn;
    }

    D3DSURFACE_DESC desc;
    m_apTexToneMap[dwCurTexture]->GetLevelDesc( 0, &desc );

    
    // Initialize the sample offsets for the initial luminance pass.
    float tU, tV;
    tU = 1.0f / (3.0f * desc.Width);
    tV = 1.0f / (3.0f * desc.Height);
    
    index=0;
    for( x = -1; x <= 1; x++ )
    {
        for( y = -1; y <= 1; y++ )
        {
            avSampleOffsets[index].x = x * tU;
            avSampleOffsets[index].y = y * tV;

            index++;
        }
    }

    
    // After this pass, the m_apTexToneMap[NUM_TONEMAP_TEXTURES-1] texture will contain
    // a scaled, grayscale copy of the HDR scene. Individual texels contain the log 
    // of average luminance values for points sampled on the HDR texture.
    m_pEffect->SetTechnique("SampleAvgLum");
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    
    m_pd3dDevice->SetRenderTarget(0, apSurfToneMap[dwCurTexture]);
    m_pd3dDevice->SetTexture(0, m_pTexSceneScaled);
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    
    
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
    }

    m_pEffect->End();
    dwCurTexture--;
    
    // Initialize the sample offsets for the iterative luminance passes
    while( dwCurTexture > 0 )
    {
        m_apTexToneMap[dwCurTexture+1]->GetLevelDesc( 0, &desc );
        GetSampleOffsets_DownScale4x4( desc.Width, desc.Height, avSampleOffsets );
    

        // Each of these passes continue to scale down the log of average
        // luminance texture created above, storing intermediate results in 
        // m_apTexToneMap[1] through m_apTexToneMap[NUM_TONEMAP_TEXTURES-1].
        m_pEffect->SetTechnique("ResampleAvgLum");
        m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));

        m_pd3dDevice->SetRenderTarget(0, apSurfToneMap[dwCurTexture]);
        m_pd3dDevice->SetTexture(0, m_apTexToneMap[dwCurTexture+1]);
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
        m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    
        
        hr = m_pEffect->Begin(&uiPassCount, 0);
        if( FAILED(hr) )
            goto LCleanReturn;
        
        for (uiPass = 0; uiPass < uiPassCount; uiPass++)
        {
            m_pEffect->Pass(uiPass);

            // Draw a fullscreen quad to sample the RT
            DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
        }

        m_pEffect->End();
        dwCurTexture--;
    }

    // Downsample to 1x1
    m_apTexToneMap[1]->GetLevelDesc( 0, &desc );
    GetSampleOffsets_DownScale4x4( desc.Width, desc.Height, avSampleOffsets );
    
 
    // Perform the final pass of the average luminance calculation. This pass
    // scales the 4x4 log of average luminance texture from above and performs
    // an exp() operation to return a single texel cooresponding to the average
    // luminance of the scene in m_apTexToneMap[0].
    m_pEffect->SetTechnique("ResampleAvgLumExp");
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    
    m_pd3dDevice->SetRenderTarget(0, apSurfToneMap[0]);
    m_pd3dDevice->SetTexture(0, m_apTexToneMap[1]);
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    
     
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
    }

    m_pEffect->End();
 

    hr = S_OK;
LCleanReturn:
    for( i=0; i < NUM_TONEMAP_TEXTURES; i++ )
    {
        SAFE_RELEASE( apSurfToneMap[i] );
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: CalculateAdaptation()
// Desc: Increment the user's adapted luminance
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::CalculateAdaptation()
{
    HRESULT hr = S_OK;
    UINT uiPass, uiPassCount;

    // Swap current & last luminance
    PDIRECT3DTEXTURE9 pTexSwap = m_pTexAdaptedLuminanceLast;
    m_pTexAdaptedLuminanceLast = m_pTexAdaptedLuminanceCur;
    m_pTexAdaptedLuminanceCur = pTexSwap;
    
    PDIRECT3DSURFACE9 pSurfAdaptedLum = NULL;
    hr = m_pTexAdaptedLuminanceCur->GetSurfaceLevel(0, &pSurfAdaptedLum);
    if( FAILED(hr) )
        return hr;

    // This simulates the light adaptation that occurs when moving from a 
    // dark area to a bright area, or vice versa. The m_pTexAdaptedLuminance
    // texture stores a single texel cooresponding to the user's adapted 
    // level.
    m_pEffect->SetTechnique("CalculateAdaptedLum");
    m_pEffect->SetFloat("g_fElapsedTime", m_fElapsedTime);
    
    m_pd3dDevice->SetRenderTarget(0, pSurfAdaptedLum);
    m_pd3dDevice->SetTexture(0, m_pTexAdaptedLuminanceLast);
    m_pd3dDevice->SetTexture(1, m_apTexToneMap[0]);
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 1, D3DSAMP_MINFILTER, D3DTEXF_POINT );

    
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        return hr;
    
    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
    }
    
    m_pEffect->End();


    SAFE_RELEASE( pSurfAdaptedLum );
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderStar()
// Desc: Render the blooming effect
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderStar()
{
    HRESULT hr = S_OK;
    UINT uiPassCount, uiPass;
    int i, d, p, s; // Loop variables

    LPDIRECT3DSURFACE9 pSurfStar = NULL;
    hr = m_apTexStar[0]->GetSurfaceLevel( 0, &pSurfStar );
    if( FAILED(hr) )
        return hr;

    // Clear the star texture
    m_pd3dDevice->ColorFill( pSurfStar, NULL, D3DCOLOR_ARGB(0, 0, 0, 0) );
    SAFE_RELEASE( pSurfStar );

    // Avoid rendering the star if it's not being used in the current glare
    if( m_GlareDef.m_fGlareLuminance <= 0.0f ||
        m_GlareDef.m_fStarLuminance <= 0.0f )
    {
        return S_OK ;
    }

    // Initialize the constants used during the effect
    const CStarDef& starDef = m_GlareDef.m_starDef ;
    const float fTanFoV = atanf(D3DX_PI/8) ;
    const D3DXVECTOR4 vWhite( 1.0f, 1.0f, 1.0f, 1.0f );
    static const s_maxPasses = 3 ;
    static const int nSamples = 8 ;
    static D3DXVECTOR4 s_aaColor[s_maxPasses][8] ;
    static const D3DXCOLOR s_colorWhite(0.63f, 0.63f, 0.63f, 0.0f) ;
    
    D3DXVECTOR4 avSampleWeights[MAX_SAMPLES];
    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];
        
    m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE) ;
    m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE) ;

    PDIRECT3DSURFACE9 pSurfSource = NULL;
    PDIRECT3DSURFACE9 pSurfDest = NULL;

    // Set aside all the star texture surfaces as a convenience
    PDIRECT3DSURFACE9 apSurfStar[NUM_STAR_TEXTURES] = {0};
    for( i=0; i < NUM_STAR_TEXTURES; i++ )
    {
        hr = m_apTexStar[i]->GetSurfaceLevel( 0, &apSurfStar[i] );
        if( FAILED(hr) )
            goto LCleanReturn;
    }

    // Get the source texture dimensions
    hr = m_pTexStarSource->GetSurfaceLevel( 0, &pSurfSource );
    if( FAILED(hr) )
        goto LCleanReturn;

    D3DSURFACE_DESC desc;
    hr = pSurfSource->GetDesc( &desc );
    if( FAILED(hr) )
        goto LCleanReturn;

    SAFE_RELEASE( pSurfSource );

    float srcW;
    srcW = (FLOAT) desc.Width;
    float srcH;
    srcH= (FLOAT) desc.Height;

    
   
    for (p = 0 ; p < s_maxPasses ; p ++)
    {
        float ratio;
        ratio = (float)(p + 1) / (float)s_maxPasses ;
        
        for (s = 0 ; s < nSamples ; s ++)
        {
            D3DXCOLOR chromaticAberrColor ;
            D3DXColorLerp(&chromaticAberrColor,
                &( CStarDef::GetChromaticAberrationColor(s) ),
                &s_colorWhite,
                ratio) ;

            D3DXColorLerp( (D3DXCOLOR*)&( s_aaColor[p][s] ),
                &s_colorWhite, &chromaticAberrColor,
                m_GlareDef.m_fChromaticAberration ) ;
        }
    }

    float radOffset;
    radOffset = m_GlareDef.m_fStarInclination + starDef.m_fInclination ;
    

    PDIRECT3DTEXTURE9 pTexSource;


    // Direction loop
    for (d = 0 ; d < starDef.m_nStarLines ; d ++)
    {
        CONST STARLINE& starLine = starDef.m_pStarLine[d] ;

        pTexSource = m_pTexStarSource;
        
        float rad;
        rad = radOffset + starLine.fInclination ;
        float sn, cs;
        sn = sinf(rad), cs = cosf(rad) ;
        D3DXVECTOR2 vtStepUV;
        vtStepUV.x = sn / srcW * starLine.fSampleLength ;
        vtStepUV.y = cs / srcH * starLine.fSampleLength ;
        
        float attnPowScale;
        attnPowScale = (fTanFoV + 0.1f) * 1.0f *
                       (160.0f + 120.0f) / (srcW + srcH) * 1.2f ;

        // 1 direction expansion loop
        m_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE) ;
        
        int iWorkTexture;
        iWorkTexture = 1 ;
        for (p = 0 ; p < starLine.nPasses ; p ++)
        {
            
            if (p == starLine.nPasses - 1)
            {
                // Last pass move to other work buffer
                pSurfDest = apSurfStar[d+4];
            }
            else {
                pSurfDest = apSurfStar[iWorkTexture];
            }

            // Sampling configration for each stage
            for (i = 0 ; i < nSamples ; i ++)
            {
                float lum;
                lum = powf( starLine.fAttenuation, attnPowScale * i );
                
                avSampleWeights[i] = s_aaColor[starLine.nPasses - 1 - p][i] *
                                lum * (p+1.0f) * 0.5f ;
                                
                
                // Offset of sampling coordinate
                avSampleOffsets[i].x = vtStepUV.x * i ;
                avSampleOffsets[i].y = vtStepUV.y * i ;
                if ( fabs(avSampleOffsets[i].x) >= 0.9f ||
                     fabs(avSampleOffsets[i].y) >= 0.9f )
                {
                    avSampleOffsets[i].x = 0.0f ;
                    avSampleOffsets[i].y = 0.0f ;
                    avSampleWeights[i] *= 0.0f ;
                }
                
            }

            
            m_pEffect->SetTechnique("Star");
            m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
            m_pEffect->SetVectorArray("g_avSampleWeights", avSampleWeights, nSamples);
            
            m_pd3dDevice->SetRenderTarget( 0, pSurfDest );
            m_pd3dDevice->SetTexture( 0, pTexSource );
            m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
            m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    
            
            hr = m_pEffect->Begin(&uiPassCount, 0);
            if( FAILED(hr) )
                return hr;
            
            for (uiPass = 0; uiPass < uiPassCount; uiPass++)
            {
                m_pEffect->Pass(uiPass);

                // Draw a fullscreen quad to sample the RT
                DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
            }
            
            m_pEffect->End();

            // Setup next expansion
            vtStepUV *= nSamples ;
            attnPowScale *= nSamples ;

            // Set the work drawn just before to next texture source.
            pTexSource = m_apTexStar[iWorkTexture];

            iWorkTexture += 1 ;
            if (iWorkTexture > 2) {
                iWorkTexture = 1 ;
            }

        }
    }


    pSurfDest = apSurfStar[0];

    
    for( i=0; i < starDef.m_nStarLines; i++ )
    {
        m_pd3dDevice->SetTexture( i, m_apTexStar[i+4] );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

        avSampleWeights[i] = vWhite * 1.0f / (FLOAT) starDef.m_nStarLines;
    }

    CHAR strTechnique[256];
    _snprintf( strTechnique, 256, "MergeTextures_%d", starDef.m_nStarLines );
    strTechnique[255] = 0;

    m_pEffect->SetTechnique(strTechnique);

    m_pEffect->SetVectorArray("g_avSampleWeights", avSampleWeights, starDef.m_nStarLines);
    
    m_pd3dDevice->SetRenderTarget( 0, pSurfDest );
    
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;
    
    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);
    }
    
    m_pEffect->End();

    
    hr = S_OK;
LCleanReturn:
    for( i=0; i < NUM_STAR_TEXTURES; i++ )
    {
        SAFE_RELEASE( apSurfStar[i] );
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: RenderBloom()
// Desc: Render the blooming effect
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderBloom()
{
    HRESULT hr = S_OK;
    UINT uiPassCount, uiPass;
    int i=0;


    D3DXVECTOR2 avSampleOffsets[MAX_SAMPLES];
    FLOAT       afSampleOffsets[MAX_SAMPLES];
    D3DXVECTOR4 avSampleWeights[MAX_SAMPLES];

    PDIRECT3DSURFACE9 pSurfScaledHDR;
    m_pTexSceneScaled->GetSurfaceLevel(0, &pSurfScaledHDR);

    PDIRECT3DSURFACE9 pSurfBloom;
    m_apTexBloom[0]->GetSurfaceLevel(0, &pSurfBloom);

    PDIRECT3DSURFACE9 pSurfHDR;
    m_pTexScene->GetSurfaceLevel(0, &pSurfHDR);  
    
    PDIRECT3DSURFACE9 pSurfTempBloom;
    m_apTexBloom[1]->GetSurfaceLevel(0, &pSurfTempBloom);

    PDIRECT3DSURFACE9 pSurfBloomSource;
    m_apTexBloom[2]->GetSurfaceLevel(0, &pSurfBloomSource);

    // Clear the bloom texture
    m_pd3dDevice->ColorFill( pSurfBloom, NULL, D3DCOLOR_ARGB(0, 0, 0, 0) );

    if (m_GlareDef.m_fGlareLuminance <= 0.0f ||
        m_GlareDef.m_fBloomLuminance <= 0.0f)
    {
        hr = S_OK;
        goto LCleanReturn;
    }

    if( m_GlareDef.m_fGlareLuminance <= 0.0f ||
        m_GlareDef.m_fBloomLuminance <= 0.0f )
    {
        return S_OK ;
    }

    RECT rectSrc;
    GetTextureRect( m_pTexBloomSource, &rectSrc );
    InflateRect( &rectSrc, -1, -1 );

    RECT rectDest;
    GetTextureRect( m_apTexBloom[2], &rectDest );
    InflateRect( &rectDest, -1, -1 );

    CoordRect coords;
    GetTextureCoords( m_pTexBloomSource, &rectSrc, m_apTexBloom[2], &rectDest, &coords );
   
    D3DSURFACE_DESC desc;
    hr = m_pTexBloomSource->GetLevelDesc( 0, &desc );
    if( FAILED(hr) )
        return hr;


    m_pEffect->SetTechnique("GaussBlur5x5");

    hr = GetSampleOffsets_GaussBlur5x5( desc.Width, desc.Height, avSampleOffsets, avSampleWeights, 1.0f );

    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    m_pEffect->SetValue("g_avSampleWeights", avSampleWeights, sizeof(avSampleWeights));
   
    m_pd3dDevice->SetRenderTarget(0, pSurfBloomSource );
    m_pd3dDevice->SetTexture( 0, m_pTexBloomSource );
    m_pd3dDevice->SetScissorRect( &rectDest );
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
       
    
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;
    
    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad( coords );
    }
    m_pEffect->End();
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

    hr = m_apTexBloom[2]->GetLevelDesc( 0, &desc );
    if( FAILED(hr) )
        return hr;

    hr = GetSampleOffsets_Bloom( desc.Width, afSampleOffsets, avSampleWeights, 3.0f, 2.0f );
    for( i=0; i < MAX_SAMPLES; i++ )
    {
        avSampleOffsets[i] = D3DXVECTOR2( afSampleOffsets[i], 0.0f );
    }
     

    m_pEffect->SetTechnique("Bloom");
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    m_pEffect->SetValue("g_avSampleWeights", avSampleWeights, sizeof(avSampleWeights));
   
    m_pd3dDevice->SetRenderTarget(0, pSurfTempBloom);
    m_pd3dDevice->SetTexture( 0, m_apTexBloom[2] );
    m_pd3dDevice->SetScissorRect( &rectDest );
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, TRUE );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
       
    
    m_pEffect->Begin(&uiPassCount, 0);
    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad( coords );
    }
    m_pEffect->End();
    m_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
    

    hr = m_apTexBloom[1]->GetLevelDesc( 0, &desc );
    if( FAILED(hr) )
        return hr;

    hr = GetSampleOffsets_Bloom( desc.Height, afSampleOffsets, avSampleWeights, 3.0f, 2.0f );
    for( i=0; i < MAX_SAMPLES; i++ )
    {
        avSampleOffsets[i] = D3DXVECTOR2( 0.0f, afSampleOffsets[i] );
    }

    
    GetTextureRect( m_apTexBloom[1], &rectSrc );
    InflateRect( &rectSrc, -1, -1 );

    GetTextureCoords( m_apTexBloom[1], &rectSrc, m_apTexBloom[0], NULL, &coords );

    
    m_pEffect->SetTechnique("Bloom");
    m_pEffect->SetValue("g_avSampleOffsets", avSampleOffsets, sizeof(avSampleOffsets));
    m_pEffect->SetValue("g_avSampleWeights", avSampleWeights, sizeof(avSampleWeights));
    
    m_pd3dDevice->SetRenderTarget(0, pSurfBloom);
    m_pd3dDevice->SetTexture(0, m_apTexBloom[1]);
   m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
       
   
    hr = m_pEffect->Begin(&uiPassCount, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    for (uiPass = 0; uiPass < uiPassCount; uiPass++)
    {
        m_pEffect->Pass(uiPass);

        // Draw a fullscreen quad to sample the RT
        DrawFullScreenQuad( coords );
    }

    m_pEffect->End();
   
  
    hr = S_OK;

LCleanReturn:
    SAFE_RELEASE( pSurfBloomSource );
    SAFE_RELEASE( pSurfTempBloom );
    SAFE_RELEASE( pSurfBloom );
    SAFE_RELEASE( pSurfHDR );
    SAFE_RELEASE( pSurfScaledHDR );
    
    return hr;
}




//-----------------------------------------------------------------------------
// Name: RenderText()
// Desc: Render the statistics & help text
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderText()
{
    RECT rc;
    int iLineY = 0;

    if( NULL == m_pFont )
        return S_OK;

    if( m_bDetailedStats )
    {
        // Output statistics
        TCHAR strBuffer[256];
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
                        
        _sntprintf( strBuffer, 256, 
                    TEXT("Glare type (g): %s\n")
                    TEXT("Tone Mapping (t): %s\n")
                    TEXT("Blue Shift (f): %s\n")
                    TEXT("Middle Gray (y/h): %.2f\n")
                    TEXT("Light 0 (i/k): %d.0e%d\n")
                    TEXT("Light 1 (o/l): %d.0e%d\n"),
                    m_GlareDef.m_strGlareName,
                    m_bToneMap ? TEXT("On") : TEXT("Off"), 
                    m_bBlueShift ? TEXT("On") : TEXT("Off"),
                    m_fKeyValue, 
                    m_nLightMantissa[0],
                    m_nLightLogIntensity[0],
                    m_nLightMantissa[1],
                    m_nLightLogIntensity[1]);
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 6*15;
        m_pFont->DrawText( NULL, strBuffer, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );

        if( !m_bDrawHelp )
        {
            SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
            m_pFont->DrawText( NULL, _T("Press F1 for help"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
        }    
    }

    if( m_bDrawHelp )
    {
        iLineY = m_d3dsdBackBuffer.Height-15*6;
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        m_pFont->DrawText( NULL, _T("Controls:"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        SetRect( &rc, 20, iLineY, 0, 0 ); 
        m_pFont->DrawText( NULL, _T("Look: Left drag mouse\n")
                                        _T("Move: A,W,S,D or Arrow Keys\n")
                                        _T("Toggle tone mapping: t\n")
                                        _T("Toggle blue shift: f\n")
                                        _T("Adjust middle gray key: y,h\n"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
    
        SetRect( &rc, 250, iLineY, 0, 0 ); 
        m_pFont->DrawText( NULL, _T("Adjust light 0 intensity: i,k\n")                                      
                                        _T("Adjust light 1 intensity: o,l\n")                                      
                                        _T("Reset values: r\n")
                                        _T("Reset camera: Home\n")
                                        _T("Show detailed stats: F4\n"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
    
        SetRect( &rc, 500, iLineY, 0, 0 );
        m_pFont->DrawText( NULL, _T("Hide help: F1\n")
                                        _T("Change Device: F2\n")
                                        _T("View readme: F9\n")
                                        _T("Quit: ESC"),
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: LoadMesh()
// Desc: Load a mesh file and perform optimization 
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
// Name: InvalidateDeviceObjects()
// Desc: Invalidates device objects.  
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    int i=0;

    if( m_pFont )
        m_pFont->OnLostDevice();

    if( m_pEffect )
        m_pEffect->OnLostDevice();

    SAFE_RELEASE(m_pWorldMesh);
    SAFE_RELEASE(m_pmeshSphere);

    SAFE_RELEASE(m_pTexScene);
    SAFE_RELEASE(m_pTexSceneScaled);
    SAFE_RELEASE(m_pTexWall);
    SAFE_RELEASE(m_pTexFloor);
    SAFE_RELEASE(m_pTexCeiling);
    SAFE_RELEASE(m_pTexPainting);
    SAFE_RELEASE(m_pTexAdaptedLuminanceCur);
    SAFE_RELEASE(m_pTexAdaptedLuminanceLast);
    SAFE_RELEASE(m_pTexBrightPass);
    SAFE_RELEASE(m_pTexBloomSource);
    SAFE_RELEASE(m_pTexStarSource);
    
    for( i=0; i < NUM_TONEMAP_TEXTURES; i++)
    {
        SAFE_RELEASE(m_apTexToneMap[i]);
    }

    for( i=0; i < NUM_STAR_TEXTURES; i++ )
    {
        SAFE_RELEASE(m_apTexStar[i]);
    }

    for( i=0; i < NUM_BLOOM_TEXTURES; i++ )
    {
        SAFE_RELEASE(m_apTexBloom[i]);
    }

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
    SAFE_RELEASE( m_pEffect );

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
// Name: MsgProc()
// Desc: Message proc function
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );
    
    switch(uMsg)
    {
    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
            case IDM_TOGGLEHELP:
                m_bDrawHelp = !m_bDrawHelp;  
                break;

            case IDM_TOGGLESTATS:
                m_bDetailedStats = !m_bDetailedStats;
                break;
        }
        break;

    case WM_KEYDOWN:
        switch(toupper(LOWORD(wParam)))
        {
            case 'T':
                m_bToneMap = !m_bToneMap;
                break;

            case 'Y':
                m_fKeyValue += 0.01f;
                if( m_fKeyValue > 1.0f )
                    m_fKeyValue = 1.0f;
                break;

            case 'H':
                m_fKeyValue -= 0.01f;
                if( m_fKeyValue < 0.0f )
                    m_fKeyValue = 0.0f;
                break;

            case 'R':
                ResetOptions();
                break;

            case 'F':
                m_bBlueShift = !m_bBlueShift;
                break;

            case 'I':
                AdjustLight(0, true);
                break;

            case 'K':
                AdjustLight(0, false);
                break;

            case 'O':
                AdjustLight(1, true);
                break;

            case 'L':
                AdjustLight(1, false);
                break;

            case 'G':
                m_eGlareType = (EGLARELIBTYPE) (1 + (int)m_eGlareType);
                if( m_eGlareType == NUM_GLARELIBTYPES )
                    m_eGlareType = (EGLARELIBTYPE) 0;

                m_GlareDef.Initialize(m_eGlareType);
                break;
        }

        return TRUE; // Discard unused keystrokes
    }

    return CD3DApplication::MsgProc(hWnd, uMsg, wParam, lParam);
}




//-----------------------------------------------------------------------------
// Name: DrawFullScreenQuad
// Desc: Draw a properly aligned quad covering the entire render target
//-----------------------------------------------------------------------------
void CMyD3DApplication::DrawFullScreenQuad(float fLeftU, float fTopV, float fRightU, float fBottomV)
{
    D3DSURFACE_DESC dtdsdRT;
    PDIRECT3DSURFACE9 pSurfRT;

    // Acquire render target width and height
    m_pd3dDevice->GetRenderTarget(0, &pSurfRT);
    pSurfRT->GetDesc(&dtdsdRT);
    pSurfRT->Release();

    // Ensure that we're directly mapping texels to pixels by offset by 0.5
    // For more info see the doc page titled "Directly Mapping Texels to Pixels"
    FLOAT fWidth5 = (FLOAT)dtdsdRT.Width - 0.5f;
    FLOAT fHeight5 = (FLOAT)dtdsdRT.Height - 0.5f;

    // Draw the quad
    ScreenVertex svQuad[4];

    svQuad[0].p = D3DXVECTOR4(-0.5f, -0.5f, 0.5f, 1.0f);
    svQuad[0].t = D3DXVECTOR2(fLeftU, fTopV);

    svQuad[1].p = D3DXVECTOR4(fWidth5, -0.5f, 0.5f, 1.0f);
    svQuad[1].t = D3DXVECTOR2(fRightU, fTopV);

    svQuad[2].p = D3DXVECTOR4(-0.5f, fHeight5, 0.5f, 1.0f);
    svQuad[2].t = D3DXVECTOR2(fLeftU, fBottomV);

    svQuad[3].p = D3DXVECTOR4(fWidth5, fHeight5, 0.5f, 1.0f);
    svQuad[3].t = D3DXVECTOR2(fRightU, fBottomV);

    m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pd3dDevice->SetFVF(ScreenVertex::FVF);
    m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, svQuad, sizeof(ScreenVertex));
    m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}





//-----------------------------------------------------------------------------
// Name: ConfirmDevice
// Desc: Confirm proper hardware support
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice(D3DCAPS9* pdtdc, DWORD dwBehavior, D3DFORMAT dtdfDisplay, D3DFORMAT dtdfBackBuffer)
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

    if(pdtdc->PixelShaderVersion < D3DPS_VERSION(2,0))
    {
        return E_FAIL;
    }

    if(pdtdc->VertexShaderVersion < D3DVS_VERSION(2,0))
    {
        return E_FAIL;
    }

    // No fallback yet, so need to support D3DFMT_A16B16G16R16F render target
    if( FAILED( m_pD3D->CheckDeviceFormat( pdtdc->AdapterOrdinal, pdtdc->DeviceType,
                    dtdfDisplay, D3DUSAGE_RENDERTARGET, 
                    D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F) ) )
    {
        return E_FAIL;
    }

    // No fallback yet, so need to support D3DFMT_R16F render target
    if( FAILED( m_pD3D->CheckDeviceFormat( pdtdc->AdapterOrdinal, pdtdc->DeviceType,
                    dtdfDisplay, D3DUSAGE_RENDERTARGET, 
                    D3DRTYPE_TEXTURE, D3DFMT_R16F) ) )
    {
        return E_FAIL;
    }


    // Need to support post-pixel processing
    if( FAILED( m_pD3D->CheckDeviceFormat( pdtdc->AdapterOrdinal, pdtdc->DeviceType,
        dtdfDisplay, D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, 
        D3DRTYPE_SURFACE, dtdfBackBuffer ) ) )
    {
        return E_FAIL;
    }


    return S_OK;
}



