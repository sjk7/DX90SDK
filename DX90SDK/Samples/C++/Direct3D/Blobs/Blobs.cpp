//-----------------------------------------------------------------------------
// File: Blobs.cpp
//
// Desc: A pixel shader effect to mimic metaball physics in image space.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define NUM_BLOBS           5
#define FIELD_OF_VIEW       ( (70.0f/90.0f)*(D3DX_PI/2.0f) )

#define GAUSSIAN_TEXSIZE    64
#define GAUSSIAN_HEIGHT     1
#define GAUSSIAN_DEVIATION  0.125f


// Vertex format for blob billboards
struct POINTVERTEX  
{
    D3DXVECTOR3 pos;
    float       size;
    D3DXVECTOR3 color;

    static const DWORD FVF;
};
const DWORD POINTVERTEX::FVF = D3DFVF_XYZ | D3DFVF_PSIZE | D3DFVF_DIFFUSE; 


// Vertex format for screen space work
struct SCREENVERTEX     
{
    D3DXVECTOR4 pos;
    D3DXVECTOR2 t0;
    D3DXVECTOR2 t1;
    D3DXVECTOR2 t2;
    D3DXVECTOR3 t3;

    static const DWORD FVF;
};
const DWORD SCREENVERTEX::FVF = D3DFVF_XYZRHW | D3DFVF_TEX4
                                | D3DFVF_TEXCOORDSIZE2(0)
                                | D3DFVF_TEXCOORDSIZE2(1)
                                | D3DFVF_TEXCOORDSIZE2(2)
                                | D3DFVF_TEXCOORDSIZE3(3);




//-----------------------------------------------------------------------------
// Name: BlobSortCB()
// Desc: Callback function for sorting blobs in ascending z order
//-----------------------------------------------------------------------------
int __cdecl BlobSortCB( const VOID* a, const VOID* b )
{
    if ( ((const POINTVERTEX*)a)->pos.z < ((const POINTVERTEX*)b)->pos.z )
        return -1;
    if ( ((const POINTVERTEX*)a)->pos.z > ((const POINTVERTEX*)b)->pos.z )
        return 1;
    return 0;
}




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
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FrameMove();
    HRESULT Render();
    HRESULT FinalCleanup();
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

    HRESULT ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior, 
                           D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat );

    HRESULT RenderFullScreenQuad( FLOAT fDepth );
    HRESULT FillBlobVB();
    void    RenderText();
    HRESULT GenerateGaussianTexture();

