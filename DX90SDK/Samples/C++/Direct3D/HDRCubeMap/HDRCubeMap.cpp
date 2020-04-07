//-----------------------------------------------------------------------------
// File: HDRCubeMap.cpp
//
// Desc: DirectX window application created by the DirectX AppWizard
//-----------------------------------------------------------------------------
#define STRICT
#include "dxstdafx.h"
#include <commdlg.h>
#include <basetsd.h>
#include <math.h>
#include <dxerr9.h>
#include "D3DFile.h"
#include "resource.h"
#pragma warning( disable : 4324 ) 

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------



#define ENVMAPSIZE 256
// Currently, 4 is the only number of lights supported.
#define NUM_LIGHTS 4
#define LIGHTMESH_RADIUS 0.2f
#define HELPTEXTCOLOR D3DXCOLOR( 0.0f, 1.0f, 0.3f, 1.0f )


D3DVERTEXELEMENT9 g_aVertDecl[] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0 },
    { 0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    D3DDECL_END()
};



LPCTSTR g_atszMeshFileName[] =
{
    _T("teapot.x"),
    _T("skullocc.x"),
    _T("car.x")
};

#define NUM_MESHES ( sizeof(g_atszMeshFileName) / sizeof(g_atszMeshFileName[0]) )

struct ORBITDATA
{
    LPCTSTR     tszXFile;  // X file
    D3DVECTOR   vAxis;     // Axis of rotation
    float       fRadius;   // Orbit radius
    float       fSpeed;    // Orbit speed in radians per second
};



// Mesh file to use for orbit objects
// These don't have to use the same mesh file.
ORBITDATA g_OrbitData[] =
{
    {_T("airplane 2.x"), { -1.0f, 0.0f, 0.0f }, 2.0f, D3DX_PI * 1.0f },
    {_T("airplane 2.x"), { 0.3f,  1.0f, 0.3f }, 2.5f, D3DX_PI / 2.0f }
};


HRESULT ComputeBoundingSphere( CD3DMesh &Mesh, D3DXVECTOR3 *pvCtr, float *pfRadius )
{
    HRESULT hr;

    // Obtain the bounding sphere
    LPD3DXMESH pMeshObj = Mesh.GetSysMemMesh();
    if( !pMeshObj )
        return E_FAIL;

    // Obtain the declaration
    D3DVERTEXELEMENT9 decl[MAX_FVF_DECL_SIZE];
    if( FAILED( hr = pMeshObj->GetDeclaration( decl ) ) )
        return hr;

    // Lock the vertex buffer
    LPVOID pVB;
    if( FAILED( hr = pMeshObj->LockVertexBuffer( D3DLOCK_READONLY, &pVB ) ) )
        return hr;

    // Compute the bounding sphere
    UINT uStride = D3DXGetDeclVertexSize( decl, 0 );
    hr = D3DXComputeBoundingSphere( (const D3DXVECTOR3 *)pVB, pMeshObj->GetNumVertices(),
                                    uStride, pvCtr, pfRadius );
    pMeshObj->UnlockVertexBuffer();

    return hr;
}




//-----------------------------------------------------------------------------
// Name: struct CObj
// Desc: Encapsulate an object in the scene with its world transformation
//       matrix.
//-----------------------------------------------------------------------------
struct CObj
{
    D3DXMATRIXA16 m_mWorld;  // World transformation
    CD3DMesh      m_Mesh;    // Mesh object

public:
    CObj()
        { D3DXMatrixIdentity( &m_mWorld ); }
    ~CObj()
        { }
    HRESULT WorldCenterAndScaleToRadius( float fRadius )
    {
        //
        // Compute the world transformation matrix
        // to center the mesh at origin in world space
        // and scale its size to the specified radius.
        //
        HRESULT hr;

        float fRadiusBound;
        D3DXVECTOR3 vCtr;
        if( FAILED( hr = ::ComputeBoundingSphere( m_Mesh, &vCtr, &fRadiusBound ) ) )
            return hr;

        D3DXMatrixTranslation( &m_mWorld, -vCtr.x, -vCtr.y, -vCtr.z );
        D3DXMATRIXA16 mScale;
        D3DXMatrixScaling( &mScale, fRadius / fRadiusBound,
                                    fRadius / fRadiusBound,
                                    fRadius / fRadiusBound );
        D3DXMatrixMultiply( &m_mWorld, &m_mWorld, &mScale );

        return hr;
    }  // WorldCenterAndScaleToRadius
};




