//-----------------------------------------------------------------------------
// File: OptionsDlg.cpp
//
// Desc: 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#undef _WIN32_IE 
#define _WIN32_IE 0x0400
#include "dxstdafx.h"
#include <commdlg.h>
#include <commctrl.h>
#include "SHPRTPixel.h"
#include "resource.h"


BOOL CALLBACK EnumChildProc( HWND hwnd, LPARAM lParam );
LRESULT CALLBACK GetMsgProc( int code, WPARAM wParam, LPARAM lParam );
void GetSimulationOptionsToolTipText( int nDlgId, NMTTDISPINFO* pNMTDI );
void GetStartupToolTipText( int nDlgId, NMTTDISPINFO* pNMTDI );
void GetCompressToolTipText( int nDlgId, NMTTDISPINFO* pNMTDI );
void UpdateConstText( HWND hDlg, DWORD dwMaxVertexShaderConst, DWORD dwTextureSize );
HWND  g_hToolTip = NULL;
HHOOK g_hMsgProcHook = NULL;
HWND  g_hDlg = NULL;

D3DFORMAT g_fmtTexture[] = 
{
    D3DFMT_Q8W8V8U8,
    D3DFMT_Q16W16V16U16
};


const int g_dwFmt = sizeof(g_fmtTexture) / sizeof(g_fmtTexture[0]);

//-----------------------------------------------------------------------------
// Name: StaticCompressDlgProc
// Desc: static helper function
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CMyD3DApplication::StaticCompressDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return g_d3dApp.CompressDlgProc( hDlg, uMsg, wParam, lParam );
}



//-----------------------------------------------------------------------------
// Name: CompressDlgProc()
// Desc: Handles messages for the compress dialog
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CMyD3DApplication::CompressDlgProc( HWND hDlg, UINT msg,
                                                    WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_INITDIALOG:
        {
            DWORD i;
            g_hDlg = hDlg;

            HWND hPCACombo = GetDlgItem( hDlg, IDC_PCA_VECTOR_COMBO );
            TCHAR sz[256];
            int nIndex = 0;
            int nSelection = -1;
            for( i=4; i<=min(m_dwOrder*m_dwOrder,24); i += 4 )
            {            
                _sntprintf( sz, 256, TEXT("%d"), i );
                sz[255] = 0;               
                nIndex = (int) SendMessage( hPCACombo, CB_ADDSTRING, 0, (LPARAM) sz );                
                if( nIndex >= 0 ) 
                    SendMessage( hPCACombo, CB_SETITEMDATA, nIndex, (LPARAM) i );
                if( i == m_dwNumPCAVectors )
                    nSelection = nIndex;
            }
            if( nSelection == -1 )
                nSelection = nIndex;
            SendMessage( hPCACombo, CB_SETCURSEL, nSelection, 0 );

            HWND hTexFormatCombo = GetDlgItem( hDlg, IDC_TEXTURE_FORMAT_COMBO );
            nIndex = 0;
            nSelection = -1;
            for( i=0; i<g_dwFmt; i++ )
            {            
                _tcsncpy( sz, D3DUtil_D3DFormatToString(g_fmtTexture[i], false), 256 );
                sz[255] = 0;               
                nIndex = (int) SendMessage( hTexFormatCombo, CB_ADDSTRING, 0, (LPARAM) sz );
                if( nIndex >= 0 ) 
                    SendMessage( hTexFormatCombo, CB_SETITEMDATA, nIndex, (LPARAM) g_fmtTexture[i] );
                if( g_fmtTexture[i] == m_fmtTexture )
                    nSelection = nIndex;
            }
            if( nSelection == -1 )
                nSelection = nIndex;
            SendMessage( hTexFormatCombo, CB_SETCURSEL, nSelection, 0 );

            g_hToolTip = CreateWindowEx( 0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, 
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                         hDlg, NULL, g_hInst, NULL );                            
            SendMessage( g_hToolTip, TTM_SETMAXTIPWIDTH, 0, 300 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_AUTOPOP, 32000 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_INITIAL, 0 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_RESHOW, 0 );
            EnumChildWindows( hDlg, (WNDENUMPROC) EnumChildProc, 0);
            g_hMsgProcHook = SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, g_hInst, GetCurrentThreadId() );

            _tcsncpy( sz, TEXT("32"), 256 );
            sz[255] = 0;
            SetDlgItemText( hDlg, IDC_MAX_CONSTS_EDIT, sz );

            _sntprintf( sz, 256, TEXT("%d"), m_dwTextureSize );
            sz[255] = 0;
            SetDlgItemText( hDlg, IDC_TEXTURE_SIZE_EDIT, sz );

            UpdateConstText( hDlg, m_d3dCaps.MaxVertexShaderConst, m_dwTextureSize );

            return TRUE;
        }

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_PCA_VECTOR_COMBO:
                case IDC_TEXTURE_FORMAT_COMBO:
                {
                    UpdateConstText( hDlg, m_d3dCaps.MaxVertexShaderConst, m_dwTextureSize );
                    break;
                }

                case IDOK:
                {
                    int nIndex = (int) SendMessage( GetDlgItem( hDlg, IDC_PCA_VECTOR_COMBO ), CB_GETCURSEL, 0, 0 );
                    LRESULT lResult = SendMessage( GetDlgItem( hDlg, IDC_PCA_VECTOR_COMBO ), CB_GETITEMDATA, nIndex, 0 );
                    if( lResult != CB_ERR )
                        m_dwNumPCAVectors = (DWORD)lResult;

                    nIndex = (int) SendMessage( GetDlgItem( hDlg, IDC_TEXTURE_FORMAT_COMBO ), CB_GETCURSEL, 0, 0 );
                    lResult = SendMessage( GetDlgItem( hDlg, IDC_TEXTURE_FORMAT_COMBO ), CB_GETITEMDATA, nIndex, 0 );
                    if( lResult != CB_ERR )
                        m_fmtTexture = (D3DFORMAT)lResult;

                    EndDialog(hDlg, IDOK);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR* pNMHDR = (LPNMHDR) lParam;
            if( pNMHDR == NULL )
                break;

            if( pNMHDR->code == TTN_NEEDTEXT )
            {
                NMTTDISPINFO* pNMTDI = (LPNMTTDISPINFO)lParam;
                int nDlgId = GetDlgCtrlID( (HWND)pNMHDR->idFrom );
                GetCompressToolTipText( nDlgId, pNMTDI );
                return TRUE;
            }
            break;
        }

        case WM_CLOSE:
            UnhookWindowsHookEx( g_hMsgProcHook );
            DestroyWindow( g_hToolTip );
            break;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: UpdateConstText
