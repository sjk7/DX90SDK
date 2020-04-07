//-----------------------------------------------------------------------------
// File: SHPRTPixel.cpp
//
// Desc: This sample demonstrates how use D3DXSHPRTSimulationTex(), a per texel 
//       precomputed radiance transfer (PRT) simulator that uses low-order 
//       spherical harmonics (SH) and records the results to a file. The sample 
//       then demonstrates how compress the results with principal 
//       component analysis (PCA) and views the compressed results with arbitrary 
//       lighting in real time with a ps_2_0 pixel shader
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "dxstdafx.h"
#include <dxerr9.h>
#include "SHPRTPixel.h"
#include "resource.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders 
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders 




//-----------------------------------------------------------------------------
// Globals variables and definitions
//-----------------------------------------------------------------------------
//#define DEBUG_VS   // uncomment this line to debug the vertex shaders
//#define DEBUG_PS   // uncomment this line to debug the pixel shaders
const float g_fSqrtPI = ((float)sqrtf(D3DX_PI));
HINSTANCE g_hInst;

// Subsurface scattering parameters from:
// "A Practical Model for Subsurface Light Transport", 
// Henrik Wann Jensen, Steve R. Marschner, Marc Levoy, Pat Hanrahan.
// SIGGRAPH 2001
const PREDEFINED_MATERIALS g_aPredefinedMaterials[] = 
{
    // name             scattering (R/G/B/A)            absorption (R/G/B/A)                    reflectance (R/G/B/A)           index of refraction
    TEXT("Default"),    {2.00f, 2.00f, 2.00f, 1.0f},    {0.0030f, 0.0030f, 0.0460f, 1.0f},      {1.00f, 1.00f, 1.00f, 1.0f},    1.3f,
    TEXT("Apple"),      {2.29f, 2.39f, 1.97f, 1.0f},    {0.0030f, 0.0030f, 0.0460f, 1.0f},      {0.85f, 0.84f, 0.53f, 1.0f},    1.3f,
    TEXT("Chicken1"),   {0.15f, 0.21f, 0.38f, 1.0f},    {0.0150f, 0.0770f, 0.1900f, 1.0f},      {0.31f, 0.15f, 0.10f, 1.0f},    1.3f,
    TEXT("Chicken2"),   {0.19f, 0.25f, 0.32f, 1.0f},    {0.0180f, 0.0880f, 0.2000f, 1.0f},      {0.32f, 0.16f, 0.10f, 1.0f},    1.3f,
    TEXT("Cream"),      {7.38f, 5.47f, 3.15f, 1.0f},    {0.0002f, 0.0028f, 0.0163f, 1.0f},      {0.98f, 0.90f, 0.73f, 1.0f},    1.3f,
    TEXT("Ketchup"),    {0.18f, 0.07f, 0.03f, 1.0f},    {0.0610f, 0.9700f, 1.4500f, 1.0f},      {0.16f, 0.01f, 0.00f, 1.0f},    1.3f,
    TEXT("Marble"),     {2.19f, 2.62f, 3.00f, 1.0f},    {0.0021f, 0.0041f, 0.0071f, 1.0f},      {0.83f, 0.79f, 0.75f, 1.0f},    1.5f,
    TEXT("Potato"),     {0.68f, 0.70f, 0.55f, 1.0f},    {0.0024f, 0.0090f, 0.1200f, 1.0f},      {0.77f, 0.62f, 0.21f, 1.0f},    1.3f,
    TEXT("Skimmilk"),   {0.70f, 1.22f, 1.90f, 1.0f},    {0.0014f, 0.0025f, 0.0142f, 1.0f},      {0.81f, 0.81f, 0.69f, 1.0f},    1.3f,
    TEXT("Skin1"),      {0.74f, 0.88f, 1.01f, 1.0f},    {0.0320f, 0.1700f, 0.4800f, 1.0f},      {0.44f, 0.22f, 0.13f, 1.0f},    1.3f,
    TEXT("Skin2"),      {1.09f, 1.59f, 1.79f, 1.0f},    {0.0130f, 0.0700f, 0.1450f, 1.0f},      {0.63f, 0.44f, 0.34f, 1.0f},    1.3f,
    TEXT("Spectralon"),{11.60f,20.40f,14.90f, 1.0f},    {0.0000f, 0.0000f, 0.0000f, 1.0f},      {1.00f, 1.00f, 1.00f, 1.0f},    1.3f,
    TEXT("Wholemilk"),  {2.55f, 3.21f, 3.77f, 1.0f},    {0.0011f, 0.0024f, 0.0140f, 1.0f},      {0.91f, 0.88f, 0.76f, 1.0f},    1.3f,
    TEXT("Custom"),     {0.00f, 0.00f, 0.00f, 1.0f},    {0.0000f, 0.0000f, 0.0000f, 1.0f},      {0.00f, 0.00f, 0.00f, 1.0f},    0.0f,
};
const int g_aPredefinedMaterialsSize = sizeof(g_aPredefinedMaterials) / sizeof(g_aPredefinedMaterials[0]);
CMyD3DApplication g_d3dApp;