private:
    LPDIRECT3DVERTEXBUFFER9 m_pBlobVB;          // Vertex buffer for blob billboards
    
    POINTVERTEX   m_BlobPoints[NUM_BLOBS];      // Position, size, and color states
    
    LPD3DXFONT    m_pFont;                      // Font for drawing text
    
    D3DXMATRIXA16 m_matWorld;                   // World matrix
    D3DXMATRIXA16 m_matView;                    // View matrix
    D3DXMATRIXA16 m_matProjection;              // Projection matrix
    D3DXMATRIXA16 m_matWorldView;               // World-view concatenation 
    
    LPDIRECT3DTEXTURE9      m_pTexGBuffer[4];   // Buffer textures for blending effect   
    LPDIRECT3DTEXTURE9      m_pTexScratch;      // Scratch texture
    LPDIRECT3DTEXTURE9      m_pTexBlob;         // Blob texture

    LPDIRECT3DCUBETEXTURE9  m_pEnvMap;          // Environment map   
    CModelViewerCamera      m_Camera;           // Arcball camera    
    LPD3DXEFFECT            m_pEffect;          // Effect
    float                   m_fAnimationTime;   // Animation clock

    BOOL                    m_bShowHelp;        // Toggle on-screen help

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
    m_strWindowTitle = TEXT( "Blobs" );
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;
    m_pFont = NULL;
    m_bShowHelp = true;
    m_pEffect = NULL;
    m_pTexScratch = NULL;
    m_pTexBlob = NULL;
    m_pEnvMap = NULL;
    m_fAnimationTime = 0.0f;
    m_pEffect = NULL;
    m_pBlobVB = NULL;

    ZeroMemory( m_pTexGBuffer, sizeof(m_pTexGBuffer) );
    ZeroMemory( m_BlobPoints, sizeof(m_BlobPoints) );
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: One-time initialization of non-device objects
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    // Set initial blob states
    for( int i=0; i < NUM_BLOBS; i++ )
    {
        m_BlobPoints[i].pos = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
        m_BlobPoints[i].size = 1.0f;
    }

    m_BlobPoints[0].color = D3DXVECTOR3(0.3f, 0.0f, 0.0f);
    m_BlobPoints[1].color = D3DXVECTOR3(0.0f, 0.3f, 0.0f);
    m_BlobPoints[2].color = D3DXVECTOR3(0.0f, 0.0f, 0.3f);
    m_BlobPoints[3].color = D3DXVECTOR3(0.3f, 0.3f, 0.0f);
    m_BlobPoints[4].color = D3DXVECTOR3(0.0f, 0.3f, 0.3f);

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    HRESULT hr;

    HDC hDC = GetDC( NULL );
    int nHeight = -MulDiv( 9, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    ReleaseDC( NULL, hDC );
    
    // Create a font for statistics and help output
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

    // Create the effect
    LPD3DXBUFFER pBufferErrors = NULL;
    TCHAR str[MAX_PATH];
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("Blobs.fx") );
    hr = D3DXCreateEffectFromFile(m_pd3dDevice, str, NULL, NULL, dwShaderFlags, NULL, &m_pEffect, &pBufferErrors );
    if( FAILED( hr ) )
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
    UINT i;
    
    if( m_pFont )
        m_pFont->OnResetDevice();
    if( m_pEffect )
        m_pEffect->OnResetDevice();

    if( FAILED( hr = GenerateGaussianTexture() ) )
        return hr;
   
    // Create the blob vertex buffer
    hr = m_pd3dDevice->CreateVertexBuffer( NUM_BLOBS * 6 * sizeof(SCREENVERTEX), 
                                           D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
                                           SCREENVERTEX::FVF, D3DPOOL_DEFAULT,
                                           &m_pBlobVB, NULL );
    if( FAILED(hr) )
        return hr;

    // Create buffer textures 
    for( i=0; i < 4; ++i )
    {
        hr = m_pd3dDevice->CreateTexture( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 1,
                                          D3DUSAGE_RENDERTARGET,
                                          D3DFMT_A16B16G16R16F,
                                          D3DPOOL_DEFAULT, &m_pTexGBuffer[i], NULL );
        if( FAILED(hr) )
            return hr;
    }


    // Create the blank texture
    hr = m_pd3dDevice->CreateTexture( 1, 1, 1,
                                      D3DUSAGE_RENDERTARGET,
                                      D3DFMT_A16B16G16R16F,
                                      D3DPOOL_DEFAULT,
                                      &m_pTexScratch, NULL );

    if( FAILED(hr) )
        return hr;

    // Set the camera parameters
    D3DXVECTOR3 vEyePt    = D3DXVECTOR3( 0.0f, -2.0f, -5.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    m_Camera.SetViewParams( &vEyePt, &vLookatPt );
    m_Camera.SetProjParams( FIELD_OF_VIEW, (FLOAT) m_d3dsdBackBuffer.Width / m_d3dsdBackBuffer.Height, 1.0f, 100.0f );
    m_Camera.SetRadius( 5.0f, 2.0f, 20.0f  );
    m_Camera.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );
    m_Camera.SetInvertPitch( true );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GenerateGaussianTexture()
