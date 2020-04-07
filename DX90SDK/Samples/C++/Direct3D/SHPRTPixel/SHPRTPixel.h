//-----------------------------------------------------------------------------
// File: SHPRTPixel.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Globals variables and definitions
//-----------------------------------------------------------------------------
#define SAMPLE_REGKEY     TEXT("Software\\Microsoft\\DirectX 9.0 SDK\\SHPRTPixel")
#define MAX_PCA_VECTORS      24
#define MAX_SH_ORDER         6
#define MAX_LIGHTS           3

#define MAX_NUM_CHANNELS     3
extern const float g_fSqrtPI;
extern HINSTANCE g_hInst;

// Struct to store material params
struct PREDEFINED_MATERIALS
{
    const TCHAR* strName;
    D3DCOLORVALUE fScatteringCoefficient;
    D3DCOLORVALUE fAbsorptionCoefficient;
    D3DCOLORVALUE fDiffuseReflectance;
    float fRelativeIndexOfRefraction;
};

extern const PREDEFINED_MATERIALS g_aPredefinedMaterials[];
extern const int g_aPredefinedMaterialsSize;


//-----------------------------------------------------------------------------
// Name: class CMyD3DApplication
// Desc: Main class to run this application. Most functionality is inherited
//       from the CD3DApplication base class.
//-----------------------------------------------------------------------------
class CMyD3DApplication : public CD3DApplication
{
private:
    ID3DXFont*              m_pFont;             
    bool                    m_bShowHelp;
    CModelViewerCamera      m_Camera;
    LPD3DXEFFECT            m_pSHPRTEffect;
    LPD3DXEFFECT            m_pSimpleLightingEffect;
    CRITICAL_SECTION        m_cs;
    BOOL                    m_bWireFrame;
    BOOL                    m_bSHPRTReadyToRender;

    LPD3DXMESH              m_pMesh;
    ID3DXBuffer*            m_pSHPRTBuffer;
    IDirect3DTexture9*      m_pPCAWeightTexture[MAX_PCA_VECTORS/4];
    float                   m_fObjectRadius;
    D3DXVECTOR3             m_vObjectCenter;

    D3DXMATRIXA16           m_mWorld;
    D3DXMATRIXA16           m_mView;
    D3DXMATRIXA16           m_mProj;

    int                     m_nTechnique;
    ID3DXMesh*              m_pSphere;

    CD3DArcBall             m_LightArcBall[MAX_LIGHTS];
    D3DXVECTOR3             m_vDefaultLightDir[MAX_LIGHTS];
    D3DXVECTOR3             m_vLightDir[MAX_LIGHTS];
    D3DXMATRIXA16           m_mLightRot[MAX_LIGHTS];
    D3DXMATRIXA16           m_mLightRotSnapshot[MAX_LIGHTS];
    float                   m_fLightScale;
    int                     m_nNumActiveLights;
    int                     m_nActiveLight;

    bool                    m_bPRTSimulationComplete;
    bool                    m_bPRTSimulationFailed;
    HANDLE                  m_hPRTSimulationThreadId;
    DWORD                   m_dwPRTSimulationThreadId;
    float                   m_bStopSimulator;
    float                   m_fPercentDone;

    // Simulation options
    TCHAR                   m_strInitialDir[MAX_PATH];
    TCHAR                   m_strInputMesh[MAX_PATH];
    DWORD                   m_dwOrder;
    DWORD                   m_dwTextureSize;
    DWORD                   m_dwNumRays;
    DWORD                   m_dwNumBounces;
    DWORD                   m_dwNumPCAVectors;
    D3DFORMAT               m_fmtTexture;
    bool                    m_bSubsurfaceScattering;
    float                   m_fLengthScale;
    bool                    m_bSpectral;
    DWORD                   m_dwPredefinedMatIndex;
    D3DXCOLOR               m_DiffuseReflectance;
    D3DXCOLOR               m_Absoption;
    D3DXCOLOR               m_ReducedScattering;
    float                   m_fRelativeIndexOfRefraction;
    TCHAR                   m_strResultsFile[MAX_PATH];

    // The basis buffer is a large array of floats where 
    // Call D3DXSHPRTCompExtractBasis() to extract the basis 
    // for every cluster.  The basis for a cluster is an array of
    // (NumPCAVectors+1)*(NumChannels*Order^2) floats. 
    // The "1+" is for the cluster mean.
    float* m_aClusterBases;

    // m_fClusteredPCA stores the incident radiance dotted with the transfer function.
    // Each cluster has an array of floats which is the size of 
    // 4+MAX_NUM_CHANNELS*NUM_PCA_VECTORS. This number comes from: there can 
    // be up to 3 channels (R,G,B), and each channel can 
    // have up to NUM_PCA_VECTORS of PCA vectors.  Each cluster also has 
    // a mean PCA vector which is described with 4 floats (and hence the +4).
    float* m_aClusteredPCA;

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

    HRESULT DoStartupDlg();
    HRESULT LoadMeshAndResults();
    HRESULT DoCompressDlg();
    HRESULT LoadMesh( TCHAR* strFileName, LPD3DXMESH* ppMesh );
    HRESULT UpdateUI();
    HRESULT UpdateLightDir();

    HRESULT RenderLightSphere( D3DXVECTOR3 lightDir, D3DXCOLOR sphereColor );
    HRESULT RenderMeshWithSHPRT();
    HRESULT RenderMeshWithSimpleLighting();
    HRESULT RenderText();

    HRESULT LaunchPRTSimulation();
    HRESULT StopPRTSimulation();
    HRESULT CompressData();
    static DWORD WINAPI StaticPRTSimulationThreadProc( LPVOID lpParameter );
    DWORD PRTSimulationThreadProc();
    static HRESULT WINAPI StaticSHPRTSimulatorCB( float fPercentDone );
    HRESULT WINAPI SHPRTSimulatorCB( float fPercentDone );

    HRESULT SetupTexWithPCAData( ID3DXMesh* pMesh, ID3DXBuffer* pPCABuffer, D3DXSHPRTBUFFER_DESC desc, DWORD dwNumPCAVectors );
    HRESULT EvalLightsAndSetConstants();
    HRESULT SetShaderConstants( float* pfRLC, float* pfGLC, float* pfBLC );
    HRESULT AdjustMeshDecl( ID3DXMesh** ppMesh );
    HRESULT LoadSHPRTEffect();

    HRESULT SaveSHPRTBufferToFile( ID3DXBuffer* pd3dxBuffer, TCHAR* strFile );
    HRESULT ReadSHPRTBufferFromFile( TCHAR* strFile, ID3DXBuffer** ppd3dxBuffer );

    static INT_PTR CALLBACK StaticStartupDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK StaticSimulationOptionsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
    static INT_PTR CALLBACK StaticCompressDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
    INT_PTR CALLBACK SimulationOptionsDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
    INT_PTR CALLBACK StartupDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
    INT_PTR CALLBACK CompressDlgProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

public:
    CMyD3DApplication();
    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
};

extern CMyD3DApplication g_d3dApp;