// Desc: update the dlg's text & controls
//-----------------------------------------------------------------------------
void UpdateConstText( HWND hDlg, DWORD dwMaxVertexShaderConst, DWORD dwTextureSize )
{
    TCHAR sz[256];

    DWORD dwNumPCAVectors = 0;
    int nIndex = (int) SendMessage( GetDlgItem( hDlg, IDC_PCA_VECTOR_COMBO ), CB_GETCURSEL, 0, 0 );
    LRESULT lResult = SendMessage( GetDlgItem( hDlg, IDC_PCA_VECTOR_COMBO ), CB_GETITEMDATA, nIndex, 0 );
    if( lResult != CB_ERR )
        dwNumPCAVectors = (DWORD)lResult;

    D3DFORMAT fmtTexture = D3DFMT_UNKNOWN;
    nIndex = (int) SendMessage( GetDlgItem( hDlg, IDC_TEXTURE_FORMAT_COMBO ), CB_GETCURSEL, 0, 0 );
    lResult = SendMessage( GetDlgItem( hDlg, IDC_TEXTURE_FORMAT_COMBO ), CB_GETITEMDATA, nIndex, 0 );
    if( lResult != CB_ERR )
        fmtTexture = (D3DFORMAT)lResult;

    _sntprintf( sz, 256, TEXT("%d"), dwNumPCAVectors/4 );
    sz[255] = 0;
    SetDlgItemText( hDlg, IDC_TEXTURE_NUMBER_EDIT, sz );

    int nBytesPerTexel = 0;
    switch( fmtTexture )
    {
        case D3DFMT_Q8W8V8U8: nBytesPerTexel = 32; break;
        case D3DFMT_Q16W16V16U16: nBytesPerTexel = 64; break;
    }
    _sntprintf( sz, 256, TEXT("%d"), dwNumPCAVectors/4 * dwTextureSize*dwTextureSize * nBytesPerTexel );
    sz[255] = 0;
    SetDlgItemText( hDlg, IDC_TEXTURE_MEMORY_EDIT, sz );

    DWORD dwNumVConsts = 1 * (1 + MAX_NUM_CHANNELS*dwNumPCAVectors/4) + 4;
    _sntprintf( sz, 256, TEXT("%d * (1 + (3*%d/4)) + 4 = %d"), 1, dwNumPCAVectors, dwNumVConsts );
    sz[255] = 0;
    SetDlgItemText( hDlg, IDC_NUM_CONSTS_EDIT, sz );

    EnableWindow( GetDlgItem(hDlg, IDOK), ( dwNumVConsts <= dwMaxVertexShaderConst ) );
}




//-----------------------------------------------------------------------------
// Name: StaticSimulationOptionsDlgProc
// Desc: static helper function
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CMyD3DApplication::StaticStartupDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return g_d3dApp.StartupDlgProc( hDlg, uMsg, wParam, lParam );
}