// Desc: Generate a texture to store gaussian distribution results
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::GenerateGaussianTexture()
{
    HRESULT hr = S_OK;
    LPDIRECT3DSURFACE9 pBlobTemp = NULL;
    LPDIRECT3DSURFACE9 pBlobNew = NULL;
    
    
    
    // Create a temporary texture
    LPDIRECT3DTEXTURE9 texTemp;
    hr = m_pd3dDevice->CreateTexture( GAUSSIAN_TEXSIZE, GAUSSIAN_TEXSIZE, 1,
                                      D3DUSAGE_DYNAMIC, D3DFMT_R32F,
                                      D3DPOOL_DEFAULT, &texTemp, NULL );
    if( FAILED(hr) )
        goto LCleanReturn;

    // Create the gaussian texture
    hr = m_pd3dDevice->CreateTexture( GAUSSIAN_TEXSIZE, GAUSSIAN_TEXSIZE, 1,
                                      D3DUSAGE_DYNAMIC, D3DFMT_R16F,
                                      D3DPOOL_DEFAULT, &m_pTexBlob, NULL );
    if( FAILED(hr) )
        goto LCleanReturn;


    // Create the environment map
    TCHAR str[MAX_PATH];
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("lobbycube.dds") );
    hr = D3DXCreateCubeTextureFromFile( m_pd3dDevice, str, &m_pEnvMap );
    if( FAILED(hr) )
        goto LCleanReturn;


    // Fill in the gaussian texture data
    D3DLOCKED_RECT Rect;
    hr = texTemp->LockRect(0, &Rect, 0, 0);
    if( FAILED(hr) )
        goto LCleanReturn;

    int u, v;
    float dx, dy, I;
    float* pBits;  
    
    for( v=0; v < GAUSSIAN_TEXSIZE; ++v )
    {
        pBits = (float*)((char*)(Rect.pBits)+v*Rect.Pitch);
        
        for( u=0; u < GAUSSIAN_TEXSIZE; ++u )
        {
            dx = 2.0f*u/(float)GAUSSIAN_TEXSIZE-1.0f;
            dy = 2.0f*v/(float)GAUSSIAN_TEXSIZE-1.0f;
            I = GAUSSIAN_HEIGHT * (float)exp(-(dx*dx+dy*dy)/GAUSSIAN_DEVIATION);

            *(pBits++) = I;  // intensity
        }
    }

    texTemp->UnlockRect(0);

    // Copy the temporary surface to the stored gaussian texture
    texTemp->GetSurfaceLevel( 0, &pBlobTemp );
    m_pTexBlob->GetSurfaceLevel( 0, &pBlobNew );
    
    hr = D3DXLoadSurfaceFromSurface( pBlobNew, 0, 0, pBlobTemp, 0, 0, D3DX_FILTER_NONE, 0 );
    if( FAILED(hr) )
        goto LCleanReturn;


    hr = S_OK;

LCleanReturn:
    SAFE_RELEASE(pBlobTemp);
    SAFE_RELEASE(pBlobNew);
    SAFE_RELEASE(texTemp);

    return hr;
}