//-----------------------------------------------------------------------------
// Name: class COrbit
// Desc: Encapsulate an orbit object in the scene with related data
//-----------------------------------------------------------------------------
class COrbit : public CObj
{
public:
    D3DXVECTOR3   m_vAxis;       // orbit axis
    float         m_fRadius;     // orbit radius
    float         m_fSpeed;      // Speed, angle in radian per second

public:
    COrbit() : m_vAxis( 0.0f, 1.0f, 0.0f ), m_fRadius(1.0f), m_fSpeed( D3DX_PI )
        { }
    void SetOrbit( D3DXVECTOR3 &vAxis, float fRadius, float fSpeed )
    {
        // Call this after m_mWorld is initialized
        m_vAxis = vAxis; m_fRadius = fRadius; m_fSpeed = fSpeed;
        D3DXVec3Normalize( &m_vAxis, &m_vAxis );

        // Translate by m_fRadius in -Y direction
        D3DXMATRIXA16 m;
        D3DXMatrixTranslation( &m, 0.0f, -m_fRadius, 0.0f );
        D3DXMatrixMultiply(&m_mWorld, &m_mWorld, &m );

        // Apply rotation from X axis to the orbit axis
        D3DXVECTOR3 vX(1.0f, 0.0f, 0.0f);
        D3DXVECTOR3 vRot;
        D3DXVec3Cross( &vRot, &m_vAxis, &vX );  // Axis of rotation
        // If the cross product is 0, m_vAxis is on the X axis
        // So we either rotate 0 or PI.
        if( D3DXVec3LengthSq( &vRot ) == 0 )
        {
            if( m_vAxis.x < 0.0f )
                D3DXMatrixRotationY( &m, D3DX_PI ); // PI
            else
                D3DXMatrixIdentity( &m );           // 0
        } else
        {
            float fAng = (float)acos( D3DXVec3Dot( &m_vAxis, &vX ) );  // Angle to rotate
            // Find out direction to rotate
            D3DXVECTOR3 vXxRot;  // X cross vRot
            D3DXVec3Cross( &vXxRot, &vX, &vRot );
            if( D3DXVec3Dot( &vXxRot, &m_vAxis ) >= 0 )
                fAng = -fAng;
            D3DXMatrixRotationAxis( &m, &vRot, fAng );
        }
        D3DXMatrixMultiply( &m_mWorld, &m_mWorld, &m );
    }
    void Orbit( float fElapsedTime )
    {
        // Compute the orbit transformation and apply to m_mWorld
        D3DXMATRIXA16 m;

        D3DXMatrixRotationAxis( &m, &m_vAxis, m_fSpeed * fElapsedTime );
        D3DXMatrixMultiply( &m_mWorld, &m_mWorld, &m );
    }
};




struct CLight
{
    D3DXVECTOR4   vPos;      // Position in world space
    D3DXVECTOR4   vMoveDir;  // Direction in which it moves
    float         fMoveDist; // Maximum distance it can move
    D3DXMATRIXA16 mWorld;    // World transform matrix for the light before animation
    D3DXMATRIXA16 mWorking;  // Working matrix (world transform after animation)
};




//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Application class. The base class (CD3DApplication) provides the 
//       generic functionality needed in all Direct3D samples. CMyD3DApplication
//       adds functionality specific to this sample program.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
    ID3DXFont*                   m_pFont;                   // Font for drawing text
    ID3DXFont*                   m_pFontSmall;              // Font for drawing text
    CModelViewerCamera           m_Camera;                  // View camera
    CLight                       m_aLights[NUM_LIGHTS];     // Parameters of light objects
    D3DXVECTOR4                  m_vLightIntensity;         // Light intensity
    LPD3DXEFFECT                 m_pEffect;                 // Effect for the sample
    CObj                         m_EnvMesh[NUM_MESHES];     // Mesh to receive environment mapping
    int                          m_nCurrMesh;               // Index of the element in m_EnvMesh we are currently displaying
    CD3DMesh*                    m_pRoom;                   // Mesh representing room (wall, floor, ceiling)
    COrbit                       m_Orbit[sizeof(g_OrbitData) /
                                         sizeof(g_OrbitData[0])]; // Orbit meshes
    CD3DMesh*                    m_pLightMesh;              // Mesh for the light object
    LPDIRECT3DVERTEXDECLARATION9 m_pVertDecl;               // Vertex decl for the sample
    LPDIRECT3DCUBETEXTURE9       m_pCubeMapFp;              // Floating point format cube map
    LPDIRECT3DCUBETEXTURE9       m_pCubeMap32;              // 32-bit cube map (for fallback)
    LPDIRECT3DSURFACE9           m_pDepthCube;              // Depth-stencil buffer for rendering to cube texture

    D3DXHANDLE                   m_hTechRenderScene;        // Technique handle for RenderScene
    D3DXHANDLE                   m_hTechRenderHDREnvMap;    // Technique handle for RenderHDREnvMap
    D3DXHANDLE                   m_hTechRenderLight;        // Technique handle for RenderLight
    D3DXHANDLE                   m_hWorldView;              // Handle for world+view matrix in effect
    D3DXHANDLE                   m_hProj;                   // Handle for projection matrix in effect
    D3DXHANDLE                   m_htxCubeMap;              // Handle for the cube texture in effect
    D3DXHANDLE                   m_htxScene;                // Handle for the scene texture in effect
    D3DXHANDLE                   m_hLightIntensity;         // Handle for the light intensity in effect
    D3DXHANDLE                   m_hLightPosView;           // Handle for view space light positions in effect
    D3DXHANDLE                   m_hReflectivity;           // Handle for reflectivity in effect

    FLOAT                        m_fWorldRotX;              // World rotation state X-axis
    FLOAT                        m_fWorldRotY;              // World rotation state Y-axis
    BOOL                         m_bShowHelp;
    BOOL                         m_bUseFloatCubeMap;        // Whether we use floating point format cube map
    float                        m_fReflectivity;           // Reflectivity value. Ranges from 0 to 1.