//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    InitCommonControls();

    g_hInst = hInst;

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
    m_strWindowTitle = _T("SHPRTPixel");
    m_d3dEnumeration.AppUsesDepthBuffer = TRUE;
    m_pFont = NULL;
    m_bCreateMultithreadDevice = true;
    m_bShowCursorWhenFullscreen = true;
    m_nTechnique = 0;
    m_dwCreationWidth  = 640;
    m_dwCreationHeight = 480;
    m_bWireFrame = false;
    m_fObjectRadius = 1.0f;
    m_vObjectCenter = D3DXVECTOR3(0,0,0);

    m_bPRTSimulationComplete = false;
    m_hPRTSimulationThreadId = NULL;
    m_dwPRTSimulationThreadId = 0;
    m_bSHPRTReadyToRender = false;

    m_bShowHelp = true;
    m_bStopSimulator = false;
    m_fPercentDone = 0.0f;

    m_pSHPRTEffect = NULL;
    m_pSimpleLightingEffect = NULL;

    m_pSphere = NULL;
    m_pMesh = NULL;
    m_pSHPRTBuffer = NULL;
    m_aClusterBases = NULL;
    m_aClusteredPCA = NULL;
    ZeroMemory( m_pPCAWeightTexture, sizeof(m_pPCAWeightTexture) );

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
    m_nNumActiveLights = 1;
    m_fLightScale = 1.0f;

    // Read persistent state information from registry
    // and if regkey is missing then set to default values
    HKEY hRegKey;
    DWORD dwResult;
    RegCreateKeyEx( HKEY_CURRENT_USER, SAMPLE_REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL,  &hRegKey, NULL );        
    TCHAR strMedia[MAX_PATH];
    DXUtil_GetDXSDKMediaPathCch( strMedia, MAX_PATH );
    DXUtil_ReadStringRegKeyCch( hRegKey, TEXT("InitialDir"), m_strInitialDir, MAX_PATH, strMedia );
    DXUtil_ReadStringRegKeyCch( hRegKey, TEXT("InputMesh"), m_strInputMesh, MAX_PATH, TEXT("shapes1.x") );
    DXUtil_ReadStringRegKeyCch( hRegKey, TEXT("ResultsFile"), m_strResultsFile, MAX_PATH, TEXT("shapes1_results.dat"));
    DXUtil_ReadIntRegKey( hRegKey, TEXT("Order"), &m_dwOrder, 6 );
    DXUtil_ReadIntRegKey( hRegKey, TEXT("TextureSize"), &m_dwTextureSize, 256 );    
    DXUtil_ReadIntRegKey( hRegKey, TEXT("TextureFormat"), &dwResult, (DWORD) D3DFMT_Q16W16V16U16 ); m_fmtTexture = (D3DFORMAT) dwResult;   
    DXUtil_ReadIntRegKey( hRegKey, TEXT("NumRays"), &m_dwNumRays, 256 );
    DXUtil_ReadIntRegKey( hRegKey, TEXT("NumBounces"), &m_dwNumBounces, 0 );
    DXUtil_ReadIntRegKey( hRegKey, TEXT("NumPCAVectors"), &m_dwNumPCAVectors, MAX_PCA_VECTORS );
    DXUtil_ReadIntRegKey( hRegKey, TEXT("SubsurfaceScattering"), &dwResult, 0 ); m_bSubsurfaceScattering = (dwResult != 0);
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("LengthScale"), &m_fLengthScale, 25.0f ); 
    DXUtil_ReadIntRegKey( hRegKey, TEXT("Spectral"), &dwResult, 1 ); m_bSpectral = (dwResult != 0);
    DXUtil_ReadIntRegKey( hRegKey, TEXT("PredefinedMatIndex"), &m_dwPredefinedMatIndex, 0 );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("RelativeIndexOfRefraction"), &m_fRelativeIndexOfRefraction, g_aPredefinedMaterials[0].fRelativeIndexOfRefraction );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("DiffuseReflectanceR"), &m_DiffuseReflectance.r, g_aPredefinedMaterials[0].fDiffuseReflectance.r );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("DiffuseReflectanceG"), &m_DiffuseReflectance.g, g_aPredefinedMaterials[0].fDiffuseReflectance.g );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("DiffuseReflectanceB"), &m_DiffuseReflectance.b, g_aPredefinedMaterials[0].fDiffuseReflectance.b );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("AbsoptionR"), &m_Absoption.r, g_aPredefinedMaterials[0].fAbsorptionCoefficient.r );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("AbsoptionG"), &m_Absoption.g, g_aPredefinedMaterials[0].fAbsorptionCoefficient.g );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("AbsoptionB"), &m_Absoption.b, g_aPredefinedMaterials[0].fAbsorptionCoefficient.b );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("ReducedScatteringR"), &m_ReducedScattering.r, g_aPredefinedMaterials[0].fScatteringCoefficient.r );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("ReducedScatteringG"), &m_ReducedScattering.g, g_aPredefinedMaterials[0].fScatteringCoefficient.g );
    DXUtil_ReadFloatRegKey( hRegKey, TEXT("ReducedScatteringB"), &m_ReducedScattering.b, g_aPredefinedMaterials[0].fScatteringCoefficient.b );

    RegCloseKey( hRegKey );
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
    InitializeCriticalSection( &m_cs );

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
    TCHAR str[MAX_PATH];
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("SimpleLighting.fx") );
    hr = D3DXCreateEffectFromFile( m_pd3dDevice, str, NULL, NULL, 
                                   dwShaderFlags, NULL, &m_pSimpleLightingEffect, NULL );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXCreateEffectFromFile"), hr );

    // This is code occurs when device is lost and the sample previously
    // viewing a mesh with PRT 
    if( m_bSHPRTReadyToRender )
    {
        hr = LoadMeshAndResults();
        if( FAILED( hr ) )
            return hr;

        // If this fails, there should be debug output as to 
        // they the .fx file failed to compile
        hr = LoadSHPRTEffect();
        if( FAILED(hr) )    
            return DXTRACE_ERR( TEXT("LoadSHPRTEffect"), hr );

    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DoStartupDlg()
// Desc: Allows the user to select to either run the simulator or 
//       load previously saved simulator results.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DoStartupDlg()
{
    HRESULT hr;

    Pause(true);
    int nResult = (int) DialogBox( g_hInst, MAKEINTRESOURCE(IDD_STARTUP), m_hWnd, StaticStartupDlgProc );
    Pause(false);
    if( nResult == IDC_RUN_PRT_SIM )
    {
        // Ask the user about PRT simulator settings and launch
        // the PRT simulator in another thread to not block the UI thread
        hr = LaunchPRTSimulation();
        if( FAILED(hr) )    
            return DXTRACE_ERR( TEXT("LaunchPRTSimulation"), hr );
    } 
    else if( nResult == IDC_VIEW_RESULTS )
    {
        hr = LoadMeshAndResults();
        if( FAILED(hr) )
            return hr;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DoCompressDlg()
// Desc: Allows the user to change the # of PCA vectors 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DoCompressDlg()
{
    HRESULT hr;

    if( !m_bWindowed )
    {
        // Go back to windowed mode to display dialog boxes
        if( FAILED( hr = ToggleFullscreen() ) )
            return DXTRACE_ERR( TEXT("ToggleFullscreen"), hr );
    }

    Pause(true);
    int nResult = (int) DialogBox( g_hInst, MAKEINTRESOURCE(IDD_COMPRESS), m_hWnd, StaticCompressDlgProc );
    Pause(false);

    if( nResult == IDOK && m_hPRTSimulationThreadId == NULL )
    {
        hr = CompressData();
        if( FAILED(hr) )
            MessageBox( m_hWnd, TEXT("Loading and compressing data failed\n"), TEXT("Error"), MB_OK );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ViewResults()
// Desc: This function stops the simulator if its running, then loads the mesh, 
//       loads the simulator results, compresses the simulator results.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::LoadMeshAndResults()
{
    HRESULT hr;
    int nResult;

    if( m_hPRTSimulationThreadId )
    {
        // Ask to stop the PRT simulator if it's running in the other thread
        DWORD dwResult = WaitForSingleObject( m_hPRTSimulationThreadId, 0 );
        if( dwResult == WAIT_TIMEOUT )
        {
            nResult = MessageBox( m_hWnd, TEXT("Simulation still processing.  Abort current simulation?"), TEXT("Warning"), MB_YESNO|MB_ICONWARNING );
            if( nResult == IDNO )
                return S_OK;

            if( FAILED( hr = StopPRTSimulation() ) )
                return hr;
        }
    }

    SAFE_RELEASE( m_pMesh );
    SAFE_RELEASE( m_pSHPRTBuffer );

    // Load the mesh, and ensure it has a known decl
    hr = LoadMesh( m_strInputMesh, &m_pMesh );
    if( FAILED(hr) )    
    {
        MessageBox( m_hWnd, TEXT("Couldn't load mesh"), TEXT("Error"), MB_OK );
        return DXTRACE_ERR( TEXT("LoadMesh"), hr );
    }

    m_bSHPRTReadyToRender = false;

    // Load the previously saved SH PRT buffer from a file
    if( FAILED( hr = ReadSHPRTBufferFromFile( m_strResultsFile, &m_pSHPRTBuffer ) ) )
    {
        MessageBox( m_hWnd, TEXT("Couldn't load simulator results file.  Run the simulator first to precompute the transfer vectors for this mesh."), TEXT("Error"), MB_OK );
        return DXTRACE_ERR( TEXT("ReadSHPRTBufferFromFile"), hr );
    }

    // Compress the saved PRT simulator results 
    hr = CompressData();
    if( FAILED(hr) )    
        return DXTRACE_ERR( TEXT("CompressData"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CompressData()
// Desc: This sets up the PCA data needed by the pixel shader
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::CompressData()
{
    HRESULT hr;

    // Ensure correct state 
    if( m_pSHPRTBuffer == NULL || 
        m_pMesh == NULL || 
        m_hPRTSimulationThreadId != NULL )
    {
        return E_FAIL;
    }

    // Get the description of the SH PRT buffer
    D3DXSHPRTBUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(D3DXSHPRTBUFFER_DESC) );
    hr = D3DXSHPRTExtractDesc( m_pSHPRTBuffer, &desc );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXSHPRTExtractDesc"), hr );

    // Make note of the order used & if its spectral data
    m_dwOrder = desc.Order;
    m_bSpectral = (desc.NumChannels == 3) ? true : false;
    assert( desc.Height == desc.Width ); // sample requires this
    m_dwTextureSize = desc.Height;

    // Compress the results buffer using 1 cluster, and dwNumPCAVectors PCA vectors.  
    // pPCABuffer will contain the compressed buffer.
    ID3DXBuffer* pPCABuffer = NULL;
    hr = D3DXSHPRTCompress( desc.Order, m_pSHPRTBuffer, 
                            D3DXSHCQUAL_FASTLOWQUALITY, 1, m_dwNumPCAVectors, 
                            &pPCABuffer );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXSHPRTCompress"), hr );

    // Normalize the the compressed data.  This adjusts the mean vectors so 
    // that the weights are between 0-1
    hr = D3DXSHPRTCompNormalizeData( pPCABuffer );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXSHPRTCompNormalizeData"), hr );

    // Setup the textures with PCA data
    hr = SetupTexWithPCAData( m_pMesh, pPCABuffer, desc, m_dwNumPCAVectors );
    if( FAILED(hr) )    
        return DXTRACE_ERR( TEXT("SetupVBWithPCAData"), hr );

    // Extract the cluster bases into a large array of floats.  
    // D3DXSHPRTCompExtractBasis will extract the basis 
    // for a single cluster.  The basis for a cluster is an array of
    // (NumPCAVectors+1)*(NumChannels*Order^2) floats.  
    // The "1+" is for the cluster mean.
    int nChannelSize   = desc.Order * desc.Order; 
    int nPCAVectorSize = nChannelSize * desc.NumChannels;  
    int nBasisSize     = nPCAVectorSize * (m_dwNumPCAVectors+1);  
    int nBufferSize    = nBasisSize * 1; 

    SAFE_DELETE_ARRAY( m_aClusterBases );
    m_aClusterBases = new float[nBufferSize];
    if( m_aClusterBases == NULL )
        return DXTRACE_ERR( TEXT("new"), E_OUTOFMEMORY );

    // extract the basis of the cluster 
    hr = D3DXSHPRTCompExtractBasis( 0, pPCABuffer, m_aClusterBases );
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXSHPRTCompExtractBasis"), hr );

    SAFE_DELETE_ARRAY( m_aClusteredPCA );
    m_aClusteredPCA = new float[1*(4+MAX_NUM_CHANNELS*m_dwNumPCAVectors)];
    if( m_aClusteredPCA == NULL )
        return DXTRACE_ERR( TEXT("new"), E_OUTOFMEMORY );

    // Cleanup and allow rendering using the SH PRT data
    SAFE_RELEASE( pPCABuffer );

    m_bSHPRTReadyToRender = true;

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    hr = LoadSHPRTEffect();
    if( FAILED(hr) )    
        return DXTRACE_ERR( TEXT("LoadSHPRTEffect"), hr );

    // Evaluate the lights in terms of SH coefficients 
    // and set the shader constants, and call this function
    // whenever the lights or object rotates and need to be reevaluated
    UpdateLightDir();
    if( FAILED( hr = EvalLightsAndSetConstants() ) )
        return hr;

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
    DWORD m_dwNumMaterials = 0;
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
                           &m_dwNumMaterials, &pMesh);
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXLoadMeshFromX"), hr );

    // Make sure there are texture coordinates which are required for texturing
    if( ((pMesh->GetFVF() & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) == 0 )
    {
        SAFE_RELEASE( m_pMesh );
        return E_FAIL;
    }

    // Lock the vertex buffer to get the object's radius & center
    // simply to help position the camera a good distance away from the mesh.
    IDirect3DVertexBuffer9* pVB = NULL;
    void* pVertices;
    hr = pMesh->GetVertexBuffer( &pVB );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("GetVertexBuffer"), hr );
    hr = pVB->Lock( 0, 0, &pVertices, 0 );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("Lock"), hr );
    hr = D3DXComputeBoundingSphere( (D3DXVECTOR3*)pVertices, pMesh->GetNumVertices(), 
                                    D3DXGetFVFVertexSize(pMesh->GetFVF()), &m_vObjectCenter, 
                                    &m_fObjectRadius );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXComputeBoundingSphere"), hr );
    pVB->Unlock();
    SAFE_RELEASE( pVB );

    // Update camera's viewing radius based on the object radius
    m_Camera.SetRadius( m_fObjectRadius*3.0f, m_fObjectRadius*1.1f, m_fObjectRadius*20.0f );
    m_Camera.SetModelCenter( m_vObjectCenter );

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
    // cache hit more often so it won't have to re-execute the vertex 4 
    // on those vertices so it will improves perf.     
    DWORD *rgdwAdjacency = new DWORD[pMesh->GetNumFaces() * 3];
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
// Name: SetupTexWithPCAData()
// Desc: Create and fill textures with PCA data
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::SetupTexWithPCAData( ID3DXMesh* pMesh,
                                                ID3DXBuffer* pPCABuffer, 
                                                D3DXSHPRTBUFFER_DESC desc,
                                                DWORD dwNumPCAVectors )
{
    HRESULT hr;

    // Each texture can how four PCA weights
    for( DWORD i=0; i<m_dwNumPCAVectors/4; i++ )
    {
        SAFE_RELEASE( m_pPCAWeightTexture[i] );

        // Create the texture with complete mipmapping
        hr = D3DXCreateTexture( m_pd3dDevice, m_dwTextureSize, m_dwTextureSize, D3DX_DEFAULT, 0,
                                m_fmtTexture, D3DPOOL_MANAGED, &m_pPCAWeightTexture[i] );
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("D3DXCreateTexture"), hr );

        // Fill the texture with 4 PCA weights starting at PCA weight i*4.
        hr = D3DXSHPRTCompExtractTexture( i*4, 4, pPCABuffer, m_pPCAWeightTexture[i] );
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("D3DXSHPRTCompExtractTexture"), hr );
    
        // Filter the PCA weights to rest of the mipmap levels
        hr = D3DXFilterTexture( m_pPCAWeightTexture[i], NULL, D3DX_DEFAULT, D3DX_DEFAULT );
        if( FAILED( hr ) )
            return DXTRACE_ERR( TEXT("D3DXFilterTexture"), hr );

        // Set the sampler state
        m_pd3dDevice->SetTexture( i, m_pPCAWeightTexture[i] );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
        m_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: LoadSHPRTEffect()
// Desc: Load the SHPRTPixel.fx effect file
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::LoadSHPRTEffect()
{
    HRESULT hr;

    // The number of pixel shader consts need by the shader can't exceed the 
    // amount the HW can support
    DWORD dwNumPConsts = 1 * (1 + MAX_NUM_CHANNELS*m_dwNumPCAVectors/4) + 4;
    if( dwNumPConsts > 32 )
        return E_FAIL;

    SAFE_RELEASE( m_pSHPRTEffect );

    D3DXMACRO aDefines[2];
    CHAR szMaxNumPCAVectors[64];
    _snprintf( szMaxNumPCAVectors, 64, "%d", m_dwNumPCAVectors );
    szMaxNumPCAVectors[63] = 0;
    aDefines[0].Name       = "NUM_PCA_VECTORS";
    aDefines[0].Definition = szMaxNumPCAVectors;
    aDefines[1].Name       = NULL;
    aDefines[1].Definition = NULL;

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
    TCHAR str[MAX_PATH];
    DXUtil_FindMediaFileCb( str, sizeof(str), TEXT("SHPRTPixel.fx") );
    hr = D3DXCreateEffectFromFile( m_pd3dDevice, str, aDefines, NULL, 
                                   dwShaderFlags, NULL, &m_pSHPRTEffect, NULL );

    // If this fails, there should be debug output as to 
    // they the .fx file failed to compile
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("D3DXCreateEffectFromFile"), hr );

    // Make sure the technique works on this card
    hr = m_pSHPRTEffect->ValidateTechnique( "PrecomputedSHLighting" );
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("ValidateTechnique"), hr );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: EvalLightsAndSetConstants()
// Desc: Evaluate the lights in terms of SH coefficients 
//       and set the shader constants, and call this function
//       whenever the lights or object rotates and need to be reevaluated
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::EvalLightsAndSetConstants()
{
    HRESULT hr;

    if( !m_bSHPRTReadyToRender || 
        m_aClusterBases == NULL || 
        m_pMesh == NULL || 
        m_pSHPRTEffect == NULL )
        return S_OK;

    D3DXCOLOR lightColor(1.0f, 1.0f, 1.0f, 1.0f);        
    lightColor *= m_fLightScale;

    float fLight[MAX_LIGHTS][3][MAX_SH_ORDER*MAX_SH_ORDER];  

    // Pass in the light direction, the intensity of each channel, and it returns
    // the source radiance as an array of order^2 SH coefficients for each color channel.  
    D3DXVECTOR3 lightDirObjectSpace;
    D3DXMATRIX mWorldInv;
    D3DXMatrixInverse(&mWorldInv, NULL, &m_mWorld);

	int i;
    for( i=0; i<m_nNumActiveLights; i++ )
    {
        // Transform the world space light dir into object space
        // Note that if there's multiple objects using PRT in the scene, then
        // for each object you need to either evaulate the lights in object space
        // evaulate the lights in world and rotate the light coefficients 
        // into object space.
        D3DXVec3TransformNormal( &lightDirObjectSpace, &m_vLightDir[i], &mWorldInv );

        // This sample uses D3DXSHEvalDirectionalLight(), but there's other 
        // types of lights provided by D3DXSHEval*.  Pass in the 
        // order of SH, color of the light, and the direction of the light 
        // in object space.
        // The output is the source radiance coefficients for the SH basis functions.  
        // There are 3 outputs, one for each channel (R,G,B). 
        // Each output is an array of m_dwOrder^2 floats.  
        hr = D3DXSHEvalDirectionalLight( m_dwOrder, &lightDirObjectSpace, 
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
        D3DXSHAdd( fLight[0][0], m_dwOrder, fLight[0][0], fLight[i][0] );
        D3DXSHAdd( fLight[0][1], m_dwOrder, fLight[0][1], fLight[i][1] );
        D3DXSHAdd( fLight[0][2], m_dwOrder, fLight[0][2], fLight[i][2] );
    }

    // Setup the shader constants based on the source radiance coefficients and
    // the cluster basis.
    if( FAILED( hr = SetShaderConstants( fLight[0][0], fLight[0][1], fLight[0][2] ) ) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: SetShaderConstants()
// Desc: Setup the shader constants based on the source radiance 
//       coefficients and the cluster basis.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::SetShaderConstants( float* pfRLC, float* pfGLC, float* pfBLC )
{
    HRESULT hr;

    DWORD dwNumChannels = (m_bSpectral) ? 3 : 1;
    int nChannelSize    = m_dwOrder * m_dwOrder; 
    int nPCAVectorSize = nChannelSize * dwNumChannels;  
    int nBufferSize     = 1 * (4+MAX_NUM_CHANNELS*m_dwNumPCAVectors);
    int nChannelMult    = (m_bSpectral?1:0) * nChannelSize;

    // Compute an array of floats to pass as constants to the vertex shader.
    // This array is the source radiance dotted with the transfer function.
    // The source radiance is the lighting environment in terms of spherical
    // harmonic coefficients which comes from D3DXSHEvalDirectionalLight, and 
    // the transfer function are in terms of spherical harmonic basis coefficients 
    // that comes from the D3DXSHPRTCompExtractBasis().

    // First we compute L dot Mk, where Mk is the mean of the cluster
    m_aClusteredPCA[0] = D3DXSHDot( m_dwOrder, &m_aClusterBases[0*nChannelMult], pfRLC );
    m_aClusteredPCA[1] = D3DXSHDot( m_dwOrder, &m_aClusterBases[1*nChannelMult], pfGLC );
    m_aClusteredPCA[2] = D3DXSHDot( m_dwOrder, &m_aClusterBases[2*nChannelMult], pfBLC );
    m_aClusteredPCA[3] = 0.0f;

    // Then we compute L dot Bkj, where Bkj is the 
    // jth PCA basis vector for cluster k
    float* pPCAStart = &m_aClusteredPCA[4];
    for( DWORD iPCA = 0; iPCA < m_dwNumPCAVectors; iPCA++ ) 
    {
        int nOffset = (iPCA+1)*nPCAVectorSize;

        pPCAStart[0*m_dwNumPCAVectors + iPCA] = D3DXSHDot( m_dwOrder, &m_aClusterBases[nOffset + 0*nChannelMult], pfRLC );
        pPCAStart[1*m_dwNumPCAVectors + iPCA] = D3DXSHDot( m_dwOrder, &m_aClusterBases[nOffset + 1*nChannelMult], pfGLC );
        pPCAStart[2*m_dwNumPCAVectors + iPCA] = D3DXSHDot( m_dwOrder, &m_aClusterBases[nOffset + 2*nChannelMult], pfBLC );
    }

    hr = m_pSHPRTEffect->SetFloatArray( "vClusteredPCA", (float*)m_aClusteredPCA, nBufferSize );
    if( FAILED( hr ) )
        return DXTRACE_ERR( TEXT("SetFloatArray"), hr );

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

    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, (m_bWireFrame) ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

    for( int i=0; i<MAX_LIGHTS; i++ )
        m_LightArcBall[i].SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );

    // Setup the camera
    FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
    m_Camera.SetProjParams( D3DX_PI/4, fAspect, 1.0f, 10000.0f );
    m_Camera.SetWindow( m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height );
    m_Camera.SetButtonMasks( MOUSE_LEFT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );
    m_Camera.SetEnablePositionMovement( false );
    m_Camera.SetResetCursorAfterMove( false );

    hr = m_pSimpleLightingEffect->OnResetDevice();
    if( FAILED(hr) )    
        return DXTRACE_ERR( TEXT("OnResetDevice"), hr );

    // This is code occurs when device is reset and the sample previously
    // viewing a mesh with PRT 
    if( m_bSHPRTReadyToRender )
    {
        hr = m_pSHPRTEffect->OnResetDevice();
        if( FAILED(hr) )    
            return DXTRACE_ERR( TEXT("OnResetDevice"), hr );

        // Set the texture/sampler states
        for( DWORD i=0; i<m_dwNumPCAVectors/4; i++ )
        {
            // Set the sampler state
            m_pd3dDevice->SetTexture( i, m_pPCAWeightTexture[i] );
            m_pd3dDevice->SetSamplerState( i, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
            m_pd3dDevice->SetSamplerState( i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
            m_pd3dDevice->SetSamplerState( i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
            m_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
            m_pd3dDevice->SetSamplerState( i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
        }

        // Evaluate the lights in terms of SH coefficients 
        // and set the shader constants, and call this function
        // whenever the lights or object rotates and need to be reevaluated
        if( FAILED( hr = EvalLightsAndSetConstants() ) )
            return hr;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
    HRESULT hr;

    // Update the camera's postion based on user input 
    m_Camera.FrameMove( m_fElapsedTime );

    // Get the world, view, projection matrix from the camera class
    m_mWorld = *m_Camera.GetWorldMatrix();       
    m_mView  = *m_Camera.GetViewMatrix();       
    m_mProj  = *m_Camera.GetProjMatrix();       

    // Show some dialog boxes if the state is right
    if( FAILED( hr = UpdateUI() ) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: UpdateUI()
// Desc: Show some dialog boxes if the state is right
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::UpdateUI()
{
    // If this is the first time call, then show the startup dialog
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        DoStartupDlg();
        s_bFirstTime = false;
    }

    // Check to see if simulation is still running
    if( m_hPRTSimulationThreadId )
    {
        DWORD dwResult = WaitForSingleObject( m_hPRTSimulationThreadId, 0 );
        if( dwResult != WAIT_TIMEOUT ) 
        {
            // Simulation completed
            HMENU hMenu = GetSubMenu( GetMenu( m_hWnd ), 1 );
            EnableMenuItem( hMenu, IDM_ABORT_SIMULATION, MF_GRAYED  );
            DrawMenuBar( m_hWnd );

            m_nTechnique = 0;
            m_bPRTSimulationComplete = true;
            m_hPRTSimulationThreadId = NULL;
            m_dwPRTSimulationThreadId = 0;

            if( !m_bPRTSimulationFailed )
            {
                HRESULT hr = CompressData();
                if( FAILED(hr) )    
                    return DXTRACE_ERR( TEXT("CompressData"), hr );
            }
        }
        else
        {
            // Yield time to the other thread 
            Sleep(10);
        }
    }

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
    D3DXMATRIXA16 mWorldViewProjT;

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
        m_pSimpleLightingEffect->SetMatrix( "mWorldViewProjection", &mWorldViewProj);
        if( m_pSHPRTEffect )
            m_pSHPRTEffect->SetMatrix( "mWorldViewProjection", &mWorldViewProj);

        // Render the scene using either N.L lighting or PRT 
        switch( m_nTechnique )
        {
            case 0: 
                if( FAILED( hr = RenderMeshWithSHPRT() ) )
                    return hr;
                break;
            case 1: 
                if( FAILED( hr = RenderMeshWithSimpleLighting() ) )
                    return hr;
                break;
        }

        // Render the text
        RenderText();

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

    m_pSimpleLightingEffect->SetTechnique( "SimpleLighting" );

    D3DXCOLOR lightOff(0,0,0,0);
    m_pSimpleLightingEffect->SetVector( "Light1DiffuseColor", (D3DXVECTOR4*)&lightOff );
    m_pSimpleLightingEffect->SetVector( "Light2DiffuseColor", (D3DXVECTOR4*)&lightOff );
    m_pSimpleLightingEffect->SetVector( "Light3DiffuseColor", (D3DXVECTOR4*)&lightOff );
    m_pSimpleLightingEffect->SetVector( "MaterialAmbientColor", (D3DXVECTOR4*)&sphereColor );

    D3DXVECTOR3 vL = lightDir * m_fObjectRadius * 1.2f;
    D3DXMatrixTranslation( &mTrans, vL.x, vL.y, vL.z );
    D3DXMatrixScaling( &mScale, m_fObjectRadius*0.05f, m_fObjectRadius*0.05f, m_fObjectRadius*0.05f );

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
// Name: RenderMeshWithSHPRT()
// Desc: Render the scene with per pixel PRT 
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RenderMeshWithSHPRT()
{
    HRESULT hr;
    UINT iPass, cPasses;

    if( !m_bSHPRTReadyToRender || m_aClusterBases == NULL || m_pMesh == NULL || m_pSHPRTEffect == NULL )
        return S_OK;

    // Set the technique and render the scene
    m_pSHPRTEffect->SetTechnique( "PrecomputedSHLighting" );

    if( FAILED( hr = m_pSHPRTEffect->Begin(&cPasses, 0) ) )
        return DXTRACE_ERR( TEXT("m_pSHPRTEffect->Begin"), hr );
    for (iPass = 0; iPass < cPasses; iPass++)
    {
        if( FAILED( hr = m_pSHPRTEffect->Pass(iPass) ) )
            return DXTRACE_ERR( TEXT("m_pSHPRTEffect->Pass"), hr );

        DWORD dwAttribs = 0;
        m_pMesh->GetAttributeTable( NULL, &dwAttribs );
        for( DWORD i=0; i<dwAttribs; i++ )
        {
            if( FAILED( hr = m_pMesh->DrawSubset(i) ) )
                return DXTRACE_ERR( TEXT("m_pMesh->DrawSubset"), hr );
        }
    }

    m_pSHPRTEffect->End();

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
    D3DXMATRIX mWorldViewT;
    D3DXVECTOR3 lightDirObjectSpace;
    D3DXCOLOR lightOn(1,1,1,1);
    D3DXCOLOR lightOff(0,0,0,0);

    lightOn *= m_fLightScale;

    if( m_pMesh == NULL )
        return S_OK;

    m_pSimpleLightingEffect->SetTechnique( "SimpleLighting" );

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
        case 0: lstrcpy( sz, TEXT("Technique: RenderMeshWithSHPRT") ); break;
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

    DWORD dwNumPConsts = 1 * (1 + MAX_NUM_CHANNELS*m_dwNumPCAVectors/4) + 4;
    _sntprintf( sz, 256, TEXT("# PCA vectors: %d, # Pixel Shader Constants (32 max): %d"), m_dwNumPCAVectors, dwNumPConsts );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    _sntprintf( sz, 256, TEXT("Texture Size: %d, Texture Format: %s"), m_dwTextureSize, D3DUtil_D3DFormatToString(m_fmtTexture,false) );
    SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
    hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("DrawText"), hr );

    if( m_hPRTSimulationThreadId )
    {
        float fPercentDone;
        // Keep 1 thread accessing the shared data at a time
        EnterCriticalSection( &m_cs );
        fPercentDone = m_fPercentDone;
        LeaveCriticalSection( &m_cs );

        _sntprintf( sz, 256, TEXT("PRT simulation in progress. %0.1f%% done.  It may take a while...\n"), fPercentDone*100.0f );
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );
    }
    else if( m_bPRTSimulationComplete && !m_bPRTSimulationFailed )
    {
        lstrcpy( sz, TEXT("PRT simulation complete!\n") );
        SetRect( &rc, 2, iLineY, 0, 0 ); iLineY += 15;
        hr = m_pFont->DrawText( NULL, sz, -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );
    }

    if( m_pMesh == NULL || (!m_bSHPRTReadyToRender && m_hPRTSimulationThreadId == NULL) )
    {
        lstrcpy( sz, TEXT("Use the PRT menu to load a mesh with simulator results") ); 
    }
    else
    {
        switch( m_nTechnique )
        {
            case 0: 
                if( m_bSHPRTReadyToRender )
                    lstrcpy( sz, TEXT("This is per pixel PRT.  Press 'N' to change to simple N.L lighting") ); 
                else
                    lstrcpy( sz, TEXT("Simulation in progress.  Press 'N' to change to simple lighting") ); 
                break;
            case 1: 
                if( m_bSHPRTReadyToRender )
                    lstrcpy( sz, TEXT("This is simple N.L lighting.  Press 'N' to change to per pixel PRT") ); 
                else
                    lstrcpy( sz, TEXT("This is simple N.L lighting.  SH PRT simulation in progress") ); 
                break;
        }
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
                                      _T("Zoom camera: Mouse wheel scroll\n")
                                      _T("Change # of active lights: L\n"),                                      
                                -1, &rc, DT_NOCLIP, D3DXCOLOR( 1.0f, 0.75f, 0.0f, 1.0f ) );
        if( FAILED(hr) )
            return DXTRACE_ERR( TEXT("DrawText"), hr );

        SetRect( &rc, 250, iLineY, 0, 0 ); 
        hr = m_pFont->DrawText( NULL, _T("Change active light: K\n")
                                      _T("Change light scale: U,I,O\n")
                                      _T("Compression settings: C,E,R\n")
                                      _T("Change render technique: N\n")
                                      _T("Wireframe toggle: W\n"),                               
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
    if( m_pSHPRTEffect )
        m_pSHPRTEffect->OnLostDevice();
    
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
    SAFE_RELEASE( m_pSphere );
    SAFE_RELEASE( m_pFont );
    SAFE_RELEASE( m_pMesh );
    SAFE_DELETE_ARRAY( m_aClusterBases );
    for( int i=0; i<MAX_PCA_VECTORS/4; i++ )
        SAFE_RELEASE( m_pPCAWeightTexture[i] )

    SAFE_RELEASE( m_pSimpleLightingEffect );
    SAFE_RELEASE( m_pSHPRTEffect );

    // Stop the PRT simulator if it's running in the other thread
    StopPRTSimulation();

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
    DeleteCriticalSection( &m_cs );
    SAFE_RELEASE( m_pSHPRTBuffer );

    // Write persistent state information from registry
    HKEY hRegKey;
    RegCreateKeyEx( HKEY_CURRENT_USER, SAMPLE_REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hRegKey, NULL );
    DXUtil_WriteIntRegKey( hRegKey, TEXT("Order"), m_dwOrder );
    DXUtil_WriteIntRegKey( hRegKey, TEXT("TextureSize"), m_dwTextureSize );    
    DXUtil_WriteIntRegKey( hRegKey, TEXT("TextureFormat"), (DWORD) m_fmtTexture );
    DXUtil_WriteIntRegKey( hRegKey, TEXT("NumRays"), m_dwNumRays );
    DXUtil_WriteIntRegKey( hRegKey, TEXT("NumBounces"), m_dwNumBounces );
    DXUtil_WriteIntRegKey( hRegKey, TEXT("NumPCAVectors"), m_dwNumPCAVectors );
    DXUtil_WriteIntRegKey( hRegKey, TEXT("SubsurfaceScattering"), m_bSubsurfaceScattering );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("LengthScale"), m_fLengthScale ); 
    DXUtil_WriteIntRegKey( hRegKey, TEXT("Spectral"), m_bSpectral ); 
    DXUtil_WriteIntRegKey( hRegKey, TEXT("PredefinedMatIndex"), m_dwPredefinedMatIndex );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("DiffuseReflectanceR"), m_DiffuseReflectance.r );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("DiffuseReflectanceG"), m_DiffuseReflectance.g );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("DiffuseReflectanceB"), m_DiffuseReflectance.b );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("AbsoptionR"), m_Absoption.r );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("AbsoptionG"), m_Absoption.g );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("AbsoptionB"), m_Absoption.b );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("ReducedScatteringR"), m_ReducedScattering.r );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("ReducedScatteringG"), m_ReducedScattering.g );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("ReducedScatteringB"), m_ReducedScattering.b );
    DXUtil_WriteFloatRegKey( hRegKey, TEXT("RelativeIndexOfRefraction"), m_fRelativeIndexOfRefraction );
    DXUtil_WriteStringRegKey( hRegKey, TEXT("InputMesh"), m_strInputMesh );
    DXUtil_WriteStringRegKey( hRegKey, TEXT("ResultsFile"), m_strResultsFile );
    DXUtil_WriteStringRegKey( hRegKey, TEXT("InitialDir"), m_strInitialDir );
    RegCloseKey( hRegKey );

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

    // If device doesn't support 1.1 vertex shaders in HW, then fail
    if( pCaps->VertexShaderVersion < D3DVS_VERSION(1,1) )
        return E_FAIL;

    // Need ps_2_0 for per pixel PRT
    if( pCaps->PixelShaderVersion < D3DPS_VERSION(2,0) )
        return E_FAIL;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: LaunchPRTSimulation()
// Desc: Get the settings from the user and launch the PRT simulator on another 
//       thread cause it'll likely take a while and the UI would be 
//       unresponsive otherwise
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::LaunchPRTSimulation()
{
    HRESULT hr;

    if( !m_bWindowed )
    {
        // Go back to windowed mode to display dialog boxes
        if( FAILED( hr = ToggleFullscreen() ) )
            return DXTRACE_ERR( TEXT("ToggleFullscreen"), hr );
    }

    if( m_hPRTSimulationThreadId )
    {
        // Ask to stop the PRT simulator if it's running in the other thread
        DWORD dwResult = WaitForSingleObject( m_hPRTSimulationThreadId, 0 );
        if( dwResult == WAIT_TIMEOUT )
        {
            int nResult = MessageBox( m_hWnd, TEXT("Simulation still processing.  Abort current simulation?"), TEXT("Warning"), MB_YESNO|MB_ICONWARNING );
            if( nResult == IDNO )
                return S_OK;

            if( FAILED( hr = StopPRTSimulation() ) )
                return hr;
        }
    }

    m_bStopSimulator = false;
    m_bPRTSimulationComplete = false;

    Pause(true);

    // Ask the user about param settings for the PRT Simulation
    int nResult = (int) DialogBox( NULL, MAKEINTRESOURCE(IDD_SIMULATION_OPTIONS), m_hWnd, StaticSimulationOptionsDlgProc );
    if( nResult == IDOK )
    {
        DialogBox( g_hInst, MAKEINTRESOURCE(IDD_COMPRESS), m_hWnd, StaticCompressDlgProc );

        m_bPRTSimulationFailed = false;
        m_bSHPRTReadyToRender = false;
        m_nTechnique = 1;

        // Launch the PRT simulator on another thread cause it'll 
        // likely take a while and the UI would be unresponsive otherwise
        m_hPRTSimulationThreadId = CreateThread( NULL, 0, StaticPRTSimulationThreadProc, 
                                                this, 0, &m_dwPRTSimulationThreadId );
    }

    HMENU hMenu = GetSubMenu( GetMenu( m_hWnd ), 1 );
    EnableMenuItem( hMenu, IDM_ABORT_SIMULATION, (m_hPRTSimulationThreadId != NULL) ? MF_ENABLED : MF_GRAYED  );

    Pause(false);
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: StopPRTSimulation()
// Desc: Set the m_bStopSimulator and wait for the simulator to close
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::StopPRTSimulation()
{
    if( m_hPRTSimulationThreadId )
    {
        DWORD dwResult = WaitForSingleObject( m_hPRTSimulationThreadId, 0 );
        if( dwResult == WAIT_TIMEOUT )
        {
            EnterCriticalSection( &m_cs );
            m_bStopSimulator = true;
            LeaveCriticalSection( &m_cs );

            // Wait for it to close
            dwResult = WaitForSingleObject( m_hPRTSimulationThreadId, 10000 );
            if( dwResult == WAIT_TIMEOUT )
                return E_FAIL;
            m_bStopSimulator = false;
            m_hPRTSimulationThreadId = NULL;
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: StaticPRTSimulationThreadProc
// Desc: static helper function
//-----------------------------------------------------------------------------
DWORD WINAPI CMyD3DApplication::StaticPRTSimulationThreadProc( LPVOID lpParameter )
{   
    return g_d3dApp.PRTSimulationThreadProc();
}




//-----------------------------------------------------------------------------
// Name: PRTSimulationThreadProc()
// Desc: Load the mesh and start the simluator and save the results to a file
//-----------------------------------------------------------------------------
DWORD CMyD3DApplication::PRTSimulationThreadProc()
{
    HRESULT hr;

    // Reset precent complete
    m_fPercentDone = 0.0f;

    // Cleanup any previous buffer
    SAFE_RELEASE( m_pMesh );
    SAFE_RELEASE( m_pSHPRTBuffer );

    // Load the mesh
    hr = LoadMesh( m_strInputMesh, &m_pMesh );
    if( FAILED(hr) )    
    {
        MessageBox( m_hWnd, TEXT("Couldn't load mesh.  Ensure that the mesh has texture coordinates and that they are in the [0, 1] range"), TEXT("Error"), MB_OK );
        DXTRACE_ERR( TEXT("LoadMesh"), hr );
        return 1;
    }

    // Note that the alpha value is ignored for the Diffuse, Absorption, 
    // and ReducedScattering parameters of the material.
    D3DXSHMATERIAL shMat;
    ZeroMemory( &shMat, sizeof(D3DXSHMATERIAL) );
    shMat.Diffuse = m_DiffuseReflectance;
    shMat.bMirror = false;
    shMat.bSubSurf = m_bSubsurfaceScattering;
    shMat.RelativeIndexOfRefraction = m_fRelativeIndexOfRefraction;
    shMat.Absorption = m_Absoption;
    shMat.ReducedScattering = m_ReducedScattering;

    // If the model has an albedo texture, then pass it to the simulator
    // This sample doesn't show off this functionality 
    LPDIRECT3DTEXTURE9 pAlbedoTexture = NULL;

    hr = D3DXSHPRTSimulationTex( m_dwOrder, m_pMesh, &shMat, 
                                 m_dwNumRays, m_dwNumBounces, m_bSubsurfaceScattering,
                                 m_fLengthScale, m_bSpectral, 
                                 &m_pSHPRTBuffer, m_dwTextureSize, m_dwTextureSize, 
                                 pAlbedoTexture, StaticSHPRTSimulatorCB );
    if( FAILED(hr) )       
    {
        if( hr == E_OUTOFMEMORY )
            MessageBox( m_hWnd, TEXT("The simulator ran out of memory.  Either reduce the texture size or increase your virtual memory."), TEXT("Error"), MB_OK );
        if( hr == D3DERR_INVALIDCALL )
            MessageBox( m_hWnd, TEXT("The simulator failed with D3DERR_INVALIDCALL.  Check the debug output for more information about why it failed."), TEXT("Error"), MB_OK );

        m_bPRTSimulationFailed = true;

        // This returns E_FAIL if the simulation was aborted from the callback
        if( hr == E_FAIL ) 
            return 1;

        DXTRACE_ERR( TEXT("D3DXSHPRTSimulation"), hr );
        return 1;
    }

    SaveSHPRTBufferToFile( m_pSHPRTBuffer, m_strResultsFile );

    return 1;
}



//-----------------------------------------------------------------------------
// Name: SaveSHPRTBufferToFile()
// Desc: Save a SH PRT buffer to a file
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::SaveSHPRTBufferToFile( ID3DXBuffer* pd3dxBuffer, TCHAR* strFile )
{
    // Create a file
    HANDLE hFile = CreateFile( strFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                               FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return DXTRACE_ERR( TEXT("CreateFile"), E_FAIL );

    // Get the buffer pointer & size
    VOID* pBuffer      = pd3dxBuffer->GetBufferPointer();
    DWORD dwBufferSize = pd3dxBuffer->GetBufferSize();

    // Write to the file
    DWORD dwWritten = 0;
    int nResult = WriteFile( hFile, pBuffer, dwBufferSize, &dwWritten, NULL );
    if( nResult == 0 || dwWritten != dwBufferSize )
    {
        CloseHandle( hFile );
        return DXTRACE_ERR( TEXT("WriteFile"), E_FAIL );
    }

    CloseHandle( hFile );
    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: ReadSHPRTBufferFromFile()
// Desc: Load the previously saved SH PRT buffer from a file
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ReadSHPRTBufferFromFile( TCHAR* strFile, ID3DXBuffer** ppd3dxBuffer )
{
    HRESULT hr;

    // Open the file
    HANDLE hFile = CreateFile( strFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 
                               FILE_ATTRIBUTE_NORMAL, NULL );
    if( hFile == INVALID_HANDLE_VALUE )
        return DXTRACE_ERR( TEXT("CreateFile"), E_FAIL );

    // Get the file size
    DWORD dwFileSize = GetFileSize( hFile, NULL );

    // Make an ID3DXBuffer
    hr = D3DXCreateBuffer( dwFileSize, ppd3dxBuffer );
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("D3DXCreateBuffer"), hr );

    VOID* pBuffer = (*ppd3dxBuffer)->GetBufferPointer();

    // Fill up the ID3DXBuffer
    DWORD dwRead = 0;
    int nResult = ReadFile( hFile, pBuffer, dwFileSize, &dwRead, NULL );
    if( nResult == 0 || dwRead != dwFileSize )
    {
        SAFE_RELEASE( *ppd3dxBuffer );
        CloseHandle( hFile );
        return DXTRACE_ERR( TEXT("ReadFile"), E_FAIL );
    }

    CloseHandle( hFile );

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: StaticSHPRTSimulatorCB()
// Desc: static helper function
//-----------------------------------------------------------------------------
HRESULT WINAPI CMyD3DApplication::StaticSHPRTSimulatorCB( float fPercentDone )
{
    return g_d3dApp.SHPRTSimulatorCB( fPercentDone );
}




//-----------------------------------------------------------------------------
// Name: SHPRTSimulatorCB()
// Desc: records the percent done and stops the simulator if requested
//-----------------------------------------------------------------------------
HRESULT WINAPI CMyD3DApplication::SHPRTSimulatorCB( float fPercentDone )
{
    // Keep only one thread accessing the shared data at a time
    EnterCriticalSection( &m_cs );

    m_fPercentDone = fPercentDone;

    HRESULT hr = S_OK;
    if( m_bStopSimulator )
        hr = E_FAIL; // return anything except S_OK will stop the simulator

    LeaveCriticalSection( &m_cs );

    return hr;
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

                case IDM_RUN_SIMULATION:
                case IDM_ABORT_SIMULATION:
                {
                    if( LOWORD(wParam) == IDM_RUN_SIMULATION )
                    {
                        // Ask the user about PRT simulator settings and launch
                        // the PRT simulator in another thread to not block the UI thread
                        LaunchPRTSimulation();
                    }
                    else
                    {
                        // Stop the PRT simulator if it's running in the other thread
                        HRESULT hr;
                        if( FAILED( hr = StopPRTSimulation() ) )
                            MessageBox( hWnd, TEXT("PRT simulator thread failed to stop\n"), TEXT("Error"), MB_OK );
                    }
                    
                    HMENU hMenu = GetSubMenu( GetMenu( hWnd ), 1 );
                    EnableMenuItem( hMenu, IDM_ABORT_SIMULATION, (m_hPRTSimulationThreadId != NULL) ? MF_ENABLED : MF_GRAYED  );
                    DrawMenuBar( hWnd );
                    break;
                }

                case IDM_PRT_VIEWRESULTS:
                {
                    DoStartupDlg();
                    break;
                }

                case IDM_PRT_COMPRESSIONSETTINGS:
                {
                    DoCompressDlg();
                    break;
                }
            }
        }

        case WM_KEYDOWN:
            switch( wParam )
            {
                case 'C': 
                    DoCompressDlg();
                    break;

                case 'N':
                    m_nTechnique++;
                    m_nTechnique %= 2;

                    if( m_nTechnique == 0 )
                        EvalLightsAndSetConstants();
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
                        EvalLightsAndSetConstants(); 
                    }
                    break;

                case 'I': m_fLightScale *= 1.25f; EvalLightsAndSetConstants(); break;
                case 'U': m_fLightScale *= 0.8f; EvalLightsAndSetConstants(); break;
                case 'O': m_fLightScale = 1.0f; EvalLightsAndSetConstants(); break;

                case 'W': m_bWireFrame = !m_bWireFrame; m_pd3dDevice->SetRenderState( D3DRS_FILLMODE, (m_bWireFrame) ? D3DFILL_WIREFRAME : D3DFILL_SOLID ); break;

                case 'E': m_dwNumPCAVectors += 4; if( m_dwNumPCAVectors > min(m_dwOrder*m_dwOrder,MAX_PCA_VECTORS) ) m_dwNumPCAVectors = min(m_dwOrder*m_dwOrder,MAX_PCA_VECTORS); CompressData(); break;
                case 'R': m_dwNumPCAVectors -= 4; if( m_dwNumPCAVectors < 4 ) m_dwNumPCAVectors = 4; CompressData(); break;
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
                EvalLightsAndSetConstants();
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
            EvalLightsAndSetConstants();
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