//-----------------------------------------------------------------------------
// Name: FillBlobVB()
// Desc: Fill the vertex buffer for the blob objects
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FillBlobVB()
{
    HRESULT hr = S_OK;
    UINT i=0; // Loop variable
    
    SCREENVERTEX* pBlobVertex;
    if( FAILED(hr = m_pBlobVB->Lock(0,0,(void**)&pBlobVertex,D3DLOCK_DISCARD)) )
        return hr;
    
    POINTVERTEX blobPos[NUM_BLOBS];
    
    for( i=0; i < NUM_BLOBS; ++i )
    {
        //transform point to camera space
        D3DXVECTOR4 blobPosCamera;
        D3DXVec3Transform(&blobPosCamera, &m_BlobPoints[i].pos, &m_matWorldView);
        
        blobPos[i] = m_BlobPoints[i];
        blobPos[i].pos.x = blobPosCamera.x;
        blobPos[i].pos.y = blobPosCamera.y;
        blobPos[i].pos.z = blobPosCamera.z;
    }

    // Sort by ascending z
    qsort( blobPos, NUM_BLOBS, sizeof(POINTVERTEX), BlobSortCB );

    int posCount=0;
    for( i=0; i < NUM_BLOBS; ++i )
    {
        D3DXVECTOR4 blobScreenPos;

        // For calculating billboarding
        D3DXVECTOR4 billOfs(blobPos[i].size,blobPos[i].size,blobPos[i].pos.z,1);
        D3DXVECTOR4 billOfsScreen;

        // Transform to screenspace
        D3DXVec3Transform(&blobScreenPos, &blobPos[i].pos, &m_matProjection);
        D3DXVec4Transform(&billOfsScreen, &billOfs, &m_matProjection);

        // Project
        float zOfs = blobScreenPos.z;
        D3DXVec4Scale(&blobScreenPos,&blobScreenPos,1.0f/blobScreenPos.w);
        D3DXVec4Scale(&billOfsScreen,&billOfsScreen,1.0f/billOfsScreen.w);


        D3DXVECTOR2 vTexCoords[] = 
        {
            D3DXVECTOR2(0.0f,0.0f),
            D3DXVECTOR2(0.0f,1.0f),
            D3DXVECTOR2(1.0f,1.0f),
            D3DXVECTOR2(1.0f,1.0f),
            D3DXVECTOR2(1.0f,0.0f),
            D3DXVECTOR2(0.0f,0.0f),
        };

        D3DXVECTOR4 vPosOffset[] =
        {
            D3DXVECTOR4(-billOfsScreen.x,-billOfsScreen.y,0.0f,0.0f),
            D3DXVECTOR4(-billOfsScreen.x, billOfsScreen.y,0.0f,0.0f),
            D3DXVECTOR4( billOfsScreen.x, billOfsScreen.y,0.0f,0.0f),
            D3DXVECTOR4( billOfsScreen.x, billOfsScreen.y,0.0f,0.0f),
            D3DXVECTOR4( billOfsScreen.x,-billOfsScreen.y,0.0f,0.0f),
            D3DXVECTOR4(-billOfsScreen.x,-billOfsScreen.y,0.0f,0.0f),
        };

        // Set constants across quad
        for( int j=0; j < 6 ;++j )
        {
            // Scale to pixels
            D3DXVec4Add( &pBlobVertex[posCount].pos, &blobScreenPos, &vPosOffset[j] );  
            
            pBlobVertex[posCount].pos.x *= m_d3dsdBackBuffer.Width;             
            pBlobVertex[posCount].pos.y *= m_d3dsdBackBuffer.Height;
            pBlobVertex[posCount].pos.x += 0.5f * m_d3dsdBackBuffer.Width; 
            pBlobVertex[posCount].pos.y += 0.5f * m_d3dsdBackBuffer.Height;
            
            pBlobVertex[posCount].t0 = vTexCoords[j];
            pBlobVertex[posCount].t1 = D3DXVECTOR2((0.5f+pBlobVertex[posCount].pos.x)*(1.0f/m_d3dsdBackBuffer.Width),
                                                   (0.5f+pBlobVertex[posCount].pos.y)*(1.0f/m_d3dsdBackBuffer.Height));
            pBlobVertex[posCount].t2.x = zOfs; 
            pBlobVertex[posCount].t2.y = blobPos[i].size;
            pBlobVertex[posCount].t3 = blobPos[i].color;

            posCount++;
        }
    }
    m_pBlobVB->Unlock();

    return hr;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Invalidates device objects.  
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pBlobVB );
    SAFE_RELEASE( m_pTexScratch );
    SAFE_RELEASE( m_pTexBlob );
    SAFE_RELEASE( m_pEnvMap );
    
    for( UINT i=0; i < 4; i++ )
    {
        SAFE_RELEASE( m_pTexGBuffer[i] );
    }
    
    if( m_pFont )
        m_pFont->OnLostDevice();
    if( m_pEffect )
        m_pEffect->OnLostDevice();

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
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    // Pass mouse messages to the ArcBall so it can build internal matrices
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
            
            break;
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
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

    // Get world/view/projection matrices from the camera class
    m_matWorld      = *m_Camera.GetWorldMatrix();
    m_matView       = *m_Camera.GetViewMatrix();
    m_matProjection = *m_Camera.GetProjMatrix();

    // Calc matricies
    D3DXMATRIXA16 matWorldViewProjection;
    m_matWorldView = m_matWorld * m_matView;
    matWorldViewProjection = m_matWorldView * m_matProjection;

    // Pass the composed transformation matrix to the effect
    m_pEffect->SetMatrix( "g_mWorldViewProjection", &matWorldViewProjection );
    
    // Pause animatation if the user is rotating around
    float fAnimationDelta = m_fElapsedTime;
    if( m_Camera.IsBeingDragged() )
        fAnimationDelta = 0.0f;
    m_fAnimationTime += fAnimationDelta;

    // Animation the blobs
    float pos = (float) ( 1.0f + cos( 2 * D3DX_PI * m_fAnimationTime/3.0f ) );
    m_BlobPoints[1].pos.x = pos;
    m_BlobPoints[2].pos.x = -pos;
    m_BlobPoints[3].pos.y = pos;
    m_BlobPoints[4].pos.y = -pos;

    FillBlobVB();

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
    UINT i;
    
    LPDIRECT3DSURFACE9 pSurfOldRenderTarget = NULL;
    LPDIRECT3DSURFACE9 pSurfOldDepthStencil = NULL;
    LPDIRECT3DSURFACE9 pGBufSurf[4];

    
    // Clear the viewport
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 255), 1.0f, 0L );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Get the initial device surfaces
        m_pd3dDevice->GetRenderTarget( 0, &pSurfOldRenderTarget );
        m_pd3dDevice->GetDepthStencilSurface( &pSurfOldDepthStencil );

        // Turn off Z
        m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
        m_pd3dDevice->SetDepthStencilSurface( NULL );

        
        // Clear the blank texture
        LPDIRECT3DSURFACE9 pSurfBlank;
        m_pTexScratch->GetSurfaceLevel( 0, &pSurfBlank );
        m_pd3dDevice->SetRenderTarget( 0, pSurfBlank );
        m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET , D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0 );
        SAFE_RELEASE( pSurfBlank );

        // clear temp textures
        for( i=0; i < 4; ++i )
        {
            m_pTexGBuffer[i]->GetSurfaceLevel( 0, &pGBufSurf[i] );
            m_pd3dDevice->SetRenderTarget( 0, pGBufSurf[i] );
            m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET , D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0 );
        }

        m_pd3dDevice->SetStreamSource( 0, m_pBlobVB, 0, sizeof(SCREENVERTEX) ); 
        m_pd3dDevice->SetFVF( SCREENVERTEX::FVF );
        
        
        // Render the blobs
        UINT iPass, nNumPasses;
    
        m_pEffect->SetTechnique( "BlobBlend" );
        
        m_pEffect->Begin( &nNumPasses, 0 );

        for( iPass=0; iPass < nNumPasses; iPass++ )
        {
            m_pEffect->Pass( iPass );

            for( i=0; i < NUM_BLOBS; ++i )
            {
                // Copy bits off of render target into scratch surface [for blending].
                m_pEffect->SetTexture( "g_tSourceBlob", m_pTexScratch );
                m_pEffect->SetTexture( "g_tSurfaceBuffer", m_pTexGBuffer[0] );
                m_pEffect->SetTexture( "g_tMatrixBuffer", m_pTexGBuffer[1] );
                m_pd3dDevice->SetRenderTarget( 0, pGBufSurf[2] );
                m_pd3dDevice->SetRenderTarget( 1, pGBufSurf[3] );
                m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, i*6, 2 );

                // Render the blob
                m_pEffect->SetTexture( "g_tSourceBlob", m_pTexBlob );
                m_pEffect->SetTexture( "g_tSurfaceBuffer", m_pTexGBuffer[2] );
                m_pEffect->SetTexture( "g_tMatrixBuffer", m_pTexGBuffer[3] );
                m_pd3dDevice->SetRenderTarget( 0, pGBufSurf[0] );
                m_pd3dDevice->SetRenderTarget( 1, pGBufSurf[1] );
                m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLELIST, i*6, 2 );
            }
        }

        m_pEffect->End();


        // Restore initial device surfaces
        m_pd3dDevice->SetDepthStencilSurface( pSurfOldDepthStencil );
        m_pd3dDevice->SetRenderTarget( 0, pSurfOldRenderTarget );
        m_pd3dDevice->SetRenderTarget( 1, NULL );

        // Light and composite blobs into backbuffer
        m_pEffect->SetTechnique( "BlobLight" );
        
        m_pEffect->Begin( &nNumPasses, 0 );

        for( iPass=0; iPass < nNumPasses; iPass++ )
        {
            m_pEffect->Pass( iPass );

            for( i=0; i < NUM_BLOBS; ++i )
            {
                m_pEffect->SetTexture( "g_tSourceBlob", m_pTexGBuffer[0] );
                m_pEffect->SetTexture( "g_tSurfaceBuffer", m_pTexGBuffer[1] );
                m_pEffect->SetTexture( "g_tMatrixBuffer", m_pTexGBuffer[1] );
                m_pEffect->SetTexture( "g_tEnvMap", m_pEnvMap );

                RenderFullScreenQuad( 1.0f );
            }
        }

        m_pEffect->End();

        // Render statistics
        RenderText();

        // End the scene
        m_pd3dDevice->EndScene();
    }

    SAFE_RELEASE( pSurfOldRenderTarget );
    SAFE_RELEASE( pSurfOldDepthStencil );

    for( i=0; i < 4; ++i )
    {
        SAFE_RELEASE( pGBufSurf[i] );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderFullScreenQuad()
// Desc: Render a quad at the specified tranformed depth 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderFullScreenQuad(float fDepth)
{
    SCREENVERTEX aVertices[4];

    aVertices[0].pos = D3DXVECTOR4( -0.5f, -0.5f, fDepth, fDepth );
    aVertices[1].pos = D3DXVECTOR4( -0.5f, m_d3dsdBackBuffer.Height - 0.5f, fDepth, fDepth );
    aVertices[2].pos = D3DXVECTOR4( m_d3dsdBackBuffer.Width - 0.5f, -0.5f, fDepth, fDepth );
    aVertices[3].pos = D3DXVECTOR4( m_d3dsdBackBuffer.Width - 0.5f, m_d3dsdBackBuffer.Height - 0.5f, fDepth, fDepth );
    
    
    aVertices[0].t0 = D3DXVECTOR2( 0.0f, 0.0f );
    aVertices[1].t0 = D3DXVECTOR2( 0.0f, 1.0f );
    aVertices[2].t0 = D3DXVECTOR2( 1.0f, 0.0f );
    aVertices[3].t0 = D3DXVECTOR2( 1.0f, 1.0f );

    for (int i=0;i<4;++i)
    {
        aVertices[i].t1 = aVertices[i].t2 = aVertices[i].t0;
        aVertices[i].t1.x += (1.0f/m_d3dsdBackBuffer.Width);
        aVertices[i].t1.y += (1.0f/m_d3dsdBackBuffer.Height);
    }


    m_pd3dDevice->SetFVF(SCREENVERTEX::FVF);
    m_pd3dDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,aVertices,sizeof(SCREENVERTEX));
    

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderText()
// Desc: Render the device statistics and help information
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

    // Draw help
    if( m_bShowHelp )
    {
        iLineY = m_d3dsdBackBuffer.Height-15*6;
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        m_pFont->DrawText( NULL, _T("Controls:"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );

        SetRect( &rc, 20, iLineY, 0, 0 ); 
        m_pFont->DrawText( NULL, _T("Rotate model: Left mouse button\n")
                                      _T("Rotate camera: Right mouse button\n")
                                      _T("Zoom camera: Mouse wheel scroll\n")
                                      _T("Hide help: F1\n"),                                      
                                      -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );

        SetRect( &rc, 250, iLineY, 0, 0 );
        m_pFont->DrawText( NULL, _T("Change Device: F2\n")
                                      _T("View readme: F9\n")
                                      _T("Quit: ESC"),
                                      -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
    }
    else
    {
        SetRect( &rc, 2, 30, 0, 0 );
        m_pFont->DrawText( NULL, _T("Press F1 for help."), 
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
    if( dwBehavior & D3DCREATE_PUREDEVICE )
        return E_FAIL;

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

    return S_OK;
}
