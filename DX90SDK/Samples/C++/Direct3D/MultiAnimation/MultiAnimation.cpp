//-----------------------------------------------------------------------------
// File: MultiAnimation.cpp
//
// Desc: This is an app that utilizes the CMultiAnimation library for creating
//       and running multiple animations together.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//-----------------------------------------------------------------------------




#include "dxstdafx.h"
#include "D3DFont.h"
#include "DSUtil.h"
#include "resource.h"
#include "MultiAnimation.h"
#include "Tiny.h"


using namespace std;


#define TXFILE_FLOOR _T("Floor.jpg")
#define FLOOR_TILECOUNT 2



//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{

protected:

    HINSTANCE             m_hInstance;                // app instance

    LPD3DXMESH            m_pMeshFloor;               // floor geometry
    D3DXMATRIX            m_mxFloor;                  // floor world xform
    D3DMATERIAL9          m_MatFloor;                 // floor material
    LPDIRECT3DTEXTURE9    m_pTxFloor;                 // floor texture

    ID3DXFont*            m_pFont;                    // framework font
    CFirstPersonCamera    m_Camera;                   // Camera for interactive character nagivation
    float                 m_fAspectRatio;             // Aspect ratio used by the FPS camera
    CSoundManager         m_DSound;                   // DSound class

    CMultiAnim            m_MultiAnim;                // the MultiAnim class for holding Tiny's mesh and frame hierarchy
    vector< CTiny* >      m_v_pCharacters;            // array of character objects; each can be associated with any of the CMultiAnims

    DWORD                 m_dwFollow;                 // which character the camera should follow; 0xffffffff for static camera
    bool                  m_bDrawReflection;          // reflection-drawing option
    bool                  m_bDisplayText;             // whether to display text
    bool                  m_bDisplayHelp;             // whether to display help text

    bool                  m_bPlaySounds;              // whether to play sounds

    float                 m_fLastAnimTime;            // Time for the animations

protected:

    HRESULT ConfirmDevice( D3DCAPS9*, DWORD, D3DFORMAT, D3DFORMAT );
    HRESULT OneTimeSceneInit();
    HRESULT Render();
    HRESULT FrameMove();
    HRESULT InitDeviceObjects();
    HRESULT RestoreDeviceObjects();
    HRESULT InvalidateDeviceObjects();
    HRESULT DeleteDeviceObjects();
    HRESULT FinalCleanup();
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    VOID    SetMenuStates();

public:

    CMyD3DApplication();
    HRESULT Create( HINSTANCE );
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
    // inherited
    m_strWindowTitle = _T("MultiAnimation");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;

    // intrinsic
    m_pMeshFloor = NULL;
    m_pTxFloor = NULL;

    m_pFont = NULL;

    m_dwFollow = 0xffffffff;
    m_bDrawReflection = true;
    m_bDisplayText = true;
    m_bDisplayHelp = true;

    m_fLastAnimTime = 0.0;

    m_dwCreationWidth = 640;
    m_dwCreationHeight = 480;

    m_bPlaySounds = true;
}