protected:
    virtual HRESULT OneTimeSceneInit();
    virtual HRESULT InitDeviceObjects();
    virtual HRESULT RestoreDeviceObjects();
    virtual HRESULT InvalidateDeviceObjects();
    virtual HRESULT DeleteDeviceObjects();
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT FinalCleanup();
    virtual HRESULT ConfirmDevice( D3DCAPS9*, DWORD, D3DFORMAT, D3DFORMAT );

    void ResetParameters();
    HRESULT RenderText();

    HRESULT LoadMesh( LPCTSTR tszName, CD3DMesh &Mesh );

    HRESULT RenderSceneIntoCubeMap();
    HRESULT RenderScene( D3DXMATRIX *pmView, D3DXMATRIX *pmProj, bool bRenderEnvMappedMesh );

public:
    LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
    CMyD3DApplication();
    virtual ~CMyD3DApplication();
};




//-----------------------------------------------------------------------------
// Global access to the app (needed for the global WndProc())
//-----------------------------------------------------------------------------
CMyD3DApplication* g_pApp  = NULL;
HINSTANCE          g_hInst = NULL;




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    g_pApp  = &d3dApp;
    g_hInst = hInst;

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
    m_strWindowTitle            = TEXT( "HDRCubeMap" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
    m_bStartFullscreen			= false;
    m_bShowCursorWhenFullscreen	= true;

    m_pFont                     = NULL;
    m_pFontSmall                = NULL;

    m_dwCreationWidth           = 640;
    m_dwCreationHeight          = 480;

    m_fWorldRotX                = 0.0f;
    m_fWorldRotY                = 0.0f;

    m_bShowHelp                 = TRUE;

    m_pEffect                   = NULL;
    m_pRoom                     = NULL;
    m_pLightMesh                = NULL;
    m_pVertDecl                 = NULL;

    m_hTechRenderScene          = NULL;
    m_hTechRenderHDREnvMap      = NULL;
    m_hTechRenderLight          = NULL;
    m_hWorldView                = NULL;
    m_hProj                     = NULL;
    m_htxCubeMap                = NULL;
    m_htxScene                  = NULL;
    m_hLightIntensity           = NULL;
    m_hLightPosView             = NULL;
    m_hReflectivity             = NULL;

    m_pCubeMapFp                = NULL;
    m_pCubeMap32                = NULL;
    m_pDepthCube                = NULL;

    m_nCurrMesh                 = 0;
    ResetParameters();
}




//-----------------------------------------------------------------------------
// Name: ~CMyD3DApplication()
// Desc: Application destructor.  Paired with CMyD3DApplication()
//-----------------------------------------------------------------------------
CMyD3DApplication::~CMyD3DApplication()
{
}




