//-----------------------------------------------------------------------------
// File: Text3D.cpp
//
// Desc: Example code showing how to do text in a Direct3D scene.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include <commdlg.h>
#include "resource.h"


//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
protected:
    LPD3DXFONT    m_pFont;
    LPD3DXFONT    m_pFont2;
    LPD3DXFONT    m_pStatsFont;
    LPD3DXMESH    m_pMesh3DText;
    WCHAR*        m_strTextBuffer;

    ID3DXSprite*  m_pD3DXSprite;

    CFirstPersonCamera m_Camera;

    TCHAR         m_strFont[LF_FACESIZE];
    int           m_nFontSize;

    D3DXMATRIXA16    m_matObj1;
    D3DXMATRIXA16    m_matObj2;

protected:
    HRESULT OneTimeSceneInit();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT FinalCleanup();
    HRESULT CreateD3DXTextMesh( LPD3DXMESH* ppMesh, TCHAR* pstrFont, DWORD dwSize, BOOL bBold, BOOL bItalic );
    HRESULT CreateD3DXFont( LPD3DXFONT* ppd3dxFont, TCHAR* pstrFont, DWORD dwSize );

public:
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
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
    m_strWindowTitle    = _T("Text3D: Text in a 3D scene");
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
    m_pMesh3DText       = NULL;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;

    lstrcpy( m_strFont, _T("Arial") );
    m_nFontSize  = 12;
    m_pFont       = NULL;
    m_pFont2      = NULL;
    m_pStatsFont  = NULL;
    m_pD3DXSprite = NULL;
    m_strTextBuffer = NULL;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    HRSRC rsrc;
    HGLOBAL hgData;
    LPVOID pvData;
    DWORD cbData;

    rsrc = FindResource( NULL, MAKEINTRESOURCE(IDR_TXT), _T("TEXT") );
    if( rsrc != NULL )
    {
        cbData = SizeofResource( NULL, rsrc );
        if( cbData > 0 )
        {
            hgData = LoadResource( NULL, rsrc );
            if( hgData != NULL )
            {
                pvData = LockResource( hgData );
                if( pvData != NULL )
                {
                    int nSize = cbData/sizeof(WCHAR) + 1;
                    m_strTextBuffer = new WCHAR[nSize];
                    memcpy( m_strTextBuffer, (WCHAR*)pvData, cbData );
                    m_strTextBuffer[nSize-1] = 0;
                }
            }
        }
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
    int nHeight;

    HDC hDC = GetDC( NULL );
    int nLogPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
    ReleaseDC( NULL, hDC );

    nHeight = -m_nFontSize * nLogPixelsY / 72;
    hr = D3DXCreateFont( m_pd3dDevice,          // D3D device
                         nHeight,               // Height
                         0,                     // Width
                         FW_BOLD,               // Weight
                         0,                     // MipLevels, 0 = autogen mipmaps
                         FALSE,                 // Italic
                         DEFAULT_CHARSET,       // CharSet
                         OUT_DEFAULT_PRECIS,    // OutputPrecision
                         DEFAULT_QUALITY,       // Quality
                         DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
                         m_strFont,              // pFaceName
                         &m_pFont);              // ppFont

    nHeight = -9 * nLogPixelsY / 72;
    if( FAILED( hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("System"), &m_pFont2 ) ) )
        return hr;

    nHeight = -9 * nLogPixelsY / 72;
    if( FAILED( hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("Arial"), &m_pStatsFont ) ) )
        return hr;

    if( FAILED( hr = D3DXCreateSprite( m_pd3dDevice, &m_pD3DXSprite ) ) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CreateD3DXFont()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::CreateD3DXFont( LPD3DXFONT* ppd3dxFont, 
                                           TCHAR* pstrFont, DWORD dwSize )
{
    HRESULT hr;
    LPD3DXFONT pd3dxFontNew = NULL;
    HDC hDC;
    INT nHeight;

    hDC = GetDC( NULL );
    nHeight = -MulDiv( dwSize, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    ReleaseDC( NULL, hDC );

    hr = D3DXCreateFont( m_pd3dDevice,          // D3D device
                         nHeight,               // Height
                         0,                     // Width
                         FW_BOLD,               // Weight
                         0,                     // MipLevels
                         FALSE,                 // Italic
                         DEFAULT_CHARSET,       // CharSet
                         OUT_DEFAULT_PRECIS,    // OutputPrecision
                         DEFAULT_QUALITY,       // Quality
                         DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
                         pstrFont,              // pFaceName
                         &pd3dxFontNew);        // ppFont

    if( SUCCEEDED( hr ) )
        *ppd3dxFont = pd3dxFontNew;

    return hr;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    // Restore the fonts
    m_pStatsFont->OnResetDevice();
    m_pFont->OnResetDevice();
    m_pFont2->OnResetDevice();
    m_pD3DXSprite->OnResetDevice();

    // Restore the textures
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
    m_pd3dDevice->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );

    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_DITHERENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x80808080);
    D3DLIGHT9 light;
    D3DXVECTOR3 vecLightDirUnnormalized(10.0f, -10.0f, 10.0f);
    ZeroMemory( &light, sizeof(D3DLIGHT9) );
    light.Type        = D3DLIGHT_DIRECTIONAL;
    light.Diffuse.r   = 1.0f;
    light.Diffuse.g   = 1.0f;
    light.Diffuse.b   = 1.0f;
    D3DXVec3Normalize( (D3DXVECTOR3*)&light.Direction, &vecLightDirUnnormalized );
    light.Position.x   = 10.0f;
    light.Position.y   = -10.0f;
    light.Position.z   = 10.0f;
    light.Range        = 1000.0f;
    m_pd3dDevice->SetLight(0, &light );
    m_pd3dDevice->LightEnable(0, TRUE);

    // Set the transform matrices
    D3DXMATRIXA16  matWorld;
    D3DXMatrixIdentity( &matWorld );
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Setup the camera with view & projection matrix
    D3DXVECTOR3 vecEye(0.0f,-5.0f, -10.0f);
    D3DXVECTOR3 vecAt (0.2f, 0.0f, 0.0f);
    m_Camera.SetViewParams( &vecEye, &vecAt );
    float fAspectRatio = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 1.0f, 1000.0f );

    SAFE_RELEASE( m_pMesh3DText );
    if( FAILED( CreateD3DXTextMesh( &m_pMesh3DText, m_strFont, m_nFontSize, FALSE, FALSE ) ) )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CreateD3DXTextMesh()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::CreateD3DXTextMesh( LPD3DXMESH* ppMesh, 
                                               TCHAR* pstrFont, DWORD dwSize,
                                               BOOL bBold, BOOL bItalic )
{
    HRESULT hr;
    LPD3DXMESH pMeshNew = NULL;
    HDC hdc = CreateCompatibleDC( NULL );
    INT nHeight = -MulDiv( dwSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
    HFONT hFont;
    HFONT hFontOld;

    hFont = CreateFont(nHeight, 0, 0, 0, bBold ? FW_BOLD : FW_NORMAL, bItalic, FALSE, FALSE, DEFAULT_CHARSET, 
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, pstrFont);
    
    hFontOld = (HFONT)SelectObject(hdc, hFont); 

    hr = D3DXCreateText(m_pd3dDevice, hdc, _T("This is calling D3DXCreateText"), 
                        0.001f, 0.4f, &pMeshNew, NULL, NULL);

    SelectObject(hdc, hFontOld);
    DeleteObject( hFont );
    DeleteDC( hdc );

    if( SUCCEEDED( hr ) )
        *ppMesh = pMeshNew;

    return hr;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // Setup five rotation matrices (for rotating text strings)
    D3DXVECTOR3 vAxis1(1.0f,2.0f,0.0f);
    D3DXVECTOR3 vAxis2(1.0f,0.0f,0.0f);
    D3DXMatrixRotationAxis( &m_matObj1, &vAxis1, m_fTime/2.0f  );
    D3DXMatrixRotationAxis( &m_matObj2, &vAxis2, m_fTime );

    D3DXMATRIX mScale;
    D3DXMatrixScaling( &mScale, 0.5f, 0.5f, 0.5f );
    m_matObj2 *= mScale;

    // Add some translational values to the matrices
    m_matObj1._41 = 1.0f;   m_matObj1._42 = 6.0f;   m_matObj1._43 = 20.0f; 
    m_matObj2._41 = -4.0f;  m_matObj2._42 = -1.0f;  m_matObj2._43 = 0.0f; 

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
    RECT rc;
    D3DMATERIAL9 mtrl;
    D3DXMATRIXA16 matWorld;

    // Get the view & projection matrix from camera.
    // User can't control camera for this simple sample
    D3DXMATRIXA16 matView = *m_Camera.GetViewMatrix();
    D3DXMATRIXA16 matProj = *m_Camera.GetProjMatrix();
    D3DXMATRIXA16 matViewProj = matView * matProj;

    m_pd3dDevice->SetTransform( D3DTS_VIEW,       &matView );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    // Clear the viewport
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0L );

    // Begin the scene 
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // Demonstration 1:
        // Draw a simple line using ID3DXFont::DrawText
#if 1 
        // Pass in DT_NOCLIP so we don't have to calc the bottom/right of the rect
        SetRect( &rc, 150, 200, 0, 0 );        
        m_pFont->DrawText( NULL, _T("This is a trival call to ID3DXFont::DrawText"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ));
#else
        // If you wanted to calc the bottom/rect of the rect make these 2 calls
        SetRect( &rc, 150, 200, 0, 0 );        
        m_pFont->DrawText( NULL, _T("This is a trival call to ID3DXFont::DrawText"), -1, &rc, DT_CALCRECT, D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ));
        m_pFont->DrawText( NULL, _T("This is a trival call to ID3DXFont::DrawText"), -1, &rc, 0, D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ));