//-----------------------------------------------------------------------------
// Name: StartupDlgProc()
// Desc: Handles messages for the startup dialog
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CMyD3DApplication::StartupDlgProc( HWND hDlg, UINT msg,
                                                    WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_INITDIALOG:
        {
            g_hDlg = hDlg;

            SetDlgItemText( hDlg, IDC_INPUT_MESH_EDIT, m_strInputMesh );
            SetDlgItemText( hDlg, IDC_SIM_RESULTS_EDIT, m_strResultsFile );

            g_hToolTip = CreateWindowEx( 0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, 
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                         hDlg, NULL, g_hInst, NULL );                            
            SendMessage( g_hToolTip, TTM_SETMAXTIPWIDTH, 0, 300 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_AUTOPOP, 32000 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_INITIAL, 0 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_RESHOW, 0 );
            EnumChildWindows( hDlg, (WNDENUMPROC) EnumChildProc, 0);
            g_hMsgProcHook = SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, g_hInst, GetCurrentThreadId() );

            return TRUE;
        }
        
        case WM_NOTIFY:
        {
            NMHDR* pNMHDR = (LPNMHDR) lParam;
            if( pNMHDR == NULL )
                break;

            if( pNMHDR->code == TTN_NEEDTEXT )
            {
                NMTTDISPINFO* pNMTDI = (LPNMTTDISPINFO)lParam;
                int nDlgId = GetDlgCtrlID( (HWND)pNMHDR->idFrom );
                GetStartupToolTipText( nDlgId, pNMTDI );
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_RUN_PRT_SIM:
                {
                    EndDialog(hDlg, IDC_RUN_PRT_SIM);
                    break;
                }

                case IDC_INPUT_MESH_BROWSE_BUTTON:
                {
                    // Display the OpenFileName dialog
                    OPENFILENAME ofn = { sizeof(OPENFILENAME), hDlg, NULL,
                                        _T(".X Files (.x)\0*.x\0All Files\0*.*\0\0"), 
                                        NULL, 0, 1, m_strInputMesh, MAX_PATH, NULL, 0, 
                                        m_strInitialDir, _T("Open Mesh File"), 
                                        OFN_FILEMUSTEXIST, 0, 1, NULL, 0, NULL, NULL };
                    if( TRUE == GetOpenFileName( &ofn ) )
                    {
                        _tcscpy( m_strInitialDir, m_strInputMesh );
                        TCHAR* pLastSlash =  _tcsrchr( m_strInitialDir, _T('\\') );
                        if( pLastSlash )
                            *pLastSlash = 0;
                        SetCurrentDirectory( m_strInitialDir );
                        SetDlgItemText( hDlg, IDC_INPUT_MESH_EDIT, m_strInputMesh );
                    }
                    break;
                }

                case IDC_SIM_RESULTS_BROWSE_BUTTON:
                {
                    // Display the OpenFileName dialog
                    OPENFILENAME ofn = { sizeof(OPENFILENAME), hDlg, NULL,
                                        _T("DAT Files (.dat)\0*.dat\0All Files\0*.*\0\0"), 
                                        NULL, 0, 1, m_strResultsFile, MAX_PATH, NULL, 0, 
                                        m_strInitialDir, _T("Open Results File"), 
                                        OFN_FILEMUSTEXIST, 0, 1, NULL, 0, NULL, NULL };
                    if( TRUE == GetOpenFileName( &ofn ) )
                    {
                        _tcscpy( m_strInitialDir, m_strResultsFile );
                        TCHAR* pLastSlash =  _tcsrchr( m_strInitialDir, _T('\\') );
                        if( pLastSlash )
                            *pLastSlash = 0;
                        SetCurrentDirectory( m_strInitialDir );
                        SetDlgItemText( hDlg, IDC_SIM_RESULTS_EDIT, m_strResultsFile );
                    }
                    break;
                }

                case IDC_VIEW_RESULTS:
                {
                    HRESULT hr;
                    GetDlgItemText( hDlg, IDC_INPUT_MESH_EDIT, m_strInputMesh, MAX_PATH );
                    GetDlgItemText( hDlg, IDC_SIM_RESULTS_EDIT, m_strResultsFile, MAX_PATH );

                    if( GetFileAttributes( m_strResultsFile ) == 0xFFFFFFFF )
                    {
                        MessageBox( m_hWnd, TEXT("Couldn't find the simulator results file.  Run the simulator first to precompute the transfer vectors for the mesh."), TEXT("Error"), MB_OK );
                        break;
                    }

                    TCHAR szMesh[MAX_PATH];
                    DXUtil_FindMediaFileCb( szMesh, sizeof(szMesh), m_strInputMesh );
                    if( GetFileAttributes( szMesh ) == 0xFFFFFFFF )
                    {
                        MessageBox( m_hWnd, TEXT("Couldn't find the mesh file.  Be sure the mesh file exists."), TEXT("Error"), MB_OK );
                        break;
                    }

                    LPD3DXMESH pMesh = NULL;
                    hr = D3DXLoadMeshFromX( szMesh, D3DXMESH_MANAGED, m_pd3dDevice, 
                                            NULL, NULL, NULL, NULL, &pMesh);
                    if( FAILED(hr) )
                    {
                        SAFE_RELEASE( pMesh );
                        MessageBox( m_hWnd, TEXT("Couldn't load the mesh file.  Be sure the mesh file is valid."), TEXT("Error"), MB_OK );
                        break;
                    }

                    // Make sure there are texture coordinates which are required for texturing
                    if( ((pMesh->GetFVF() & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) == 0 )
                    {
                        SAFE_RELEASE( pMesh );
                        MessageBox( m_hWnd, TEXT("The mesh file does not have texture coordinates.  They are needed for per pixel PRT."), TEXT("Error"), MB_OK );
                        break;
                    }
                    SAFE_RELEASE( pMesh );

                    EndDialog(hDlg, IDC_VIEW_RESULTS);
                    break;
                }

                case IDCANCEL:
                {
                    EndDialog(hDlg, IDCANCEL);
                    break;
                }
            }
            break;

        case WM_CLOSE:
            UnhookWindowsHookEx( g_hMsgProcHook );
            DestroyWindow( g_hToolTip );
            break;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: StaticSimulationOptionsDlgProc
// Desc: static helper function
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CMyD3DApplication::StaticSimulationOptionsDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return g_d3dApp.SimulationOptionsDlgProc( hDlg, uMsg, wParam, lParam );
}



//-----------------------------------------------------------------------------
// Name: SimulationOptionsDlgProc()
// Desc: Handles messages for the Simulation options dialog
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CMyD3DApplication::SimulationOptionsDlgProc( HWND hDlg, UINT msg,
                                                             WPARAM wParam, LPARAM lParam )
{
    static bool s_bComboBoxSelChange = false;

    switch( msg )
    {
        case WM_INITDIALOG:
        {
            TCHAR sz[256];
            g_hDlg = hDlg;
            SetDlgItemText( hDlg, IDC_MESH_NAME, m_strInputMesh );

            HWND hOrderSlider = GetDlgItem( hDlg, IDC_ORDER_SLIDER );
            SendMessage( hOrderSlider, TBM_SETRANGE, 0, MAKELONG(2, 6) );
            SendMessage( hOrderSlider, TBM_SETTICFREQ, 1, 0 );
            SendMessage( hOrderSlider, TBM_SETPOS, 1, m_dwOrder );

            HWND hTexSizeCombo = GetDlgItem( hDlg, IDC_TEXTURE_SIZE_COMBO );
			DWORD i;
            int nIndex = 0;
            int nSelection = -1;
            for( i=6; i<12; i++ )
            {                
                DWORD nSize = (DWORD)powf(2.0f,(float)i);
                _sntprintf( sz, 256, TEXT("%d"), nSize ); sz[255] = 0;                 
                nIndex = (int) SendMessage( hTexSizeCombo, CB_ADDSTRING, 0, (LPARAM) sz );                
                if( nIndex >= 0 ) 
                    SendMessage( hTexSizeCombo, CB_SETITEMDATA, nIndex, (LPARAM) nSize );
                if( nSize == m_dwTextureSize )
                    nSelection = nIndex;
            }
            if( nSelection == -1 )
                nSelection = nIndex;
            SendMessage( hTexSizeCombo, CB_SETCURSEL, nSelection, 0 );

            HWND hNumBouncesSpin = GetDlgItem( hDlg, IDC_NUM_BOUNCES_SPIN );
            SendMessage( hNumBouncesSpin, UDM_SETRANGE, 0, (LPARAM) MAKELONG( 3, 0 ) );
            SendMessage( hNumBouncesSpin, UDM_SETPOS, 0, (LPARAM) MAKELONG( m_dwNumBounces, 0) );  

            HWND hNumRaysSpin = GetDlgItem( hDlg, IDC_NUM_RAYS_SPIN );
            UDACCEL udAccel[3];
            udAccel[0].nSec = 0; udAccel[0].nInc = 1;
            udAccel[1].nSec = 1; udAccel[1].nInc = 100;           
            udAccel[2].nSec = 2; udAccel[2].nInc = 1000;           
            SendMessage( hNumRaysSpin, UDM_SETACCEL, 3, (LPARAM) udAccel );           
            SendMessage( hNumRaysSpin, UDM_SETRANGE, 0, (LPARAM) MAKELONG( 10000, 10 ) );
            SendMessage( hNumRaysSpin, UDM_SETPOS, 0, (LPARAM) MAKELONG( m_dwNumRays, 0) );  

            CheckDlgButton( hDlg, IDC_SUBSURF_CHECK, m_bSubsurfaceScattering ? BST_CHECKED : BST_UNCHECKED );
            CheckDlgButton( hDlg, IDC_SPECTRAL_CHECK, m_bSpectral ? BST_CHECKED : BST_UNCHECKED );
            SendMessage( hDlg, WM_COMMAND, IDC_SUBSURF_CHECK, 0 );

            HWND hPreDefCombo = GetDlgItem( hDlg, IDC_PREDEF_COMBO );
            for( i=0; i<(DWORD)g_aPredefinedMaterialsSize; i++ )
            {
                nIndex = (int) SendMessage( hPreDefCombo, CB_ADDSTRING, 0, (LPARAM) g_aPredefinedMaterials[i].strName );                
                if( nIndex >= 0 ) 
                    SendMessage( hPreDefCombo, CB_SETITEMDATA, nIndex, (LPARAM) &g_aPredefinedMaterials[i] );
            }
            SendMessage( hPreDefCombo, CB_SETCURSEL, m_dwPredefinedMatIndex, 0 );

            s_bComboBoxSelChange = true;
            _sntprintf( sz, 256, TEXT("%0.2f"), m_fRelativeIndexOfRefraction ); sz[255] = 0; SetDlgItemText( hDlg, IDC_REFRACTION_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_Absoption.r ); sz[255] = 0; SetDlgItemText( hDlg, IDC_ABSORPTION_R_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_Absoption.g ); sz[255] = 0; SetDlgItemText( hDlg, IDC_ABSORPTION_G_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_Absoption.b ); sz[255] = 0; SetDlgItemText( hDlg, IDC_ABSORPTION_B_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_DiffuseReflectance.r ); sz[255] = 0; SetDlgItemText( hDlg, IDC_REFLECTANCE_R_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_DiffuseReflectance.g ); sz[255] = 0; SetDlgItemText( hDlg, IDC_REFLECTANCE_G_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_DiffuseReflectance.b ); sz[255] = 0; SetDlgItemText( hDlg, IDC_REFLECTANCE_B_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_ReducedScattering.r ); sz[255] = 0; SetDlgItemText( hDlg, IDC_SCATTERING_R_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_ReducedScattering.g ); sz[255] = 0; SetDlgItemText( hDlg, IDC_SCATTERING_G_EDIT, sz ); 
            _sntprintf( sz, 256, TEXT("%0.2f"), m_ReducedScattering.b ); sz[255] = 0; SetDlgItemText( hDlg, IDC_SCATTERING_B_EDIT, sz ); 
            s_bComboBoxSelChange = false;

            _sntprintf( sz, 256, TEXT("%0.1f"), m_fLengthScale ); sz[255] = 0; SetDlgItemText( hDlg, IDC_LENGTH_SCALE_EDIT, sz );

            SetDlgItemText( hDlg, IDC_OUTPUT_EDIT, m_strResultsFile );

            g_hToolTip = CreateWindowEx( 0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP, 
                                         CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                                         hDlg, NULL, g_hInst, NULL );                            
            SendMessage( g_hToolTip, TTM_SETMAXTIPWIDTH, 0, 300 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_AUTOPOP, 32000 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_INITIAL, 0 );
            SendMessage( g_hToolTip, TTM_SETDELAYTIME, (WPARAM)(DWORD) TTDT_RESHOW, 0 );
            EnumChildWindows( hDlg, (WNDENUMPROC) EnumChildProc, 0);
            g_hMsgProcHook = SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, g_hInst, GetCurrentThreadId() );

            return TRUE;
        }
        
        case WM_NOTIFY:
        {
            NMHDR* pNMHDR = (LPNMHDR) lParam;
            if( pNMHDR == NULL )
                break;

            if( pNMHDR->code == TTN_NEEDTEXT )
            {
                NMTTDISPINFO* pNMTDI = (LPNMTTDISPINFO)lParam;
                int nDlgId = GetDlgCtrlID( (HWND)pNMHDR->idFrom );
                GetSimulationOptionsToolTipText( nDlgId, pNMTDI );
                return TRUE;
            }

            switch( wParam )
            {
                case IDC_ORDER_SLIDER:
                {
                    if( pNMHDR && pNMHDR->code == NM_RELEASEDCAPTURE )
                    {
                        TCHAR sz[256];
                        HWND hPCACombo = GetDlgItem( hDlg, IDC_PCA_VECTOR_COMBO );
                        SendMessage( hPCACombo, CB_RESETCONTENT, 0, 0 );
                        DWORD dwOrder = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_ORDER_SLIDER ), TBM_GETPOS, 0, 0 );
                        int nIndex = 0;
                        for( DWORD i=4; i<=24; i += 4 )
                        {
                            if( i > dwOrder * dwOrder )
                                break;

                            _sntprintf( sz, 256, TEXT("%d"), i );
                            nIndex = (int) SendMessage( hPCACombo, CB_ADDSTRING, 0, (LPARAM) sz );                
                            if( nIndex >= 0 ) 
                                SendMessage( hPCACombo, CB_SETITEMDATA, nIndex, (LPARAM) i );
                        }
                        SendMessage( hPCACombo, CB_SETCURSEL, nIndex, 0 );
                        break;
                    }
                }
            }

            break;
        }

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDC_SPECTRAL_CHECK:
                case IDC_SUBSURF_CHECK:
                {
                    BOOL bSubSurface = (IsDlgButtonChecked( hDlg, IDC_SUBSURF_CHECK ) == BST_CHECKED );
                    BOOL bSpectral   = (IsDlgButtonChecked( hDlg, IDC_SPECTRAL_CHECK ) == BST_CHECKED );
                    EnableWindow( GetDlgItem( hDlg, IDC_REFRACTION_TEXT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_ABSORPTION_TEXT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_SCATTERING_TEXT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_REFLECTANCE_R_EDIT ), true );
                    EnableWindow( GetDlgItem( hDlg, IDC_REFLECTANCE_G_EDIT ), bSpectral );
                    EnableWindow( GetDlgItem( hDlg, IDC_REFLECTANCE_B_EDIT ), bSpectral );
                    EnableWindow( GetDlgItem( hDlg, IDC_ABSORPTION_R_EDIT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_ABSORPTION_G_EDIT ), bSubSurface && bSpectral );
                    EnableWindow( GetDlgItem( hDlg, IDC_ABSORPTION_B_EDIT ), bSubSurface && bSpectral );
                    EnableWindow( GetDlgItem( hDlg, IDC_SCATTERING_R_EDIT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_SCATTERING_G_EDIT ), bSubSurface && bSpectral );
                    EnableWindow( GetDlgItem( hDlg, IDC_SCATTERING_B_EDIT ), bSubSurface && bSpectral );
                    EnableWindow( GetDlgItem( hDlg, IDC_REFRACTION_EDIT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_LENGTH_SCALE_TEXT ), bSubSurface );
                    EnableWindow( GetDlgItem( hDlg, IDC_LENGTH_SCALE_EDIT ), bSubSurface );
                    break;
                }

                case IDC_PREDEF_COMBO:
                    if( HIWORD(wParam) == CBN_SELCHANGE )
                    {
                        HWND hPreDefCombo = GetDlgItem( hDlg, IDC_PREDEF_COMBO );
                        int nIndex = (int) SendMessage( hPreDefCombo, CB_GETCURSEL, 0, 0 );
                        LRESULT lResult = SendMessage( hPreDefCombo, CB_GETITEMDATA, nIndex, 0 );
                        if( lResult != CB_ERR )
                        {
                            // If this is the "Custom" material, don't change the numbers
                            if( nIndex == g_aPredefinedMaterialsSize-1 )
                                break; 

                            PREDEFINED_MATERIALS* pMat = (PREDEFINED_MATERIALS*) lResult;
                            TCHAR sz[256];
                            ZeroMemory( sz, 256 );

                            s_bComboBoxSelChange = true;
                            _sntprintf( sz, 256, TEXT("%0.4f"), pMat->fAbsorptionCoefficient.r ); SetDlgItemText( hDlg, IDC_ABSORPTION_R_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.4f"), pMat->fAbsorptionCoefficient.g ); SetDlgItemText( hDlg, IDC_ABSORPTION_G_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.4f"), pMat->fAbsorptionCoefficient.b ); SetDlgItemText( hDlg, IDC_ABSORPTION_B_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fDiffuseReflectance.r ); SetDlgItemText( hDlg, IDC_REFLECTANCE_R_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fDiffuseReflectance.g ); SetDlgItemText( hDlg, IDC_REFLECTANCE_G_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fDiffuseReflectance.b ); SetDlgItemText( hDlg, IDC_REFLECTANCE_B_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fScatteringCoefficient.r ); SetDlgItemText( hDlg, IDC_SCATTERING_R_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fScatteringCoefficient.g ); SetDlgItemText( hDlg, IDC_SCATTERING_G_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fScatteringCoefficient.b ); SetDlgItemText( hDlg, IDC_SCATTERING_B_EDIT, sz ); 
                            _sntprintf( sz, 256, TEXT("%0.2f"), pMat->fRelativeIndexOfRefraction ); SetDlgItemText( hDlg, IDC_REFRACTION_EDIT, sz ); 
                            s_bComboBoxSelChange = false;
                        }
                    }
                    break;

                case IDC_REFLECTANCE_R_EDIT:
                case IDC_REFLECTANCE_G_EDIT:
                case IDC_REFLECTANCE_B_EDIT:
                case IDC_SCATTERING_R_EDIT:
                case IDC_SCATTERING_G_EDIT:
                case IDC_SCATTERING_B_EDIT:
                case IDC_ABSORPTION_R_EDIT:
                case IDC_ABSORPTION_G_EDIT:
                case IDC_ABSORPTION_B_EDIT:
                {
                    if( HIWORD(wParam) == EN_CHANGE && !s_bComboBoxSelChange )
                    {
                        HWND hPreDefCombo = GetDlgItem( hDlg, IDC_PREDEF_COMBO );
                        SendMessage( hPreDefCombo, CB_SETCURSEL, g_aPredefinedMaterialsSize-1, 0 );
                    }
                    break;
                }

                case IDC_INPUT_BROWSE_BUTTON:
                {
                    // Warn user about unique texture parameterization requirement
                    // Comment out this line if you don't want to see this warning 
                    MessageBox( hDlg, TEXT("Texture space PRT requires a mesh with unique texture parameterization which means that every point on the surface must map to a unique point on the texture. In other words there can be no overlap in the texture space. If the mesh has texture coordinates but they are not unique then the simulator will accept the mesh but the output will look incorrect. Also, the texture coordinates need to be between 0 and 1 and the texture coordinate spacing needs to be enough so that there are 2 texels between pieces including the border.  Note: many mesh files in the media directory do not satisfy these requirements."), TEXT("Texture space PRT requirements"), MB_ICONWARNING|MB_OK );

                    // Display the OpenFileName dialog
                    OPENFILENAME ofn = { sizeof(OPENFILENAME), hDlg, NULL,
                                        _T(".X Files (.x)\0*.x\0All Files\0*.*\0\0"), 
                                        NULL, 0, 1, m_strInputMesh, MAX_PATH, NULL, 0, 
                                        m_strInitialDir, _T("Open Mesh File"), 
                                        OFN_FILEMUSTEXIST, 0, 1, NULL, 0, NULL, NULL };
                    if( TRUE == GetOpenFileName( &ofn ) )
                    {
                        _tcscpy( m_strInitialDir, m_strInputMesh );
                        TCHAR* pLastSlash =  _tcsrchr( m_strInitialDir, _T('\\') );
                        if( pLastSlash )
                            *pLastSlash = 0;
                        SetCurrentDirectory( m_strInitialDir );
                        SetDlgItemText( hDlg, IDC_MESH_NAME, m_strInputMesh );
                    }
                    break;
                }

                case IDC_OUTPUT_BROWSE_BUTTON:
                {
                    // Display the OpenFileName dialog
                    OPENFILENAME ofn = { sizeof(OPENFILENAME), hDlg, NULL,
                                        _T("DAT Files (.dat)\0*.dat\0All Files\0*.*\0\0"), 
                                        NULL, 0, 1, m_strResultsFile, MAX_PATH, NULL, 0, 
                                        m_strInitialDir, _T("Save Results File"), 
                                        OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_NOREADONLYRETURN, 
                                        0, 1, TEXT(".dat"), 0, NULL, NULL };
                    if( TRUE == GetSaveFileName( &ofn ) )
                    {
                        _tcscpy( m_strInitialDir, m_strResultsFile );
                        TCHAR* pLastSlash =  _tcsrchr( m_strInitialDir, _T('\\') );
                        if( pLastSlash )
                            *pLastSlash = 0;
                        SetCurrentDirectory( m_strInitialDir );
                        SetDlgItemText( hDlg, IDC_OUTPUT_EDIT, m_strResultsFile );
                    }
                    break;
                }

                case IDOK:
                {
                    HRESULT hr;
                    GetDlgItemText( hDlg, IDC_MESH_NAME, m_strInputMesh, MAX_PATH );

                    TCHAR szMesh[MAX_PATH];
                    DXUtil_FindMediaFileCb( szMesh, sizeof(szMesh), m_strInputMesh );
                    if( GetFileAttributes( szMesh ) == 0xFFFFFFFF )
                    {
                        MessageBox( m_hWnd, TEXT("Couldn't find the mesh file.  Be sure the mesh file exists."), TEXT("Error"), MB_OK );
                        break;
                    }

                    LPD3DXMESH pMesh = NULL;
                    hr = D3DXLoadMeshFromX( szMesh, D3DXMESH_MANAGED, m_pd3dDevice, 
                                            NULL, NULL, NULL, NULL, &pMesh);
                    if( FAILED(hr) )
                    {
                        SAFE_RELEASE( pMesh );
                        MessageBox( m_hWnd, TEXT("Couldn't load the mesh file.  Be sure the mesh file is valid."), TEXT("Error"), MB_OK );
                        break;
                    }

                    // Make sure there are texture coordinates which are required for texturing
                    if( ((pMesh->GetFVF() & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT) == 0 )
                    {
                        SAFE_RELEASE( pMesh );
                        MessageBox( m_hWnd, TEXT("The mesh file does not have texture coordinates.  They are needed for per pixel PRT."), TEXT("Error"), MB_OK );
                        break;
                    }
                    SAFE_RELEASE( pMesh );

                    m_dwOrder = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_ORDER_SLIDER ), TBM_GETPOS, 0, 0 );
                    m_dwNumBounces = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_NUM_BOUNCES_SPIN ), UDM_GETPOS, 0, 0 );
                    m_dwNumRays = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_NUM_RAYS_SPIN ), UDM_GETPOS, 0, 0 );
                    m_bSubsurfaceScattering = (IsDlgButtonChecked( hDlg, IDC_SUBSURF_CHECK ) == BST_CHECKED );
                    m_bSpectral = (IsDlgButtonChecked( hDlg, IDC_SPECTRAL_CHECK ) == BST_CHECKED );
                    m_dwPredefinedMatIndex = (DWORD) SendMessage( GetDlgItem( hDlg, IDC_PREDEF_COMBO ), CB_GETCURSEL, 0, 0 );

                    int nIndex = (int) SendMessage( GetDlgItem( hDlg, IDC_TEXTURE_SIZE_COMBO ), CB_GETCURSEL, 0, 0 );
                    LRESULT lResult = SendMessage( GetDlgItem( hDlg, IDC_TEXTURE_SIZE_COMBO ), CB_GETITEMDATA, nIndex, 0 );
                    if( lResult != CB_ERR )
                        m_dwTextureSize = (DWORD)lResult;

                    TCHAR sz[256];
                    GetDlgItemText( hDlg, IDC_REFRACTION_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_fRelativeIndexOfRefraction ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_ABSORPTION_R_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_Absoption.r ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_ABSORPTION_G_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_Absoption.g ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_ABSORPTION_B_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_Absoption.b ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_REFLECTANCE_R_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_DiffuseReflectance.r ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_REFLECTANCE_G_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_DiffuseReflectance.g ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_REFLECTANCE_B_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_DiffuseReflectance.b ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_SCATTERING_R_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_ReducedScattering.r ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_SCATTERING_G_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_ReducedScattering.g ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_SCATTERING_B_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_ReducedScattering.b ) == 0 ) return false;
                    GetDlgItemText( hDlg, IDC_LENGTH_SCALE_EDIT, sz, 256 ); if( _stscanf( sz, TEXT("%f"), &m_fLengthScale ) == 0 ) return false;

                    GetDlgItemText( hDlg, IDC_OUTPUT_EDIT, m_strResultsFile, MAX_PATH );

                    EndDialog(hDlg, IDOK);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            UnhookWindowsHookEx( g_hMsgProcHook );
            DestroyWindow( g_hToolTip );
            break;
    }

    return FALSE;
}