//-----------------------------------------------------------------------------
// Name: ResetParameters()
// Desc: Reset light and material parameters to default values
//-----------------------------------------------------------------------------
void CMyD3DApplication::ResetParameters()
{
    m_bUseFloatCubeMap          = TRUE;
    m_fReflectivity             = 0.4f;
    m_vLightIntensity           = D3DXVECTOR4( 24.0f, 24.0f, 24.0f, 24.0f );

    if( m_pEffect )
    {
        m_pEffect->SetFloat( m_hReflectivity, m_fReflectivity );
        m_pEffect->SetVector( m_hLightIntensity, &m_vLightIntensity );
    }
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
    // Initialize camera parameters
    m_Camera.SetModelCenter( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
    m_Camera.SetRadius( 3.0f );
    m_Camera.SetEnablePositionMovement( false );

    m_pRoom = new CD3DMesh;
    if( !m_pRoom )
        return E_OUTOFMEMORY;

    m_pLightMesh = new CD3DMesh;
    if( !m_pLightMesh )
        return E_OUTOFMEMORY;

    // Set the light positions
    m_aLights[0].vPos = D3DXVECTOR4( -3.5f, 2.3f, -4.0f, 1.0f );
    m_aLights[0].vMoveDir = D3DXVECTOR4( 0.0f, 0.0f, 1.0f, 0.0f );
    m_aLights[0].fMoveDist = 8.0f;
#if NUM_LIGHTS > 1
    m_aLights[1].vPos = D3DXVECTOR4(  3.5f, 2.3f,  4.0f, 1.0f );
    m_aLights[1].vMoveDir = D3DXVECTOR4( 0.0f, 0.0f, -1.0f, 0.0f );
    m_aLights[1].fMoveDist = 8.0f;
#endif
#if NUM_LIGHTS > 2
    m_aLights[2].vPos = D3DXVECTOR4( -3.5f, 2.3f,  4.0f, 1.0f );
    m_aLights[2].vMoveDir = D3DXVECTOR4( 1.0f, 0.0f, 0.0f, 0.0f );
    m_aLights[2].fMoveDist = 7.0f;
#endif
#if NUM_LIGHTS > 3
    m_aLights[3].vPos = D3DXVECTOR4(  3.5f, 2.3f, -4.0f, 1.0f );
    m_aLights[3].vMoveDir = D3DXVECTOR4( -1.0f, 0.0f, 0.0f, 0.0f );
    m_aLights[3].fMoveDist = 7.0f;
#endif

    // Drawing loading status message until app finishes loading
    SendMessage( m_hWnd, WM_PAINT, 0, 0 );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device initialization, this code checks the display device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
                                          D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
    UNREFERENCED_PARAMETER( adapterFormat );
    UNREFERENCED_PARAMETER( backBufferFormat );
    UNREFERENCED_PARAMETER( dwBehavior );
    UNREFERENCED_PARAMETER( pCaps );

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

    // Must support cube textures
    if( !( pCaps->TextureCaps & D3DPTEXTURECAPS_CUBEMAP ) )
        return E_FAIL;

    // Must support vertex shader 1.1 or use software vertex processing
    if( pCaps->VertexShaderVersion < D3DVS_VERSION( 1, 1 ) &&
        ( dwBehavior & D3DCREATE_SOFTWARE_VERTEXPROCESSING ) == 0 )
        return E_FAIL;

    // Must support pixel shader 2.0
    if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return E_FAIL;

    // need to support D3DFMT_A16B16G16R16F render target
    if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    adapterFormat, D3DUSAGE_RENDERTARGET, 
                    D3DRTYPE_CUBETEXTURE, D3DFMT_A16B16G16R16F ) ) )
    {
        return E_FAIL;
    }

    // need to support D3DFMT_A8R8G8B8 render target
    if( FAILED( m_pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                    adapterFormat, D3DUSAGE_RENDERTARGET, 
                    D3DRTYPE_CUBETEXTURE, D3DFMT_A8R8G8B8 ) ) )
    {
        return E_FAIL;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CMyD3DApplication::LoadMesh
// Desc: Load mesh from file and convert vertices to our format
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::LoadMesh( LPCTSTR tszName, CD3DMesh &Mesh )
{
    HRESULT hr;

    if( FAILED( hr = Mesh.Create( m_pd3dDevice, tszName ) ) )
        return hr;
    hr = Mesh.SetVertexDecl( m_pd3dDevice, g_aVertDecl );

    return hr;
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
    int i;
    HRESULT hr;

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
    TCHAR tszPath[MAX_PATH];
    if( FAILED( hr = DXUtil_FindMediaFileCch( tszPath, MAX_PATH, _T("HDRCubeMap.fx") ) ) )
        return hr;
    if( FAILED( hr = D3DXCreateEffectFromFile( m_pd3dDevice,
                                               tszPath,
                                               NULL,
                                               NULL,
                                               dwShaderFlags,
                                               NULL,
                                               &m_pEffect,
                                               NULL ) ) )
        return hr;

    m_hTechRenderScene     = m_pEffect->GetTechniqueByName( "RenderScene" );
    m_hTechRenderHDREnvMap = m_pEffect->GetTechniqueByName( "RenderHDREnvMap" );
    m_hTechRenderLight     = m_pEffect->GetTechniqueByName( "RenderLight" );
    m_hWorldView           = m_pEffect->GetParameterByName( NULL, "g_mWorldView" );
    m_hProj                = m_pEffect->GetParameterByName( NULL, "g_mProj" );
    m_htxCubeMap           = m_pEffect->GetParameterByName( NULL, "g_txCubeMap" );
    m_htxScene             = m_pEffect->GetParameterByName( NULL, "g_txScene" );
    m_hLightIntensity      = m_pEffect->GetParameterByName( NULL, "g_vLightIntensity" );
    m_hLightPosView        = m_pEffect->GetParameterByName( NULL, "g_vLightPosView" );
    m_hReflectivity        = m_pEffect->GetParameterByName( NULL, "g_fReflectivity" );

    // Initialize reflectivity
    m_pEffect->SetFloat( m_hReflectivity, m_fReflectivity );

    // Initialize light intensity
    m_pEffect->SetVector( m_hLightIntensity, &m_vLightIntensity );

    // Create vertex declaration
    if( FAILED( hr = m_pd3dDevice->CreateVertexDeclaration( g_aVertDecl, &m_pVertDecl ) ) )
    {
        return hr;
    }

    // Init the font
    HDC hDC = GetDC( NULL );
    int nHeight = -MulDiv( 12, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    if( FAILED( hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("Arial"), &m_pFont ) ) )
        return hr;
    nHeight = -MulDiv( 9, GetDeviceCaps(hDC, LOGPIXELSY), 72 );
    if( FAILED( hr = D3DXCreateFont( m_pd3dDevice, nHeight, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, 
                         OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                         TEXT("Arial"), &m_pFontSmall ) ) )
        return hr;
    ReleaseDC( NULL, hDC );

    // Load the mesh
    for( i = 0; i < NUM_MESHES; ++i )
    {
        if( FAILED( LoadMesh( g_atszMeshFileName[i], m_EnvMesh[i].m_Mesh ) ) )
            return D3DAPPERR_MEDIANOTFOUND;
        m_EnvMesh[i].WorldCenterAndScaleToRadius( 1.0f );  // Scale to radius of 1
    }
    // Load the room object
    if( FAILED( LoadMesh( _T("room.x"), *m_pRoom ) ) )
        return D3DAPPERR_MEDIANOTFOUND;
    // Load the light object
    if( FAILED( LoadMesh( _T("sphere0.x"), *m_pLightMesh ) ) )
        return D3DAPPERR_MEDIANOTFOUND;
    // Initialize the world matrix for the lights
    D3DXVECTOR3 vCtr;
    float fRadius;
    if( FAILED( ComputeBoundingSphere( *m_pLightMesh, &vCtr, &fRadius ) ) )
        return E_FAIL;
    D3DXMATRIXA16 mWorld, m;
    D3DXMatrixTranslation( &mWorld, -vCtr.x, -vCtr.y, -vCtr.z );
    D3DXMatrixScaling( &m, LIGHTMESH_RADIUS / fRadius,
                            LIGHTMESH_RADIUS / fRadius,
                            LIGHTMESH_RADIUS / fRadius );
    D3DXMatrixMultiply( &mWorld, &mWorld, &m );
    for( i = 0; i < NUM_LIGHTS; ++i )
    {
        D3DXMatrixTranslation( &m, m_aLights[i].vPos.x,
                                    m_aLights[i].vPos.y,
                                    m_aLights[i].vPos.z );
        D3DXMatrixMultiply( &m_aLights[i].mWorld, &mWorld, &m );
    }

    // Orbit
    for( i = 0; i < sizeof(m_Orbit) / sizeof(m_Orbit[0]); ++i )
    {
        if( FAILED( LoadMesh( g_OrbitData[i].tszXFile, m_Orbit[i].m_Mesh ) ) )
            return D3DAPPERR_MEDIANOTFOUND;

        m_Orbit[i].WorldCenterAndScaleToRadius( 0.7f );
        D3DXVECTOR3 vAxis( g_OrbitData[i].vAxis );
        m_Orbit[i].SetOrbit( vAxis, g_OrbitData[i].fRadius, g_OrbitData[i].fSpeed );
    }

    // World transform to identity
    D3DXMATRIXA16 mIdent;
    D3DXMatrixIdentity( &mIdent );
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &mIdent );

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
    int i;
    HRESULT hr;

    // Restore effect object
    m_pEffect->OnResetDevice();

    // Set the camera projection matrix
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI/4, fAspect, 0.01f, 1000.0f );

    // Set up the view parameters for the camera
    D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, -10.0f );
    D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    m_Camera.SetViewParams( &vFromPt, &vLookatPt );

    // Set up the window parameter for the camera
    m_Camera.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height, 1.0f );

    // Restore the font
    m_pFont->OnResetDevice();
    m_pFontSmall->OnResetDevice();

    // Restore meshes
    for( i = 0; i < NUM_MESHES; ++i )
        m_EnvMesh[i].m_Mesh.RestoreDeviceObjects( m_pd3dDevice );
    m_pRoom->RestoreDeviceObjects( m_pd3dDevice );
    m_pLightMesh->RestoreDeviceObjects( m_pd3dDevice );
    for( i = 0; i < sizeof(m_Orbit) / sizeof(m_Orbit[0]); ++i )
        m_Orbit[i].m_Mesh.RestoreDeviceObjects( m_pd3dDevice );

    // Create the cube textures
    if( FAILED( hr = m_pd3dDevice->CreateCubeTexture( ENVMAPSIZE,
                                                      1,
                                                      D3DUSAGE_RENDERTARGET,
                                                      D3DFMT_A16B16G16R16F,
                                                      D3DPOOL_DEFAULT,
                                                      &m_pCubeMapFp,
                                                      NULL ) ) )
        return hr;

    if( FAILED( hr = m_pd3dDevice->CreateCubeTexture( ENVMAPSIZE,
                                                      1,
                                                      D3DUSAGE_RENDERTARGET,
                                                      D3DFMT_A8R8G8B8,
                                                      D3DPOOL_DEFAULT,
                                                      &m_pCubeMap32,
                                                      NULL ) ) )
        return hr;

    // Create the stencil buffer to be used with the cube textures
    if( FAILED( hr = m_pd3dDevice->CreateDepthStencilSurface( ENVMAPSIZE,
                                                              ENVMAPSIZE,
                                                              m_d3dSettings.DepthStencilBufferFormat(),
                                                              m_d3dSettings.MultisampleType(),
                                                              m_d3dSettings.MultisampleQuality(),
                                                              TRUE,
                                                              &m_pDepthCube,
                                                              NULL ) ) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    for( int i = 0; i < sizeof(m_Orbit) / sizeof(m_Orbit[0]); ++i )
        m_Orbit[i].Orbit( m_fElapsedTime );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderSceneIntoCubeMap()
// Desc: Set up the cube map by rendering the scene into it.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderSceneIntoCubeMap()
{
    HRESULT hr;
    LPDIRECT3DCUBETEXTURE9 pCubeMap = m_bUseFloatCubeMap ? m_pCubeMapFp
                                                         : m_pCubeMap32;

    // The projection matrix has a FOV of 90 degrees and asp ratio of 1
    D3DXMATRIXA16 mProj;
    D3DXMatrixPerspectiveFovLH( &mProj, D3DX_PI * 0.5f, 1.0f, 0.01f, 100.0f );

    D3DXMATRIXA16 mViewDir( *m_Camera.GetViewMatrix() );
    mViewDir._41 = mViewDir._42 = mViewDir._43 = 0.0f;

    LPDIRECT3DSURFACE9 pRTOld = NULL;
    m_pd3dDevice->GetRenderTarget( 0, &pRTOld );
    LPDIRECT3DSURFACE9 pDSOld = NULL;
    if( SUCCEEDED( m_pd3dDevice->GetDepthStencilSurface( &pDSOld ) ) )
    {
        // If the device has a depth-stencil buffer, use
        // the depth stencil buffer created for the cube textures.
        m_pd3dDevice->SetDepthStencilSurface( m_pDepthCube );
    }

    for( int nFace = 0; nFace < 6; ++nFace )
    {
        LPDIRECT3DSURFACE9 pSurf;

        hr = pCubeMap->GetCubeMapSurface( (D3DCUBEMAP_FACES)nFace, 0, &pSurf );
        if( FAILED( hr ) )
            continue;

        hr = m_pd3dDevice->SetRenderTarget( 0, pSurf );
        SAFE_RELEASE( pSurf );

        if( FAILED( hr ) )
            continue;

        D3DXMATRIXA16 mView = D3DUtil_GetCubeMapViewMatrix( nFace );
        D3DXMatrixMultiply( &mView, &mViewDir, &mView );

        m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER,
                             0x000000ff, 1.0f, 0L );

        // Begin the scene
        if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
        {
            RenderScene( &mView, &mProj, false );

            // End the scene.
            m_pd3dDevice->EndScene();
        }
    }

    // Restore depth-stencil buffer and render target
    if( pDSOld )
    {
        m_pd3dDevice->SetDepthStencilSurface( pDSOld );
        SAFE_RELEASE( pDSOld );
    }
    m_pd3dDevice->SetRenderTarget( 0, pRTOld );
    SAFE_RELEASE( pRTOld );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderScene()
// Desc: Renders the scene with a specific view and projection matrix.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderScene( D3DXMATRIX *pmView, D3DXMATRIX *pmProj, bool bRenderEnvMappedMesh )
{
    UINT p, cPass;
    D3DXMATRIXA16 mWorldView;

    m_pEffect->SetMatrix( m_hProj, pmProj );

    // Write camera-space light positions to effect
    D3DXVECTOR4 avLightPosView[NUM_LIGHTS];
    for( int i = 0; i < NUM_LIGHTS; ++i )
    {
        // Animate the lights
        float fDisp = ( 1.0f + cosf( fmodf( m_fTime, D3DX_PI ) ) ) * 0.5f * m_aLights[i].fMoveDist; // Distance to move
        D3DXVECTOR4 vMove = m_aLights[i].vMoveDir * fDisp;  // In vector form
        D3DXMatrixTranslation( &m_aLights[i].mWorking, vMove.x, vMove.y, vMove.z ); // Matrix form
        D3DXMatrixMultiply( &m_aLights[i].mWorking, &m_aLights[i].mWorld, &m_aLights[i].mWorking );
        vMove += m_aLights[i].vPos;  // Animated world coordinates
        D3DXVec4Transform( &avLightPosView[i], &vMove, pmView );
    }
    m_pEffect->SetVectorArray( m_hLightPosView, avLightPosView, NUM_LIGHTS );

    //
    // Render the environment-mapped mesh if specified
    //

    if( bRenderEnvMappedMesh )
    {
        m_pEffect->SetTechnique( m_hTechRenderHDREnvMap );

        // Combine the offset and scaling transformation with
        // rotation from the camera to form the final
        // world transformation matrix.
        D3DXMatrixMultiply( &mWorldView, &m_EnvMesh[m_nCurrMesh].m_mWorld, m_Camera.GetWorldMatrix() );
        D3DXMatrixMultiply( &mWorldView, &mWorldView, pmView );
        m_pEffect->SetMatrix( m_hWorldView, &mWorldView );

        LPDIRECT3DCUBETEXTURE9 pCubeMap = m_bUseFloatCubeMap ? m_pCubeMapFp
                                                             : m_pCubeMap32;
        m_pEffect->SetTexture( m_htxCubeMap, pCubeMap );

        m_pEffect->Begin( &cPass, D3DXFX_DONOTSAVESTATE );
        for( p = 0; p < cPass; ++p )
        {
            m_pEffect->Pass( p );
            LPD3DXMESH pMesh = m_EnvMesh[m_nCurrMesh].m_Mesh.GetLocalMesh();
            for( DWORD i = 0; i < m_EnvMesh[m_nCurrMesh].m_Mesh.m_dwNumMaterials; ++i )
                pMesh->DrawSubset( i );
        }
        m_pEffect->End();
    }

    //
    // Render light spheres
    //

    m_pEffect->SetTechnique( m_hTechRenderLight );

    m_pEffect->Begin( &cPass, D3DXFX_DONOTSAVESTATE );
    for( p = 0; p < cPass; ++p )
    {
        m_pEffect->Pass( p );

        for( int i = 0; i < NUM_LIGHTS; ++i )
        {
            D3DXMatrixMultiply( &mWorldView, &m_aLights[i].mWorking, pmView );
            m_pEffect->SetMatrix( m_hWorldView, &mWorldView );
            m_pLightMesh->Render( m_pd3dDevice );
        }
    }
    m_pEffect->End();

    //
    // Render the rest of the scene
    //

    m_pEffect->SetTechnique( m_hTechRenderScene );

    m_pEffect->Begin( &cPass, D3DXFX_DONOTSAVESTATE );
    for( p = 0; p < cPass; ++p )
    {
        LPD3DXMESH pMeshObj;
        m_pEffect->Pass( p );

        //
        // Orbits
        //
        for( int i = 0; i < sizeof(m_Orbit) / sizeof(m_Orbit[0]); ++i )
        {
            D3DXMatrixMultiply( &mWorldView, &m_Orbit[i].m_mWorld, pmView );
            m_pEffect->SetMatrix( m_hWorldView, &mWorldView );
            // Obtain the D3DX mesh object
            pMeshObj = m_Orbit[i].m_Mesh.GetLocalMesh();
            // Iterate through each subset and render with its texture
            for( DWORD m = 0; m < m_Orbit[i].m_Mesh.m_dwNumMaterials; ++m )
            {
                m_pEffect->SetTexture( m_htxScene, m_Orbit[i].m_Mesh.m_pTextures[m] );
                pMeshObj->DrawSubset( m );
            }
        }

        //
        // The room object (walls, floor, ceiling)
        //

        m_pEffect->SetMatrix( m_hWorldView, pmView );
        pMeshObj = m_pRoom->GetLocalMesh();
        // Iterate through each subset and render with its texture
        for( DWORD m = 0; m < m_pRoom->m_dwNumMaterials; ++m )
        {
            m_pEffect->SetTexture( m_htxScene, m_pRoom->m_pTextures[m] );
            pMeshObj->DrawSubset( m );
        }
    }
    m_pEffect->End();

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
    // Call this in Render() instead of FrameMove() so we can
    // still move camera while single stepping.
    m_Camera.FrameMove( m_fElapsedTime );

    RenderSceneIntoCubeMap();

    // Clear the render buffers
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                        0x000000ff, 1.0f, 0L );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        RenderScene( m_Camera.GetViewMatrix(), m_Camera.GetProjMatrix(), true );

        // Render stats and help text
        RenderText();

        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderText()
// Desc: Renders stats and help text to the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderText()
{
    RECT rc;

    // Output display stats

    SetRect( &rc, 2, 0, 0, 0 );
    m_pFont->DrawText( NULL, m_strFrameStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));
    rc.top += 20;
    m_pFont->DrawText( NULL, m_strDeviceStats, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ));

    // Output statistics & help
    if( m_bShowHelp )
    {
        SetRect( &rc, 2, 40, 0, 0 );
        m_pFontSmall->DrawText( NULL,
                                _T("Press F1 to hide help"),
                                -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );

        SetRect( &rc, 10, m_d3dsdBackBuffer.Height - 188, 0, 0 );
        m_pFontSmall->DrawText( NULL, _T("Keyboard controls:"), -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );
        rc.top += 20;
        m_pFontSmall->DrawText( NULL,
                                _T("Rotate object\nAdjust camera\nZoom In/Out\n")
                                _T("Change mesh\nChange cube map format\n")
                                _T("Adjust reflectivity\nAdjust light intensity\n")
                                _T("Reset parameters\n")
                                _T("Help\nChange device\nExit"),
                                -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );
        SetRect( &rc, 168, m_d3dsdBackBuffer.Height - 168, 0, 0 );
        m_pFontSmall->DrawText( NULL,
                                _T("Left drag mouse\nRight drag mouse\nMouse wheel\n")
                                _T("N\nF\n")
                                _T("E,D\nW,S\n")
                                _T("R\n")
                                _T("F1\nF2\nEsc"),
                                -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );
    } else
    {
        SetRect( &rc, 2, 40, 0, 0 );
        m_pFontSmall->DrawText( NULL,
                                _T("Press F1 for help"),
                                -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );
    }

    //
    // Output cube mapping stat
    //
    SetRect( &rc, 270, m_d3dsdBackBuffer.Height - 48, 0, 0 );
    m_pFontSmall->DrawText( NULL,
                            _T("Cube map format ( F )\n")
                            _T("Material reflectivity ( e/E, d/D )\n")
                            _T("Light intensity ( w/W, s/S )\n"),
                            -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );
    SetRect( &rc, 450, m_d3dsdBackBuffer.Height - 48, 0, 0 );
    TCHAR tszBuf[MAX_PATH];
    _stprintf( tszBuf, _T("%s\n%.2f\n%.1f"),
               m_bUseFloatCubeMap ? _T("A16B16G16R16F ( D3D9 / HDR )") : _T("A8R8G8B8 ( D3D8 )"),
               m_fReflectivity, m_vLightIntensity.x );
    m_pFontSmall->DrawText( NULL,
                            tszBuf,
                            -1, &rc, DT_NOCLIP, HELPTEXTCOLOR );


    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Overrrides the main WndProc, so the sample can do custom message