#endif

        // Demonstration 2:
        // Allow multiple draw calls to sort by texture changes by ID3DXSprite
        // When drawing 2D text use flags: D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE
        // When drawing 3D text use flags: D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_BACKTOFRONT
        m_pD3DXSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );
        SetRect( &rc, 10, m_d3dpp.BackBufferHeight - 15*6, 0, 0 );        
        m_pFont2->DrawText( m_pD3DXSprite, _T("These multiple calls to ID3DXFont::DrawText() using the same ID3DXSprite."), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
        SetRect( &rc, 10, m_d3dpp.BackBufferHeight - 15*5, 0, 0 );        
        m_pFont2->DrawText( m_pD3DXSprite, _T("ID3DXFont now caches letters on one or more textures."), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
        SetRect( &rc, 10, m_d3dpp.BackBufferHeight - 15*4, 0, 0 );        
        m_pFont2->DrawText( m_pD3DXSprite, _T("In order to sort by texture state changes on muliple"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
        SetRect( &rc, 10, m_d3dpp.BackBufferHeight - 15*3, 0, 0 );        
        m_pFont2->DrawText( m_pD3DXSprite, _T("draw calls to DrawText() pass a ID3DXSprite and use"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
        SetRect( &rc, 10, m_d3dpp.BackBufferHeight - 15*2, 0, 0 );        
        m_pFont2->DrawText( m_pD3DXSprite, _T("flags D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
        m_pD3DXSprite->End();

        // Demonstration 3:
        // Word wrapping and unicode text.  
        // Note that not all fonts support dynamic font linking. 
        rc.left = 10;
        rc.top = 60;
        rc.right = m_d3dpp.BackBufferWidth - 10;
        rc.bottom = m_d3dpp.BackBufferHeight - 10;
        m_pFont2->DrawTextW( NULL, m_strTextBuffer, -1, &rc, DT_LEFT | DT_WORDBREAK | DT_EXPANDTABS, D3DXCOLOR( 1.0f, 0.0f, 1.0f, 1.0f ));

        // Demonstration 4:
        // Draw 3D extruded text using a mesh created with D3DXFont
        if( m_pMesh3DText != NULL )
        {
            ZeroMemory( &mtrl, sizeof(D3DMATERIAL9) );
            mtrl.Diffuse.r = mtrl.Ambient.r = 0.0f;
            mtrl.Diffuse.g = mtrl.Ambient.g = 0.0f;
            mtrl.Diffuse.b = mtrl.Ambient.b = 1.0f;
            mtrl.Diffuse.a = mtrl.Ambient.a = 1.0f;
            m_pd3dDevice->SetMaterial( &mtrl );
            m_pd3dDevice->SetTransform( D3DTS_WORLD, &m_matObj2 );
            m_pMesh3DText->DrawSubset(0);
        }

        // Show frame rate
        SetRect( &rc, 2, 0, 0, 0 );
        m_pStatsFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
        SetRect( &rc, 2, 15, 0, 0 );
        m_pStatsFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );

        // End the scene
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    m_pStatsFont->OnLostDevice();
    m_pFont->OnLostDevice();
    m_pFont2->OnLostDevice();
    m_pD3DXSprite->OnLostDevice();
    SAFE_RELEASE( m_pMesh3DText );

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
    SAFE_RELEASE( m_pFont2 );
    SAFE_RELEASE( m_pStatsFont );
    SAFE_RELEASE( m_pD3DXSprite );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    SAFE_DELETE_ARRAY( m_strTextBuffer );
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_SIZE:
        {
            if( SIZE_RESTORED == wParam )
            {
                if( !m_bMaximized && !m_bMinimized )
                {
                    // This sample demonstrates word wrapping, so if the 
                    // window size is changing because the user dragging the window 
                    // edges we'll recalc the size, re-init, and repaint
                    // the scene as the window resizes.  This would be very
                    // slow for complex scene but works here since this sample 
                    // is trival.
                    HandlePossibleSizeChange();

                    // Repaint the window
                    if( m_pd3dDevice && !m_bActive && m_bWindowed &&
                        m_bDeviceObjectsInited && m_bDeviceObjectsRestored )
                    {
                        HRESULT hr;
                        Render();
                        hr = m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
                        if( D3DERR_DEVICELOST == hr )
                            m_bDeviceLost = true;
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch( LOWORD(wParam) )
            {
                case IDM_FONT:
                {
                    HDC hdc;
                    LONG lHeight;
                    hdc = GetDC( hWnd );
                    lHeight = -MulDiv( m_nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
                    ReleaseDC( hWnd, hdc );
                    hdc = NULL;

                    LOGFONT lf;
                    lstrcpy( lf.lfFaceName, m_strFont );
                    lf.lfHeight = lHeight;

                    CHOOSEFONT cf;
                    ZeroMemory( &cf, sizeof(cf) );
                    cf.lStructSize = sizeof(cf);
                    cf.hwndOwner   = m_hWnd;
                    cf.lpLogFont   = &lf;
                    cf.Flags       = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_TTONLY;
                    
                    if( ChooseFont( &cf ) )
                    {
                        LPD3DXFONT pFontNew = NULL;
                        LPD3DXMESH pMesh3DTextNew = NULL;
                        TCHAR* pstrFontNameNew = lf.lfFaceName;
                        int dwFontSizeNew = cf.iPointSize / 10;
                        bool bSuccess = false;
                        bool bBold   = ((cf.nFontType & BOLD_FONTTYPE) == BOLD_FONTTYPE);
                        bool bItalic = ((cf.nFontType & ITALIC_FONTTYPE) == ITALIC_FONTTYPE);

                        HDC hDC = GetDC( NULL );
                        int nLogPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
                        ReleaseDC( NULL, hDC );

                        int nHeight = -MulDiv(dwFontSizeNew, nLogPixelsY, 72);
                        if( SUCCEEDED( D3DXCreateFont( m_pd3dDevice, nHeight, 0, bBold ? FW_BOLD : FW_NORMAL, 0, bItalic, DEFAULT_CHARSET, 
                                            OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, pstrFontNameNew, &pFontNew ) ) )
                        {
                            if( SUCCEEDED( CreateD3DXTextMesh( &pMesh3DTextNew, pstrFontNameNew, dwFontSizeNew, bBold, bItalic ) ) )
                            {
                                bSuccess = true;
                                SAFE_RELEASE( m_pFont );
                                m_pFont = pFontNew;
                                pFontNew = NULL;

                                SAFE_RELEASE( m_pMesh3DText );
                                m_pMesh3DText = pMesh3DTextNew;
                                pMesh3DTextNew = NULL;

                                lstrcpy( m_strFont, pstrFontNameNew );
                                m_nFontSize = dwFontSizeNew;
                            }
                        }
                        
                        SAFE_RELEASE( pMesh3DTextNew );
                        SAFE_RELEASE( pFontNew );

                        if( !bSuccess )
                        {
                            MessageBox( m_hWnd, TEXT("Could not create that font."), 
                                m_strWindowTitle, MB_OK );
                        }
                    }
                    break;
                }
            }
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}