//-----------------------------------------------------------------------------
// Name: EnumChildProc
// Desc: Helper function to add tooltips to all the children (buttons/etc) 
//       of the dialog box
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumChildProc( HWND hwndChild, LPARAM lParam )
{
    TOOLINFO ti;
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_IDISHWND;
    ti.hwnd     = g_hDlg;
    ti.uId      = (UINT_PTR) hwndChild;
    ti.hinst    = 0;
    ti.lpszText = LPSTR_TEXTCALLBACK;
    SendMessage( g_hToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti );

    return TRUE;
}




//-----------------------------------------------------------------------------
// Name: GetMsgProc
// Desc: msg proc hook needed to display tooltips in a dialog box
//-----------------------------------------------------------------------------
LRESULT CALLBACK GetMsgProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    MSG* pMSG = (MSG*) lParam;
    if( nCode < 0 || !(IsChild( g_hDlg, pMSG->hwnd) ) )
        return CallNextHookEx( g_hMsgProcHook, nCode, wParam, lParam ); 

    switch( pMSG->message )
    {
        case WM_MOUSEMOVE: 
        case WM_LBUTTONDOWN: 
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: 
        case WM_RBUTTONUP:
        {
            if( g_hToolTip )
            {
                MSG newMsg;
                newMsg.lParam  = pMSG->lParam;
                newMsg.wParam  = pMSG->wParam;
                newMsg.message = pMSG->message;
                newMsg.hwnd    = pMSG->hwnd;
                SendMessage( g_hToolTip, TTM_RELAYEVENT, 0, (LPARAM) &newMsg );
            }
            break;
        }
    }

    return CallNextHookEx( g_hMsgProcHook, nCode, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: GetStartupToolTipText
// Desc: returns the tooltip text for the control 
//-----------------------------------------------------------------------------
void GetStartupToolTipText( int nDlgId, NMTTDISPINFO* pNMTDI )
{
    TCHAR* str = NULL;
    switch( nDlgId )
    {
        case IDC_WHY: str = TEXT("Precomputed radiance transfer (PRT) using low-order spherical harmonic (SH) basis functions have a number of advantages over typical diffuse (N dot L) lighting.  Area light sources, interreflections, soft shadows, self shadowing, and subsurface scattering can be rendered in real time after a precomputed simulation."); break;

        case IDC_HOW: str = TEXT("The approach is to precompute for a given object an expensive light transport simulation that simulates shadows, interreflections, and subsurface scattering.  For each vertex or texel, the results are recorded as transfer coefficients for the SH basis functions.  Then in real time the incident radiance is computed as another set of coefficients of the SH basis functions.  These two sets of coefficients are used in real time by the shader to create the lighting for the object."); break;

        case IDC_WHAT: str = TEXT("This sample demonstrates how use D3DXSHPRTSimulationTex(), a texel based SH PRT simulator and records the results to a file.  It can then view the results in result time with a ps_2_0 pixel shader."); break;

        case IDC_RUN_PRT_SIM: str = TEXT("Run the PRT SH simulator after setting up its parameters"); break;

        case IDC_INPUT_MESH_TEXT: 
        case IDC_INPUT_MESH_EDIT: str = TEXT("The file name of the mesh to render."); break;

        case IDC_INPUT_MESH_BROWSE_BUTTON: str = TEXT("Select the input mesh file name"); break;

        case IDC_SIM_RESULTS_TEXT: 
        case IDC_SIM_RESULTS_EDIT: str = TEXT("Previously saved results of the simulator for this mesh.  The simulator results are used in real time in the pixel shader to light the mesh.  If the results from a different mesh are used then either an error will occur or the lighting will be incorrect."); break;

        case IDC_SIM_RESULTS_BROWSE_BUTTON: str = TEXT("Select the simulation results file name"); break;

        case IDC_VIEW_RESULTS: str = TEXT("View the mesh with a ps_2_0 pixel shader using previously saved simulator results."); break;

        case IDCANCEL: str = TEXT("Cancel the dialog"); break;
    }

    pNMTDI->lpszText = str;
}




//-----------------------------------------------------------------------------
// Name: GetSimulationOptionsToolTipText
// Desc: returns the tooltip text for the control 
//-----------------------------------------------------------------------------
void GetSimulationOptionsToolTipText( int nDlgId, NMTTDISPINFO* pNMTDI )
{
    TCHAR* str = NULL;
    switch( nDlgId )
    {
        case IDC_INPUT_MESH_TEXT: 
        case IDC_MESH_NAME: str = TEXT("This is the file that is loaded as a ID3DXMesh* and passed into D3DXSHPRTSimulationTex() so that it can compute and return spherical harmonic transfer coefficients for each texel in a texture. It returns these coefficients in a ID3DXBuffer*. This process takes some time and should be precomputed, however the results can be used in real time. For more detailed information, see \"Precomputed Radiance Transfer for Real-Time Rendering in Dynamic, Low-Frequency Lighting Environments\" by Peter-Pike Sloan, Jan Kautz, and John Snyder, SIGGRAPH 2002."); break;

        case IDC_ORDER_TEXT: 
        case IDC_ORDER_SLIDER: str = TEXT("This controls the number of spherical harmonic basis functions used. The simulator generates order^2 coefficients per channel. Higher order allows for higher frequency lighting environments which allow for sharper shadows with the tradeoff of more coefficients per texel that need to be processed by the pixel shader. For convex objects (no shadows), 3rd order has very little approximation error.  For more detailed information, see \"Spherical Harmonic Lighting: The Gritty Details\" by Robin Green, GDC 2003 and \"An Efficient Representation of Irradiance Environment Maps\" by Ravi Ramamoorthi, and Pat Hanrahan, SIGGRAPH 2001."); break;

        case IDC_TEXTURE_SIZE_TEXT: 
        case IDC_TEXTURE_SIZE_COMBO: str = TEXT("This is the size of the width and height of the texture.  The larger this number the more accurate the final result will be, but it will increase time it takes to precompute the transfer coefficients and require the more texture memory."); break;

        case IDC_NUM_BOUNCES_TEXT: 
        case IDC_NUM_BOUNCES_EDIT: 
        case IDC_NUM_BOUNCES_SPIN: str = TEXT("This controls the number of bounces simulated. If this is non-zero then inter-reflections are calculated. Inter-reflections are, for example, when a light shines on a red wall and bounces on a white wall. The white wall even though it contains no red in the material will reflect some red do to the bouncing of the light off the red wall."); break;

        case IDC_NUM_RAYS_TEXT: 
        case IDC_NUM_RAYS_EDIT: 
        case IDC_NUM_RAYS_SPIN: str = TEXT("This controls the number of rays to shoot at each sample. The more rays the more accurate the final result will be, but it will increase time it takes to precompute the transfer coefficients."); break;

        case IDC_SUBSURF_CHECK: str = TEXT("If checked then subsurface scattering will be done in the simulator. Subsurface scattering is when light penetrates a translucent surface and comes out the other side. For example, a jade sculpture or a flashlight shining through skin exhibits subsurface scattering. The simulator assumes the mesh is made of a homogenous material. If subsurface scattering is not used, then the length scale, the relative index of refraction, the reduced scattering coefficients, and the absorption coefficients are not used."); break;

        case IDC_SPECTRAL_CHECK: str = TEXT("If checked then the simulator will process 3 channels: red, green, and blue and return order^2 spherical harmonic transfer coefficients for each of these channels in a single ID3DXBuffer* buffer. Otherwise it use values of only one channel (the red channel) and return the transfer coefficients for just that single channel. A single channel is useful for lighting environments that don't need to have the whole spectrum of light such as shadows"); break;

        case IDC_RED_TEXT: str = TEXT("The values below are the red coefficients. If spectral is turned off, then this is the channel that will be used."); break;
        case IDC_GREEN_TEXT: str = TEXT("The values below are the green coefficients"); break;
        case IDC_BLUE_TEXT: str = TEXT("The values below are the blue coefficients"); break;

        case IDC_PREDEF_COMBO: 
        case IDC_PREDEF_TEXT: str = TEXT("These are some example materials. Choosing one of these materials with change the all the material values below. The parameters for these materials are from \"A Practical Model for Subsurface Light Transport\" by Henrik Wann Jensen, Steve R. Marschner, Marc Levoy, Pat Hanrahan, SIGGRAPH 2001. The relative index of refraction is with respect the material immersed in air."); break;

        case IDC_REFRACTION_TEXT: 
        case IDC_REFRACTION_EDIT: str = TEXT("Relative index of refraction is the ratio between two absolute indexes of refraction. An index of refraction is ratio of the sine of the angle of incidence to the sine of the angle of refraction."); break;

        case IDC_LENGTH_SCALE_TEXT: 
        case IDC_LENGTH_SCALE_EDIT: str = TEXT("When subsurface scattering is used the object is mapped to a cube of length scale mm per side. For example, if length scale is 10, then the object is mapped to a 10mm x 10mm x 10mm cube.  The smaller the cube the more light penetrates the object."); break;

        case IDC_SCATTERING_TEXT: 
        case IDC_SCATTERING_G_EDIT: 
        case IDC_SCATTERING_B_EDIT: 
        case IDC_SCATTERING_R_EDIT: str = TEXT("The reduced scattering coefficient is a parameter to the volume rendering equation used to model light propagation in a participating medium. For more detail, see \"A Practical Model for Subsurface Light Transport\" by Henrik Wann Jensen, Steve R. Marschner, Marc Levoy, Pat Hanrahan, SIGGRAPH 2001"); break;

        case IDC_ABSORPTION_TEXT: 
        case IDC_ABSORPTION_G_EDIT: 
        case IDC_ABSORPTION_B_EDIT: 
        case IDC_ABSORPTION_R_EDIT: str = TEXT("The absorption coefficient is a parameter to the volume rendering equation used to model light propagation in a participating medium. For more detail, see \"A Practical Model for Subsurface Light Transport\" by Henrik Wann Jensen, Steve R. Marschner, Marc Levoy, Pat Hanrahan, SIGGRAPH 2001"); break;

        case IDC_REFLECTANCE_TEXT: 
        case IDC_REFLECTANCE_R_EDIT: 
        case IDC_REFLECTANCE_B_EDIT: 
        case IDC_REFLECTANCE_G_EDIT: str = TEXT("The diffuse reflectance coefficient is the fraction of diffuse light reflected back. This value is typically between 0 and 1."); break;

        case IDC_OUTPUT_TEXT: 
        case IDC_OUTPUT_EDIT: str = TEXT("This sample will save a binary buffer of spherical harmonic transfer coefficients to a file which the sample can read in later.  This is the file name of the where the resulting binary buffer will be saved"); break;

        case IDC_OUTPUT_BROWSE_BUTTON: str = TEXT("Select the output file name"); break;

        case IDOK: str = TEXT("This will start the simulator based on the options selected above. This process takes some time and should be precomputed, however the results can be used in real time. When the simulator is done calculating the spherical harmonic transfer coefficients for each texel, the sample will use D3DXSHPRTCompress() to compress the data based on the number of PCA vectors chosen. This sample will then save the binary data of coefficients to a file. This sample will also allow you to view the results by passing these coefficients as a texture to a pixel shader for real time rendering (if the Direct3D device has hardware accelerated programmable pixel shader support). This sample also stores the settings of this dialog to the registry so it remembers them for next time."); break;

        case IDCANCEL: str = TEXT("This cancels the dialog, does not save the settings, and does not run the simulator."); break;
    }

    pNMTDI->lpszText = str;
}




//-----------------------------------------------------------------------------
// Name: GetCompressToolTipText
// Desc: returns the tooltip text for the control 
//-----------------------------------------------------------------------------
void GetCompressToolTipText( int nDlgId, NMTTDISPINFO* pNMTDI )
{
    TCHAR* str = NULL;
    switch( nDlgId )
    {
        case IDC_NUM_PCA_TEXT: 
        case IDC_PCA_VECTOR_COMBO: str = TEXT("The number of PCA vectors controls the amount of compression done after the simulator has run. There are order^2 transfer coefficients per channel. So for example, with order 6 and spectral turned on that would be 108 coefficients per texel. To reduce the amount of data that needs to be passed to the pixel shader, D3DXSHPRTCompress() can be used to compress the data with a certain number of PCA vectors. The number of coefficients per pixel will be reduced to the number of PCA vectors. So, for example, with 20 PCA vectors and order 6 then instead of 108 coefficients per texel we will only need 20 coefficients per texel. The number of PCA vectors must be less than the order^2. For more detailed information, see \"Clustered Principal Components for Precomputed Radiance Transfer\" by Peter-Pike Sloan, Jesse Hall, John Hart, and John Snyder to appear in the Proceedings of SIGGRAPH 2003"); break;

        case IDC_MAX_CONSTS_EDIT: 
        case IDC_MAX_CONSTS_TEXT: str = TEXT("This is the maximum number of constant supported by this HW device for a ps_2_0 shader.  This is number is simply 32 for ps_2_0 shaders."); break;

        case IDC_NUM_CONSTS_EDIT: 
        case IDC_NUM_CONSTS_TEXT: str = TEXT("This is the number of pixel shader constants that will be needed by the sample's shader given the current dialog settings.  The formula is 1*(1+MAX_NUM_CHANNELS*dwNumPCAVectors/4)+4."); break;

        case IDC_TEXTURE_SIZE_COMBO:
        case IDC_TEXTURE_SIZE_EDIT: str = TEXT("This is the size of the width and height of the texture.  This is number is set when the simulator is run."); break;

        case IDC_TEXTURE_NUMBER_EDIT:
        case IDC_TEXTURE_NUMBER_TEXT: str = TEXT("This is the number of textures needed to store all of the PCA weights.  This is number is the number of PCA weights divided by the number of coefficients that can be stored in a texel."); break;

        case IDC_TEXTURE_FORMAT_COMBO:
        case IDC_TEXTURE_FORMAT_TEXT: str = TEXT("This the format of the texture(s) that store the PCA weights."); break;

        case IDC_TEXTURE_MEMORY_EDIT:
        case IDC_TEXTURE_MEMORY_TEXT: str = TEXT("This is the amount of texture memory in bytes that will be used to store all of the PCA weights given current texture size and format."); break;
    }

    pNMTDI->lpszText = str;
}