//       handling (e.g. processing mouse, keyboard, or menu commands).
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam )
{
    m_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    switch( uMsg )
    {
        //
        // Use WM_CHAR to handle parameter adjustment so
        // that we can control the granularity based on
        // the letter cases.
        case WM_CHAR:
        {
            switch( wParam )
            {
                case 'W':
                case 'w':
                    if( 'w' == wParam )
                        m_vLightIntensity += D3DXVECTOR4( 0.1f, 0.1f, 0.1f, 0.0f );
                    else
                        m_vLightIntensity += D3DXVECTOR4( 10.0f, 10.0f, 10.0f, 0.0f );
                    m_pEffect->SetVector( m_hLightIntensity, &m_vLightIntensity );
                    break;

                case 'S':
                case 's':
                    if( 's' == wParam )
                        m_vLightIntensity -= D3DXVECTOR4( 0.1f, 0.1f, 0.1f, 0.0f );
                    else
                        m_vLightIntensity -= D3DXVECTOR4( 10.0f, 10.0f, 10.0f, 0.0f );
                    if( m_vLightIntensity.x < 0.0f )
                        m_vLightIntensity.x =
                        m_vLightIntensity.y =
                        m_vLightIntensity.z = 0.0f;
                    m_pEffect->SetVector( m_hLightIntensity, &m_vLightIntensity );
                    break;

                case 'D':
                case 'd':
                    if( 'd' == wParam)
                        m_fReflectivity -= 0.01f;
                    else
                        m_fReflectivity -= 0.1f;
                    if( m_fReflectivity < 0 )
                        m_fReflectivity = 0;
                    m_pEffect->SetFloat( m_hReflectivity, m_fReflectivity );
                    break;

                case 'E':
                case 'e':
                    if( 'e' == wParam )
                        m_fReflectivity += 0.01f;
                    else
                        m_fReflectivity += 0.1f;
                    if( m_fReflectivity > 1.0f )
                        m_fReflectivity = 1.0f;
                    m_pEffect->SetFloat( m_hReflectivity, m_fReflectivity );
                    break;
            }

            return 0;
        }

        case WM_KEYDOWN:
        {
            switch( wParam )
            {
                case 'N':
                    if( ++m_nCurrMesh == NUM_MESHES )
                        m_nCurrMesh = 0;
                    break;

                case 'F':
                    m_bUseFloatCubeMap = !m_bUseFloatCubeMap;
                    break;
            }

            return 0;
        }

        case WM_COMMAND:
        {
            switch( LOWORD(wParam) )
            {
                case IDM_TOGGLEHELP:
                    m_bShowHelp = !m_bShowHelp;
                    break;

                case IDM_RESETPARAMS:
                    ResetParameters();
                    break;
            }
        }
    }

    return CD3DApplication::MsgProc( hWnd, uMsg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Invalidates device objects.  Paired with RestoreDeviceObjects()
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    int i;

    m_pFont->OnLostDevice();
    m_pFontSmall->OnLostDevice();

    for( i = 0; i < NUM_MESHES; ++i )
        m_EnvMesh[i].m_Mesh.InvalidateDeviceObjects();
    m_pRoom->InvalidateDeviceObjects();
    m_pLightMesh->InvalidateDeviceObjects();
    for( i = 0; i < sizeof(m_Orbit) / sizeof(m_Orbit[0]); ++i )
        m_Orbit[i].m_Mesh.InvalidateDeviceObjects();

    m_pEffect->OnLostDevice();

    SAFE_RELEASE( m_pCubeMapFp );
    SAFE_RELEASE( m_pCubeMap32 );
    SAFE_RELEASE( m_pDepthCube );

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
    int i;

    for( i = 0; i < NUM_MESHES; ++i )
        m_EnvMesh[i].m_Mesh.Destroy();
    SAFE_RELEASE( m_pFont );
    SAFE_RELEASE( m_pFontSmall );
    SAFE_RELEASE( m_pVertDecl );

    m_pRoom->Destroy();
    m_pLightMesh->Destroy();
    for( i = 0; i < sizeof(m_Orbit) / sizeof(m_Orbit[0]); ++i )
        m_Orbit[i].m_Mesh.Destroy();

    SAFE_RELEASE( m_pEffect );

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
    delete m_pRoom; m_pRoom = NULL;
    delete m_pLightMesh; m_pLightMesh = NULL;

    return S_OK;
}