HRESULT CMyD3DApplication::Create( HINSTANCE hInstance )
{
    m_hInstance = hInstance;
    return CD3DApplication::Create( hInstance );
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

    // Need to support vs 1.1 or use software vertex processing
    if( pCaps->VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
    {                                                     
        if( (dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
            return E_FAIL;
    }

    // Need to support A8R8G8B8 textures
    if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                           adapterFormat, 0,
                                           D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8 ) ) )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    srand( GetTickCount() );

    // Setup the camera with view matrix
    D3DXVECTOR3 vEye( .5f, .55f, -.2f );
    D3DXVECTOR3 vAt( .5f,  .125f, .5f );
    m_Camera.SetViewParams( &vEye, &vAt );
    m_Camera.SetScalers( 0.01f, 1.0f );  // Camera movement parameters

    // set up dsound
    m_DSound.Initialize( m_hWnd, DSSCL_PRIORITY );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    HRESULT hr;

    // font stuff
    HDC hDC = GetDC( NULL );
    int nHeight = -MulDiv( 10, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    ReleaseDC( NULL, hDC );
    if( FAILED( hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("Arial"), &m_pFont ) ) )
        return hr;

    // Initialize floor textures
    TCHAR tszPath[ MAX_PATH ];
    hr = DXUtil_FindMediaFileCch( tszPath, MAX_PATH, TXFILE_FLOOR );
    if( FAILED( hr ) )
        return hr;

    hr = D3DXCreateTextureFromFile( m_pd3dDevice, tszPath, &m_pTxFloor );
    if( FAILED( hr ) )
        return hr;

    D3DXMATRIX mx;
    // floor geometry transform
    D3DXMatrixRotationX( & m_mxFloor, -D3DX_PI / 2.0f );
    D3DXMatrixRotationY( & mx, D3DX_PI / 4.0f );
    D3DXMatrixMultiply( & m_mxFloor, & m_mxFloor, & mx );
    D3DXMatrixTranslation( & mx, 0.5f, 0.0f, 0.5f );
    D3DXMatrixMultiply( & m_mxFloor, & m_mxFloor, & mx );

    // set material for floor
    m_MatFloor.Diffuse = D3DXCOLOR( 1.f, 1.f, 1.f, .75f );
    m_MatFloor.Ambient = D3DXCOLOR( 1.f, 1.f, 1.f, 1.f );
    m_MatFloor.Specular = D3DXCOLOR( 0.f, 0.f, 0.f, 1.f );
    m_MatFloor.Emissive = D3DXCOLOR( .0f, 0.f, 0.f, 0.f );
    m_MatFloor.Power = 0.f;

    SetMenuStates();

    // set up MultiAnim
    TCHAR sXFile[ MAX_PATH ];
    TCHAR sFXFile[ MAX_PATH ];

    hr = DXUtil_FindMediaFileCch( sXFile, MAX_PATH, _T("tiny_4anim.x") );
    if( FAILED( hr ) )
        return hr;
    hr = DXUtil_FindMediaFileCch( sFXFile, MAX_PATH, _T("MultiAnimation.fx") );
    if( FAILED( hr ) )
        return hr;

    CMultiAnimAllocateHierarchy AH;
    AH.SetMA( & m_MultiAnim );

    hr = m_MultiAnim.Setup( m_pd3dDevice, sXFile, sFXFile, & AH );
    if( FAILED( hr ) )
        return hr;

    // get device caps
    D3DCAPS9 caps;
    m_pd3dDevice->GetDeviceCaps( & caps );

    // Select the technique that fits the shader version.
    // We could have used ValidateTechnique()/GetNextValidTechniqe() to find the
    // best one, but this is convenient for our purposes.
    if( caps.VertexShaderVersion <= D3DVS_VERSION( 1, 1 ) )
    {
        if( caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            m_MultiAnim.SetTechnique( "t2" );
            m_MultiAnim.SetSWVP( true );
        }
        else
        {
            m_MultiAnim.SetTechnique( "t1" );
            m_MultiAnim.SetSWVP( false );
        }
    }
    else
    {
        m_MultiAnim.SetTechnique( "t0" );
        m_MultiAnim.SetSWVP( false );
    }

    // Restore steps for tiny instances
    vector< CTiny* >::iterator itCurCP, itEndCP = m_v_pCharacters.end();
    for( itCurCP = m_v_pCharacters.begin(); itCurCP != itEndCP; ++ itCurCP )
    {
        ( * itCurCP )->RestoreDeviceObjects( m_pd3dDevice );
    }

    // If there is no instance, make sure we have at least one.
    if( m_v_pCharacters.size() == 0 )
    {
        CTiny * pTiny = new CTiny;
        if( pTiny == NULL )
            return E_OUTOFMEMORY;

        hr = pTiny->Setup( & m_MultiAnim, & m_v_pCharacters, & m_DSound, 0.f );
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Initialize scene objects.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
    HRESULT hr;

    // set lighting
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_AMBIENT, D3DCOLOR_ARGB( 255, 255, 255, 255 ) );
    m_pd3dDevice->LightEnable( 0, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_NORMALIZENORMALS, TRUE );

    // Set up the camera's projection matrix
    m_fAspectRatio = m_d3dsdBackBuffer.Width / (FLOAT)m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DXToRadian(60.0f), m_fAspectRatio, 0.001f, 100.0f );

    // create the floor geometry
    LPD3DXMESH pMesh;
    hr = D3DXCreatePolygon( m_pd3dDevice, 1.2f, 4, & pMesh, NULL );
    if( FAILED( hr ) )
        return hr;

    hr = pMesh->CloneMeshFVF( D3DXMESH_WRITEONLY, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, m_pd3dDevice, & m_pMeshFloor );
    pMesh->Release();
    if( FAILED( hr ) )
        return hr;

    DWORD dwNumVx = m_pMeshFloor->GetNumVertices();

    struct Vx
    {
        D3DXVECTOR3 vPos;
        D3DXVECTOR3 vNorm;
        float fTex[ 2 ];
    };

    // Initialize its texture coordinates
    Vx * pVx;
    hr = m_pMeshFloor->LockVertexBuffer( 0, (VOID **) & pVx );
    if( FAILED( hr ) )
        return hr;

    for( DWORD i = 0; i < dwNumVx; ++ i )
    {
        if( fabs( pVx->vPos.x ) < 0.01 )
        {
            if( pVx->vPos.y > 0 )
            {
                pVx->fTex[ 0 ] = 0.0f;
                pVx->fTex[ 1 ] = 0.0f;
            } else
            if( pVx->vPos.y < 0.0f )
            {
                pVx->fTex[ 0 ] = 1.0f * FLOOR_TILECOUNT;
                pVx->fTex[ 1 ] = 1.0f * FLOOR_TILECOUNT;
            } else
            {
                pVx->fTex[ 0 ] = 0.5f * FLOOR_TILECOUNT;
                pVx->fTex[ 1 ] = 0.5f * FLOOR_TILECOUNT;
            }
        } else
        if( pVx->vPos.x > 0.0f )
        {
            pVx->fTex[ 0 ] = 1.0f * FLOOR_TILECOUNT;
            pVx->fTex[ 1 ] = 0.0f;
        } else
        {
            pVx->fTex[ 0 ] = 0.0f;
            pVx->fTex[ 1 ] = 1.0f * FLOOR_TILECOUNT;
        }

        ++ pVx;
    }

    m_pMeshFloor->UnlockVertexBuffer();

    // Restore font
    if( m_pFont )
        m_pFont->OnResetDevice();

    // reset the timer
    m_fLastAnimTime = m_fTime;

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

    SAFE_RELEASE( m_pMeshFloor );

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

    vector< CTiny* >::iterator itCurCP, itEndCP = m_v_pCharacters.end();
    for( itCurCP = m_v_pCharacters.begin(); itCurCP != itEndCP; ++ itCurCP )
    {
        ( *itCurCP )->InvalidateDeviceObjects();
    }

    CMultiAnimAllocateHierarchy AH;
    AH.SetMA( & m_MultiAnim );
    m_MultiAnim.Cleanup( & AH );

    SAFE_RELEASE( m_pTxFloor );
    SAFE_RELEASE( m_pMeshFloor );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    vector< CTiny* >::iterator itCurCP, itEndCP = m_v_pCharacters.end();
    for( itCurCP = m_v_pCharacters.begin(); itCurCP != itEndCP; ++ itCurCP )
    {
        ( * itCurCP )->Cleanup();
        delete * itCurCP;
    }
    m_v_pCharacters.clear();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    // we only need to call the animation routine 60 times per second
    if( m_fTime - m_fLastAnimTime > ( 1.0f / 60.f ) )
    {
        vector< CTiny* >::iterator itCur, itEnd = m_v_pCharacters.end();
        for( itCur = m_v_pCharacters.begin(); itCur != itEnd; ++ itCur )
            ( * itCur )->Animate( m_fTime - m_fLastAnimTime );

        m_fLastAnimTime = m_fTime;
    }

    // Update the camera 
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
    // clear
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 
        0x003fafff, 1.0f, 0L );

    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        // set up the camera
        D3DXMATRIX mx, mxView, mxProj;
        D3DXVECTOR3 vEye;
        D3DXVECTOR3 vLightDir;

        // are we following a tiny, or an independent arcball camera?
        if( m_dwFollow == 0xffffffff )
        {
            // Light direction is same as camera front (reversed)
            vLightDir = -m_Camera.GetWorldAhead();
            
            // set static transforms
            mxView = *m_Camera.GetViewMatrix();
            mxProj = *m_Camera.GetProjMatrix();
            m_pd3dDevice->SetTransform( D3DTS_VIEW, & mxView );
            m_pd3dDevice->SetTransform( D3DTS_PROJECTION, & mxProj );
            vEye = m_Camera.GetEyePt();
        }
        else
        {
            // set follow transforms
            CTiny * pChar = m_v_pCharacters[ m_dwFollow ];

            D3DXVECTOR3 vCharPos;
            D3DXVECTOR3 vCharFacing;
            pChar->GetPosition( & vCharPos );
            pChar->GetFacing( & vCharFacing );
            vEye = D3DXVECTOR3  ( vCharPos.x, 0.25f, vCharPos.z );
            D3DXVECTOR3 vAt     ( vCharPos.x, 0.0125f, vCharPos.z ),
                        vUp     ( 0.0f, 1.0f, 0.0f );
            vCharFacing.x *= .25; vCharFacing.y = 0.f; vCharFacing.z *= .25;
            vEye -= vCharFacing;
            vAt += vCharFacing;

            D3DXMatrixLookAtLH( & mxView, & vEye, & vAt, & vUp );
            m_pd3dDevice->SetTransform( D3DTS_VIEW, & mxView );

            D3DXMatrixPerspectiveFovLH( &mxProj, D3DXToRadian(60.0f), m_fAspectRatio, 0.1f, 100.0f );
            m_pd3dDevice->SetTransform( D3DTS_PROJECTION, & mxProj );

            // Set the light direction and normalize
            D3DXVec3Subtract( &vLightDir, &vEye, &vAt );
            D3DXVec3Normalize( &vLightDir, &vLightDir );
        }

        LPD3DXEFFECT pEffect = m_MultiAnim.GetEffect();
        if( pEffect )
        {
            // set view-proj
            D3DXMatrixMultiply( & mx, & mxView, & mxProj );
            pEffect->SetMatrix( "mViewProj", & mx );

            // Set the light direction so that the
            // visible side is lit.
            D3DXVECTOR4 v( vLightDir.x, vLightDir.y, vLightDir.z, 1.0f );
            pEffect->SetVector( "lhtDir", &v );

            pEffect->Release();
        }

        // set the fixed function shader for drawing the floor
        m_pd3dDevice->SetVertexShader( NULL );
        m_pd3dDevice->SetFVF( m_pMeshFloor->GetFVF() );

        // draw the floor
        m_pd3dDevice->SetTexture( 0, m_pTxFloor );
        m_pd3dDevice->SetMaterial( & m_MatFloor );
        m_pd3dDevice->SetTransform( D3DTS_WORLD, & m_mxFloor );
        m_pMeshFloor->DrawSubset( 0 );

        // draw each tiny
        vector< CTiny* >::iterator itCur, itEnd = m_v_pCharacters.end();
        for( itCur = m_v_pCharacters.begin(); itCur != itEnd; ++ itCur )
        {
            // set the time to update the hierarchy
            ( * itCur )->AdvanceTime( m_fElapsedTime, & vEye );
            // draw the mesh
            ( * itCur )->Draw();
        }

        //
        // Output text information
        //
        if( m_pFont )
        {
            RECT rc;
            TCHAR sDesc[ 256 ];

            // Dump out the FPS and device stats
            SetRect( &rc, 2, 0, 0, 0 );
            m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
            rc.top += 16;
            m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
            rc.top += 16;
            _stprintf( sDesc, _T("  Time: %2.3f"), m_fTime );
            m_pFont->DrawText( NULL, sDesc, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
            rc.top += 16;
            _stprintf( sDesc, _T("  Number of models: %d"), m_v_pCharacters.size() );
            m_pFont->DrawText( NULL, sDesc, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));

            // We can only display either the behavior text or help text,
            // with the help text having priority.
            if( m_bDisplayHelp )
            {
                rc.top += 16;
                if( m_dwFollow == 0xFFFFFFFF )
                    m_pFont->DrawText( NULL, _T("Press F1 to hide help"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                else
                    // Following an instance. Let user know F1 shows animation info
                    m_pFont->DrawText( NULL, _T("Press F1 to show animation info"), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));

                rc.left = 20;
                rc.top = m_d3dsdBackBuffer.Height - 165;
                m_pFont->DrawText( NULL,
                                   _T("Global Controls:\n")
                                   _T("Add instance\n")
                                   _T("Next view\n")
                                   _T("Previous view\n")
                                   _T("Reset camera\n")
                                   _T("Reset time\n")
                                   _T("Toggle behavior text\n")
                                   _T("Change device\n")
                                   _T("Quit\n")
                                   _T("Relinquish control for all instances"),
                                   -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                rc.left = 250;
                m_pFont->DrawText( NULL,
                                   _T("\n")
                                   _T("Z\n")
                                   _T("N\n")
                                   _T("P\n")
                                   _T("R\n")
                                   _T("I\n")
                                   _T("T\n")
                                   _T("F2\n")
                                   _T("Esc\n")
                                   _T("V"),
                                   -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                rc.top = m_d3dsdBackBuffer.Height - 85;
                rc.left = 370;
                m_pFont->DrawText( NULL,
                                   _T("For a specific instance:\n")
                                   _T("Take control\n")
                                   _T("Move forward\n")
                                   _T("Turn\n")
                                   _T("Enable run mode\n"),
                                   -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                rc.left = 520;
                m_pFont->DrawText( NULL,
                                   _T("\n")
                                   _T("C\n")
                                   _T("W\n")
                                   _T("A,D\n")
                                   _T("Shift\n"),
                                   -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));

            } else
            if( m_bDisplayText )
            {
                rc.top += 16;
                m_pFont->DrawText( NULL, _T("Press F1 for help."), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));

                // output data for T[m_dwFollow]
                if( m_dwFollow != 0xffffffff )
                {
                    int i;
                    CTiny * pChar = m_v_pCharacters[ m_dwFollow ];
                    vector < String > v_sReport;
                    pChar->Report( v_sReport );
                    rc.left = 5;
                    rc.top = m_d3dsdBackBuffer.Height - 115;
                    for( i = 0; i < 6; ++i )
                    {
                        m_pFont->DrawText( NULL, v_sReport[i].c_str(), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                        rc.top += 16;
                    }
                    m_pFont->DrawText( NULL, v_sReport[16].c_str(), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));

                    rc.left = 220;
                    rc.top = m_d3dsdBackBuffer.Height - 85;
                    for( i = 6; i < 11; ++i )
                    {
                        m_pFont->DrawText( NULL, v_sReport[i].c_str(), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                        rc.top += 16;
                    }

                    rc.left = 390;
                    rc.top = m_d3dsdBackBuffer.Height - 85;
                    for( i = 11; i < 16; ++i )
                    {
                        m_pFont->DrawText( NULL, v_sReport[i].c_str(), -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
                        rc.top += 16;
                    }
                }
            }
        }

        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message proc function to handle key and menu input
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Pass messages to camera class for camera movement if the
    // global camera if active
    if( -1 == m_dwFollow )
        m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    // Handle key presses
    switch( uMsg )
    {
    case WM_COMMAND:
        {
            switch( LOWORD( wParam ) )
            {
                case IDM_TOGGLEHELP:
                    m_bDisplayHelp = !m_bDisplayHelp;
                    break;

                case ID_OPTIONS_REFLECTION:
                    m_bDrawReflection = ! m_bDrawReflection;
                    break;

                case ID_OPTIONS_TAKECONTROL:
                    if( m_dwFollow != 0xffffffff )
                    {
                        CTiny * pChar = m_v_pCharacters[ m_dwFollow ];
                        pChar->SetUserControl();
                    }
                    break;

                case ID_OPTIONS_RESETTIME:
                {
                    DXUtil_Timer( TIMER_RESET );
                    m_fTime = DXUtil_Timer( TIMER_GETAPPTIME );
                    m_fLastAnimTime = m_fTime;
                    vector< CTiny* >::iterator itCur, itEnd = m_v_pCharacters.end();
                    for( itCur = m_v_pCharacters.begin(); itCur != itEnd; ++ itCur )
                        ( * itCur )->ResetTime();
                    break;
                }

                case ID_OPTIONS_TOGGLESOUNDS:
                {
                    m_bPlaySounds = ! m_bPlaySounds;
                    vector< CTiny* >::iterator itCur, itEnd = m_v_pCharacters.end();
                    for( itCur = m_v_pCharacters.begin(); itCur != itEnd; ++ itCur )
                        ( * itCur )->SetSounds( m_bPlaySounds );
                    break;
                }

                case IDM_ADDTINY:
                {
                    HRESULT hr;

                    CTiny * pTiny = new CTiny;
                    if( pTiny == NULL )
                        break;

                    hr = pTiny->Setup( & m_MultiAnim, & m_v_pCharacters, & m_DSound, m_fTime );
                    if( SUCCEEDED( hr ) )
                        pTiny->SetSounds( m_bPlaySounds );
                    else
                        delete pTiny;
                    break;
                }

                case ID_VIEW_TOGGLEBEHAVIORTEXT:
                    m_bDisplayText = ! m_bDisplayText;
                    break;

                case ID_VIEW_RESETCAMERA:
                    m_dwFollow = 0xffffffff;
                    break;

                case ID_VIEW_PREVIOUSVIEW:
                    if( m_v_pCharacters.size() != 0 )
                    {
                        if( m_dwFollow == 0xffffffff )
                            m_dwFollow = (DWORD) m_v_pCharacters.size() - 1;
                        else if( m_dwFollow == 0 )
                            m_dwFollow = 0xffffffff;
                        else
                            -- m_dwFollow;
                    }
                    break;

                case ID_VIEW_NEXTVIEW:
                    if( m_v_pCharacters.size() != 0 )
                    {
                        if( m_dwFollow == 0xffffffff )
                            m_dwFollow = 0;
                        else if( m_dwFollow == (DWORD) m_v_pCharacters.size() - 1 )
                            m_dwFollow = 0xffffffff;
                        else
                            ++ m_dwFollow;
                    }
                    break;
            }

            // Update the menus, in case any state changes occurred
            SetMenuStates();
        }
    }

    // Pass remaining messages to default handler
    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: SetMenuStates()
// Desc:
//-----------------------------------------------------------------------------
void CMyD3DApplication::SetMenuStates()
{
    HMENU hMenu = GetMenu( m_hWnd );

    CheckMenuItem( hMenu, ID_OPTIONS_REFLECTION,
                    m_bDrawReflection ? MF_CHECKED : MF_UNCHECKED );
    EnableMenuItem( hMenu, ID_OPTIONS_TAKECONTROL,
                    m_dwFollow != 0xffffffff ? MF_ENABLED : MF_GRAYED );
}
