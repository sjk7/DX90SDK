//-----------------------------------------------------------------------------
// File: D3DVisualization.cpp
//
// Desc: Modified application class for the Direct3D samples framework library. 
//       This class is extended for use as a base class for Windows Media 
//       Player visualizations.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "d3dvisualization.h"

#define COMPILE_MULTIMON_STUBS
#include <multimon.h>




//-----------------------------------------------------------------------------
// Global access to the app (needed for the static access)
//-----------------------------------------------------------------------------
static CD3DVisualization* g_pD3DApp = NULL;




//-----------------------------------------------------------------------------
// Name: CD3DVisualization()
// Desc: Constructor
//-----------------------------------------------------------------------------
CD3DVisualization::CD3DVisualization()
{
    g_pD3DApp           = this;

    m_pD3D              = NULL;
    m_pd3dDevice        = NULL;
    m_hWnd              = NULL;
    m_hWndFocus         = NULL;
    m_hMenu             = NULL;
    m_hMonitor          = NULL;
    m_bWindowed         = true;
    m_bDeviceLost       = false;
    m_bMinimized        = false;
    m_bMaximized        = false;
    m_bIgnoreSizeChange = false;
    m_bDeviceObjectsInited = false;
    m_bDeviceObjectsRestored = false;
    m_dwCreateFlags     = 0;

    m_bFrameMoving      = true;
    m_fTime             = 0.0f;
    m_fElapsedTime      = 0.0f;
    m_fFPS              = 0.0f;
    m_strDeviceStats[0] = _T('\0');
    m_strFrameStats[0]  = _T('\0');

    m_strWindowTitle    = _T("D3D9 Application");
    m_dwCreationWidth   = 400;
    m_dwCreationHeight  = 300;
    m_bShowCursorWhenFullscreen = false;
    m_bStartFullscreen  = false;
    m_bAllowDialogBoxMode = false;

    m_bInitialized = FALSE;
    m_bSkinMode = FALSE;
    m_bErrorInitializing = FALSE;
    ZeroMemory( &m_strErrorMessage, sizeof(m_strErrorMessage) );

    m_pTexRenderTarget = NULL;
    m_pTexScratch = NULL;

    m_fTimeLastResizeCheck = 0.0f;

    SetRect( &m_RectTarget, 0, 0, 1, 1 );

    // When m_bClipCursorWhenFullscreen is true, the cursor is limited to
    // the device window when the app goes fullscreen.  This prevents users
    // from accidentally clicking outside the app window on a multimon system.
    // This flag is turned off by default for debug builds, since it makes 
    // multimon debugging difficult.
#if defined(_DEBUG) || defined(DEBUG)
    m_bClipCursorWhenFullscreen = false;
#else
    m_bClipCursorWhenFullscreen = true;
#endif
}




//-----------------------------------------------------------------------------
// Name: CreateFullMode()
// Desc: Create the visualization to run in Media Player's full mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::CreateFullMode( HWND hWnd )
{
    HRESULT hr = S_OK;

    m_bInitialized = TRUE;
    m_bSkinMode = FALSE;
    
    m_hWnd = hWnd;
  
    hr = Create();
    if( FAILED(hr) )
        m_bErrorInitializing = TRUE;

    return hr;
}




//-----------------------------------------------------------------------------
// Name: CreateSkinMode()
// Desc: Create the visualization to run in Media Player's skin mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::CreateSkinMode()
{
    HRESULT hr = S_OK;

    m_bInitialized = TRUE;
    m_bSkinMode = TRUE;
    
    m_hWnd = GetDesktopWindow();

    hr = Create();
    if( FAILED(hr) )
        m_bErrorInitializing = TRUE;

    return hr;
}




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Release the visualization's resources
//-----------------------------------------------------------------------------
VOID CD3DVisualization::Destroy()
{
    m_bInitialized = FALSE;
    m_bErrorInitializing = FALSE;

    Cleanup3DEnvironment();
    SAFE_RELEASE( m_pD3D );
    FinalCleanup();
    
    m_hWnd = m_hWndFocus = NULL;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Render the visualization in skin mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::Render(TimedLevel *pLevels, HDC hdc, RECT *rc)
{
    HRESULT hr = S_OK;

    PDIRECT3DSURFACE9 pSurfOldRenderTarget = NULL;
    PDIRECT3DSURFACE9 pSurfNewRenderTarget = NULL;
    PDIRECT3DSURFACE9 pSurfScratch = NULL;
    HDC hdcScratch = NULL;
    
    if( !m_bInitialized )
        return E_FAIL;

    if( m_bErrorInitializing )
    {
        TextOut( hdc, 5, 5, m_strErrorMessage, _tcslen( m_strErrorMessage ) );
        return S_OK;
    }

    // If enough time has passed since the last check, make sure the target
    // dimensions are the same
    if( m_fTime - m_fTimeLastResizeCheck > 1.0f )
    {
        m_fTimeLastResizeCheck = m_fTime;

        // If the user has moved Media Player to a new window, recreate the device
        HMONITOR hMonitor = MonitorFromWindow( m_hWnd, MONITOR_DEFAULTTOPRIMARY );
        if( hMonitor != m_hMonitor )
        {
            Destroy();
            return S_OK;
        }

        // If size has changed, set the correct backbuffer size and recreate
        if( !EqualRect( rc, &m_RectTarget ) )
        {
            CopyRect( &m_RectTarget, rc );
            
            m_d3dpp.BackBufferWidth  = rc->right - rc->left;
            m_d3dpp.BackBufferHeight = rc->bottom - rc->top;
    
            if( m_pd3dDevice != NULL )
            {
                // Reset the 3D environment
                if( FAILED( hr = Reset3DEnvironment() ) )
                {
                    if( hr != D3DERR_OUTOFVIDEOMEMORY )
                        hr = D3DAPPERR_RESETFAILED;
                    DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
                }
            }
        }
    }

    m_pAudioLevels = pLevels;

    if( m_bDeviceLost )
    {
        // Yield some CPU time to other processes
        Sleep( 100 ); // 100 milliseconds
    }

    // Switch the render target since we're not drawing to a window
    hr = m_pd3dDevice->GetRenderTarget( 0, &pSurfOldRenderTarget );
    if( FAILED(hr) )
        goto LCleanReturn;

    hr = m_pTexRenderTarget->GetSurfaceLevel( 0, &pSurfNewRenderTarget );
    if( FAILED(hr) )
        goto LCleanReturn;
    
    hr = m_pd3dDevice->SetRenderTarget( 0, pSurfNewRenderTarget );
    if( FAILED(hr) )
        goto LCleanReturn;

    // Render a frame
    hr = Render3DEnvironment();
    if( FAILED(hr) )
        goto LCleanReturn;

    // Restore the old render target
    hr = m_pd3dDevice->SetRenderTarget( 0, pSurfOldRenderTarget );
    if( FAILED(hr) )
        goto LCleanReturn;

    hr = m_pTexScratch->GetSurfaceLevel( 0, &pSurfScratch );
    if( FAILED(hr) )
        goto LCleanReturn;

    hr = m_pd3dDevice->GetRenderTargetData( pSurfNewRenderTarget, pSurfScratch );
    if( FAILED(hr) )
        goto LCleanReturn;

    hr = pSurfScratch->GetDC( &hdcScratch );
    if( FAILED(hr) )
        goto LCleanReturn;

    // Copy the data to the target DC
    BitBlt( hdc, rc->left, rc->top, 
            rc->right - rc->left,
            rc->bottom - rc->top,
            hdcScratch, 0, 0, SRCCOPY );


    hr = S_OK;

LCleanReturn:
    if( hdcScratch )
        pSurfScratch->ReleaseDC( hdcScratch );

    SAFE_RELEASE( pSurfScratch );
    SAFE_RELEASE( pSurfNewRenderTarget );
    SAFE_RELEASE( pSurfOldRenderTarget );

    return hr;
}




//-----------------------------------------------------------------------------
// Name: GoFullscreen()
// Desc: Toggle fullscreen
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::GoFullscreen(BOOL fFullScreen)
{
    if( !m_bInitialized )
        return E_FAIL;

    if( (BOOL) m_bWindowed == fFullScreen )
    {  
        return ToggleFullscreen();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderFullScreen()
// Desc: Render the visualization in full screen mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::RenderFullScreen(TimedLevel *pLevels)
{
    if( !m_bInitialized )
        return E_FAIL;

    if( m_bErrorInitializing )
        return S_OK;

    m_pAudioLevels = pLevels;

    if( m_bDeviceLost )
    {
        // Yield some CPU time to other processes
        Sleep( 100 ); // 100 milliseconds
    }
    // Render a frame
    return Render3DEnvironment();
}




//-----------------------------------------------------------------------------
// Name: RenderWindowed()
// Desc: Render the visualization in Media Player's full mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::RenderWindowed(TimedLevel *pLevels, BOOL fRequiredRender )
{
    if( !m_bInitialized )
        return E_FAIL;

    if( m_bErrorInitializing )
    {
        HDC hdc;
        hdc = GetDC( m_hWnd );
        TextOut( hdc, 5, 5, m_strErrorMessage, _tcslen( m_strErrorMessage ) );
        ReleaseDC( m_hWnd, hdc );
        return S_OK;
    }

    if( m_fTime - m_fTimeLastResizeCheck > 1.0f )
    {
        m_fTimeLastResizeCheck = m_fTime;

        // If the user has moved Media Player to a new window, recreate the device
        HMONITOR hMonitor = MonitorFromWindow( m_hWnd, MONITOR_DEFAULTTOPRIMARY );
        if( hMonitor != m_hMonitor )
        {
            Destroy();
            return S_OK;
        }

        HandlePossibleSizeChange();
    }

    m_pAudioLevels = pLevels;

    if( m_bDeviceLost )
    {
        // Yield some CPU time to other processes
        Sleep( 100 ); // 100 milliseconds
    }
    // Render a frame
    return Render3DEnvironment();
}




//-----------------------------------------------------------------------------
// Name: ConfirmDeviceHelper()
// Desc: Static function used by D3DEnumeration
//-----------------------------------------------------------------------------
bool CD3DVisualization::ConfirmDeviceHelper( D3DCAPS9* pCaps, VertexProcessingType vertexProcessingType, 
                         D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
    DWORD dwBehavior;

    if (vertexProcessingType == SOFTWARE_VP)
        dwBehavior = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    else if (vertexProcessingType == MIXED_VP)
        dwBehavior = D3DCREATE_MIXED_VERTEXPROCESSING;
    else if (vertexProcessingType == HARDWARE_VP)
        dwBehavior = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else if (vertexProcessingType == PURE_HARDWARE_VP)
        dwBehavior = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
    else
        dwBehavior = 0; // TODO: throw exception
    
    return SUCCEEDED( g_pD3DApp->ConfirmDevice( pCaps, dwBehavior, adapterFormat, backBufferFormat ) );
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Here's what this function does:
//       - Checks to make sure app is still active (if fullscreen, etc)
//       - Checks to see if it is time to draw with DXUtil_Timer, if not, it just returns S_OK
//       - Calls FrameMove() to recalculate new positions
//       - Calls Render() to draw the new frame
//       - Updates some frame count statistics
//       - Calls m_pd3dDevice->Present() to display the rendered frame.
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::Create()
{
    HRESULT hr;

    g_pD3DApp = this;

    // Create the Direct3D object
    m_pD3D = Direct3DCreate9( D3D_SDK_VERSION );
    if( m_pD3D == NULL )
        return DisplayErrorMsg( D3DAPPERR_NODIRECT3D, MSGERR_APPMUSTEXIT );

    // Build a list of Direct3D adapters, modes and devices. The
    // ConfirmDevice() callback is used to confirm that only devices that
    // meet the app's requirements are considered.
    m_d3dEnumeration.SetD3D( m_pD3D );
    m_d3dEnumeration.ConfirmDeviceCallback = ConfirmDeviceHelper;
    if( FAILED( hr = m_d3dEnumeration.Enumerate() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }


    // The focus window can be a specified to be a different window than the
    // device window.  If not, use the device window as the focus window.
    if( m_hWndFocus == NULL )
        m_hWndFocus = m_hWnd;

    // Save window properties
    m_dwWindowStyle = GetWindowLong( m_hWnd, GWL_STYLE );
    GetWindowRect( m_hWnd, &m_rcWindowBounds );
    GetClientRect( m_hWnd, &m_rcWindowClient );

    if( FAILED( hr = ChooseInitialD3DSettings() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    // Initialize the application timer
    DXUtil_Timer( TIMER_START );

    // Initialize the app's custom scene stuff
    if( FAILED( hr = OneTimeSceneInit() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    // Initialize the 3D environment for the app
    if( FAILED( hr = Initialize3DEnvironment() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FindBestWindowedMode()
// Desc: Sets up m_d3dSettings with best available windowed mode, subject to 
//       the bRequireHAL and bRequireREF constraints.  Returns false if no such
//       mode can be found.
//-----------------------------------------------------------------------------
bool CD3DVisualization::FindBestWindowedMode( bool bRequireHAL, bool bRequireREF )
{
    // Retrieve the monitor which shares the most screen space with the 
    // render window
    m_hMonitor = MonitorFromWindow( m_hWnd, MONITOR_DEFAULTTOPRIMARY );

    // Determine which adapter draws to that monitor
    D3DAdapterInfo* pAdapterInfo = NULL;
    for( UINT iai = 0; iai < m_d3dEnumeration.m_pAdapterInfoList->Count(); iai++ )
    {
        D3DAdapterInfo* pInfo = (D3DAdapterInfo*)m_d3dEnumeration.m_pAdapterInfoList->GetPtr(iai);
        HMONITOR hAdapterMon = m_pD3D->GetAdapterMonitor( pInfo->AdapterOrdinal );

        if( hAdapterMon == m_hMonitor )
        {
            pAdapterInfo = pInfo;
            break;
        }
    }

    if( pAdapterInfo == NULL )
        return false;

    // Get display mode of current adapter
    D3DDISPLAYMODE primaryDesktopDisplayMode;
    m_pD3D->GetAdapterDisplayMode(pAdapterInfo->AdapterOrdinal, &primaryDesktopDisplayMode);

    D3DAdapterInfo* pBestAdapterInfo = NULL;
    D3DDeviceInfo* pBestDeviceInfo = NULL;
    D3DDeviceCombo* pBestDeviceCombo = NULL;

    for( UINT idi = 0; idi < pAdapterInfo->pDeviceInfoList->Count(); idi++ )
    {
        D3DDeviceInfo* pDeviceInfo = (D3DDeviceInfo*)pAdapterInfo->pDeviceInfoList->GetPtr(idi);
        if (bRequireHAL && pDeviceInfo->DevType != D3DDEVTYPE_HAL)
            continue;
        if (bRequireREF && pDeviceInfo->DevType != D3DDEVTYPE_REF)
            continue;
        for( UINT idc = 0; idc < pDeviceInfo->pDeviceComboList->Count(); idc++ )
        {
            D3DDeviceCombo* pDeviceCombo = (D3DDeviceCombo*)pDeviceInfo->pDeviceComboList->GetPtr(idc);
            bool bAdapterMatchesBB = (pDeviceCombo->BackBufferFormat == pDeviceCombo->AdapterFormat);
            if (!pDeviceCombo->IsWindowed)
                continue;
            if (pDeviceCombo->AdapterFormat != primaryDesktopDisplayMode.Format)
                continue;
            // If we haven't found a compatible DeviceCombo yet, or if this set
            // is better (because it's a HAL, and/or because formats match better),
            // save it
            if( pBestDeviceCombo == NULL || 
                pBestDeviceCombo->DevType != D3DDEVTYPE_HAL && pDeviceCombo->DevType == D3DDEVTYPE_HAL ||
                pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesBB )
            {
                pBestAdapterInfo = pAdapterInfo;
                pBestDeviceInfo = pDeviceInfo;
                pBestDeviceCombo = pDeviceCombo;
                if( pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesBB )
                {
                    // This windowed device combo looks great -- take it
                    goto EndWindowedDeviceComboSearch;
                }
                // Otherwise keep looking for a better windowed device combo
            }
        }
    }
  
EndWindowedDeviceComboSearch:
    if (pBestDeviceCombo == NULL )
        return false;

    m_d3dSettings.pWindowed_AdapterInfo = pBestAdapterInfo;
    m_d3dSettings.pWindowed_DeviceInfo = pBestDeviceInfo;
    m_d3dSettings.pWindowed_DeviceCombo = pBestDeviceCombo;
    m_d3dSettings.IsWindowed = true;
    m_d3dSettings.Windowed_DisplayMode = primaryDesktopDisplayMode;
    m_d3dSettings.Windowed_Width = m_rcWindowClient.right - m_rcWindowClient.left;
    m_d3dSettings.Windowed_Height = m_rcWindowClient.bottom - m_rcWindowClient.top;
    if (m_d3dEnumeration.AppUsesDepthBuffer)
        m_d3dSettings.Windowed_DepthStencilBufferFormat = *(D3DFORMAT*)pBestDeviceCombo->pDepthStencilFormatList->GetPtr(0);
    m_d3dSettings.Windowed_MultisampleType = *(D3DMULTISAMPLE_TYPE*)pBestDeviceCombo->pMultiSampleTypeList->GetPtr(0);
    m_d3dSettings.Windowed_MultisampleQuality = 0;
    m_d3dSettings.Windowed_VertexProcessingType = *(VertexProcessingType*)pBestDeviceCombo->pVertexProcessingTypeList->GetPtr(0);
    m_d3dSettings.Windowed_PresentInterval = *(UINT*)pBestDeviceCombo->pPresentIntervalList->GetPtr(0);
    return true;
}




//-----------------------------------------------------------------------------
// Name: FindBestFullscreenMode()
// Desc: Sets up m_d3dSettings with best available fullscreen mode, subject to 
//       the bRequireHAL and bRequireREF constraints.  Returns false if no such
//       mode can be found.
//-----------------------------------------------------------------------------
bool CD3DVisualization::FindBestFullscreenMode( bool bRequireHAL, bool bRequireREF )
{
    // For fullscreen, default to first HAL DeviceCombo that supports the current desktop 
    // display mode, or any display mode if HAL is not compatible with the desktop mode, or 
    // non-HAL if no HAL is available
    D3DDISPLAYMODE adapterDesktopDisplayMode;
    D3DDISPLAYMODE bestAdapterDesktopDisplayMode;
    D3DDISPLAYMODE bestDisplayMode;
    bestAdapterDesktopDisplayMode.Width = 0;
    bestAdapterDesktopDisplayMode.Height = 0;
    bestAdapterDesktopDisplayMode.Format = D3DFMT_UNKNOWN;
    bestAdapterDesktopDisplayMode.RefreshRate = 0;

    D3DAdapterInfo* pBestAdapterInfo = NULL;
    D3DDeviceInfo* pBestDeviceInfo = NULL;
    D3DDeviceCombo* pBestDeviceCombo = NULL;

    for( UINT iai = 0; iai < m_d3dEnumeration.m_pAdapterInfoList->Count(); iai++ )
    {
        D3DAdapterInfo* pAdapterInfo = (D3DAdapterInfo*)m_d3dEnumeration.m_pAdapterInfoList->GetPtr(iai);
        m_pD3D->GetAdapterDisplayMode( pAdapterInfo->AdapterOrdinal, &adapterDesktopDisplayMode );
        for( UINT idi = 0; idi < pAdapterInfo->pDeviceInfoList->Count(); idi++ )
        {
            D3DDeviceInfo* pDeviceInfo = (D3DDeviceInfo*)pAdapterInfo->pDeviceInfoList->GetPtr(idi);
            if (bRequireHAL && pDeviceInfo->DevType != D3DDEVTYPE_HAL)
                continue;
            if (bRequireREF && pDeviceInfo->DevType != D3DDEVTYPE_REF)
                continue;
            for( UINT idc = 0; idc < pDeviceInfo->pDeviceComboList->Count(); idc++ )
            {
                D3DDeviceCombo* pDeviceCombo = (D3DDeviceCombo*)pDeviceInfo->pDeviceComboList->GetPtr(idc);
                bool bAdapterMatchesBB = (pDeviceCombo->BackBufferFormat == pDeviceCombo->AdapterFormat);
                bool bAdapterMatchesDesktop = (pDeviceCombo->AdapterFormat == adapterDesktopDisplayMode.Format);
                if (pDeviceCombo->IsWindowed)
                    continue;
                // If we haven't found a compatible set yet, or if this set
                // is better (because it's a HAL, and/or because formats match better),
                // save it
                if (pBestDeviceCombo == NULL ||
                    pBestDeviceCombo->DevType != D3DDEVTYPE_HAL && pDeviceInfo->DevType == D3DDEVTYPE_HAL ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && pBestDeviceCombo->AdapterFormat != adapterDesktopDisplayMode.Format && bAdapterMatchesDesktop ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesDesktop && bAdapterMatchesBB )
                {
                    bestAdapterDesktopDisplayMode = adapterDesktopDisplayMode;
                    pBestAdapterInfo = pAdapterInfo;
                    pBestDeviceInfo = pDeviceInfo;
                    pBestDeviceCombo = pDeviceCombo;
                    if (pDeviceInfo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesDesktop && bAdapterMatchesBB)
                    {
                        // This fullscreen device combo looks great -- take it
                        goto EndFullscreenDeviceComboSearch;
                    }
                    // Otherwise keep looking for a better fullscreen device combo
                }
            }
        }
    }
EndFullscreenDeviceComboSearch:
    if (pBestDeviceCombo == NULL)
        return false;

    // Need to find a display mode on the best adapter that uses pBestDeviceCombo->AdapterFormat
    // and is as close to bestAdapterDesktopDisplayMode's res as possible
    bestDisplayMode.Width = 0;
    bestDisplayMode.Height = 0;
    bestDisplayMode.Format = D3DFMT_UNKNOWN;
    bestDisplayMode.RefreshRate = 0;
    for( UINT idm = 0; idm < pBestAdapterInfo->pDisplayModeList->Count(); idm++ )
    {
        D3DDISPLAYMODE* pdm = (D3DDISPLAYMODE*)pBestAdapterInfo->pDisplayModeList->GetPtr(idm);
        if( pdm->Format != pBestDeviceCombo->AdapterFormat )
            continue;
        if( pdm->Width == bestAdapterDesktopDisplayMode.Width &&
            pdm->Height == bestAdapterDesktopDisplayMode.Height && 
            pdm->RefreshRate == bestAdapterDesktopDisplayMode.RefreshRate )
        {
            // found a perfect match, so stop
            bestDisplayMode = *pdm;
            break;
        }
        else if( pdm->Width == bestAdapterDesktopDisplayMode.Width &&
                 pdm->Height == bestAdapterDesktopDisplayMode.Height && 
                 pdm->RefreshRate > bestDisplayMode.RefreshRate )
        {
            // refresh rate doesn't match, but width/height match, so keep this
            // and keep looking
            bestDisplayMode = *pdm;
        }
        else if( pdm->Width == bestAdapterDesktopDisplayMode.Width )
        {
            // width matches, so keep this and keep looking
            bestDisplayMode = *pdm;
        }
        else if( bestDisplayMode.Width == 0 )
        {
            // we don't have anything better yet, so keep this and keep looking
            bestDisplayMode = *pdm;
        }
    }

    m_d3dSettings.pFullscreen_AdapterInfo = pBestAdapterInfo;
    m_d3dSettings.pFullscreen_DeviceInfo = pBestDeviceInfo;
    m_d3dSettings.pFullscreen_DeviceCombo = pBestDeviceCombo;
    m_d3dSettings.IsWindowed = false;
    m_d3dSettings.Fullscreen_DisplayMode = bestDisplayMode;
    if (m_d3dEnumeration.AppUsesDepthBuffer)
        m_d3dSettings.Fullscreen_DepthStencilBufferFormat = *(D3DFORMAT*)pBestDeviceCombo->pDepthStencilFormatList->GetPtr(0);
    m_d3dSettings.Fullscreen_MultisampleType = *(D3DMULTISAMPLE_TYPE*)pBestDeviceCombo->pMultiSampleTypeList->GetPtr(0);
    m_d3dSettings.Fullscreen_MultisampleQuality = 0;
    m_d3dSettings.Fullscreen_VertexProcessingType = *(VertexProcessingType*)pBestDeviceCombo->pVertexProcessingTypeList->GetPtr(0);
    m_d3dSettings.Fullscreen_PresentInterval = D3DPRESENT_INTERVAL_DEFAULT;
    return true;
}




//-----------------------------------------------------------------------------
// Name: ChooseInitialD3DSettings()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::ChooseInitialD3DSettings()
{
    bool bFoundFullscreen = FindBestFullscreenMode( false, false );
    bool bFoundWindowed = FindBestWindowedMode( false, false );
    m_d3dSettings.SetDeviceClip( false );

    if( m_bStartFullscreen && bFoundFullscreen )
        m_d3dSettings.IsWindowed = false;
    if( !bFoundWindowed && bFoundFullscreen )
        m_d3dSettings.IsWindowed = false;

    if( !bFoundFullscreen && !bFoundWindowed )
        return D3DAPPERR_NOCOMPATIBLEDEVICES;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: HandlePossibleSizeChange()
// Desc: Reset the device if the client area size has changed.
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::HandlePossibleSizeChange()
{
    HRESULT hr = S_OK;
    RECT rcClientOld;
    rcClientOld = m_rcWindowClient;

    if( m_bIgnoreSizeChange )
        return S_OK;

    // Update window properties
    GetWindowRect( m_hWnd, &m_rcWindowBounds );
    GetClientRect( m_hWnd, &m_rcWindowClient );

    if( rcClientOld.right - rcClientOld.left !=
        m_rcWindowClient.right - m_rcWindowClient.left ||
        rcClientOld.bottom - rcClientOld.top !=
        m_rcWindowClient.bottom - m_rcWindowClient.top)
    {
        m_d3dpp.BackBufferWidth  = m_rcWindowClient.right - m_rcWindowClient.left;
        m_d3dpp.BackBufferHeight = m_rcWindowClient.bottom - m_rcWindowClient.top;
    
        if( m_pd3dDevice != NULL )
        {
            // Reset the 3D environment
            if( FAILED( hr = Reset3DEnvironment() ) )
            {
                if( hr != D3DERR_OUTOFVIDEOMEMORY )
                    hr = D3DAPPERR_RESETFAILED;
                DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
            }
        }
    }

    return hr;
}




//-----------------------------------------------------------------------------
// Name: Initialize3DEnvironment()
// Desc: Usually this function is not overridden.  Here's what this function does:
//       - Sets the windowed flag to be either windowed or fullscreen
//       - Sets parameters for z-buffer depth and back buffer
//       - Creates the D3D device
//       - Sets the window position (if windowed, that is)
//       - Makes some determinations as to the abilites of the driver (HAL, etc)
//       - Sets up some cursor stuff
//       - Calls InitDeviceObjects()
//       - Calls RestoreDeviceObjects()
//       - If all goes well, m_bActive is set to TRUE, and the function returns
//       - Otherwise, initialization is reattempted using the reference device
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::Initialize3DEnvironment()
{
    HRESULT hr;

    D3DAdapterInfo* pAdapterInfo = m_d3dSettings.PAdapterInfo();
    D3DDeviceInfo* pDeviceInfo = m_d3dSettings.PDeviceInfo();

    m_bWindowed = m_d3dSettings.IsWindowed;

    // Set up the presentation parameters
    BuildPresentParamsFromSettings();

    if( pDeviceInfo->Caps.PrimitiveMiscCaps & D3DPMISCCAPS_NULLREFERENCE )
    {
        // Warn user about null ref device that can't render anything
        DisplayErrorMsg( D3DAPPERR_NULLREFDEVICE, 0 );
    }

    DWORD behaviorFlags;
    if (m_d3dSettings.GetVertexProcessingType() == SOFTWARE_VP)
        behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == MIXED_VP)
        behaviorFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == HARDWARE_VP)
        behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == PURE_HARDWARE_VP)
        behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
    else
        behaviorFlags = 0; // TODO: throw exception

    // Create the device
    hr = m_pD3D->CreateDevice( m_d3dSettings.AdapterOrdinal(), pDeviceInfo->DevType,
                               m_hWndFocus, behaviorFlags, &m_d3dpp,
                               &m_pd3dDevice );

    if( SUCCEEDED(hr) )
    {
        // Store device Caps
        m_pd3dDevice->GetDeviceCaps( &m_d3dCaps );
        m_dwCreateFlags = behaviorFlags;

        // Store device description
        if( pDeviceInfo->DevType == D3DDEVTYPE_REF )
            lstrcpy( m_strDeviceStats, TEXT("REF") );
        else if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
            lstrcpy( m_strDeviceStats, TEXT("HAL") );
        else if( pDeviceInfo->DevType == D3DDEVTYPE_SW )
            lstrcpy( m_strDeviceStats, TEXT("SW") );

        if( behaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING &&
            behaviorFlags & D3DCREATE_PUREDEVICE )
        {
            if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
                lstrcat( m_strDeviceStats, TEXT(" (pure hw vp)") );
            else
                lstrcat( m_strDeviceStats, TEXT(" (simulated pure hw vp)") );
        }
        else if( behaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING )
        {
            if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
                lstrcat( m_strDeviceStats, TEXT(" (hw vp)") );
            else
                lstrcat( m_strDeviceStats, TEXT(" (simulated hw vp)") );
        }
        else if( behaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING )
        {
            if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
                lstrcat( m_strDeviceStats, TEXT(" (mixed vp)") );
            else
                lstrcat( m_strDeviceStats, TEXT(" (simulated mixed vp)") );
        }
        else if( behaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING )
        {
            lstrcat( m_strDeviceStats, TEXT(" (sw vp)") );
        }

        if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
        {
            // Be sure not to overflow m_strDeviceStats when appending the adapter 
            // description, since it can be long.  Note that the adapter description
            // is initially CHAR and must be converted to TCHAR.
            lstrcat( m_strDeviceStats, TEXT(": ") );
            const int cchDesc = sizeof(pAdapterInfo->AdapterIdentifier.Description);
            TCHAR szDescription[cchDesc];
            DXUtil_ConvertAnsiStringToGenericCch( szDescription, 
                pAdapterInfo->AdapterIdentifier.Description, cchDesc );
            int maxAppend = sizeof(m_strDeviceStats) / sizeof(TCHAR) -
                lstrlen( m_strDeviceStats ) - 1;
            _tcsncat( m_strDeviceStats, szDescription, maxAppend );
        }

        // Store render target surface desc
        LPDIRECT3DSURFACE9 pBackBuffer = NULL;
        m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
        pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
        pBackBuffer->Release();

        // Set up the fullscreen cursor
        if( m_bShowCursorWhenFullscreen && !m_bWindowed )
        {
            HCURSOR hCursor;
#ifdef _WIN64
            hCursor = (HCURSOR)GetClassLongPtr( m_hWnd, GCLP_HCURSOR );
#else
            hCursor = (HCURSOR)ULongToHandle( GetClassLong( m_hWnd, GCL_HCURSOR ) );
#endif
            D3DUtil_SetDeviceCursor( m_pd3dDevice, hCursor, true );
            m_pd3dDevice->ShowCursor( true );
        }

        // Confine cursor to fullscreen window
        if( m_bClipCursorWhenFullscreen )
        {
            if (!m_bWindowed )
            {
                RECT rcWindow;
                GetWindowRect( m_hWnd, &rcWindow );
                ClipCursor( &rcWindow );
            }
            else
            {
                ClipCursor( NULL );
            }
        }

        // Initialize the app's device-dependent objects
        hr = InitDeviceObjects();
        if( FAILED(hr) )
        {
            DeleteDeviceObjects();
        }
        else
        {
            m_bDeviceObjectsInited = true;

            // If running in skin mode, create an alternate render target
            if( m_bSkinMode )
            {
                hr = CreateRenderTarget();
                if( FAILED(hr) )
                    return hr;
            }

            hr = RestoreDeviceObjects();
            if( FAILED(hr) )
            {
                ReleaseRenderTarget();
                InvalidateDeviceObjects();
            }
            else
            {
                m_bDeviceObjectsRestored = true;
                return S_OK;
            }
        }

        // Cleanup before we try again
        Cleanup3DEnvironment();
    }

    // If that failed, fall back to the reference rasterizer
    if( hr != D3DAPPERR_MEDIANOTFOUND && 
        hr != HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) && 
        pDeviceInfo->DevType == D3DDEVTYPE_HAL )
    {
        // Let the user know we are switching from HAL to the reference rasterizer
        DisplayErrorMsg( hr, MSGWARN_SWITCHEDTOREF );

        hr = Initialize3DEnvironment();
    }
    return hr;
}



//-----------------------------------------------------------------------------
// Name: CreateRenderTarget()
// Desc: Create an alternate render target for visualizations running in skin
//       mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::CreateRenderTarget()
{
    HRESULT hr = S_OK;

    hr = m_pd3dDevice->CreateTexture( m_RectTarget.right - m_RectTarget.left,
                                      m_RectTarget.bottom - m_RectTarget.top,
                                      1, D3DUSAGE_RENDERTARGET,
                                      D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, 
                                      &m_pTexRenderTarget, NULL );

    if( FAILED(hr) )
        return hr;

    hr = m_pd3dDevice->CreateTexture( m_RectTarget.right - m_RectTarget.left,
                                      m_RectTarget.bottom - m_RectTarget.top,
                                      1, 0,
                                      D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, 
                                      &m_pTexScratch, NULL );

    if( FAILED(hr) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: BuildPresentParamsFromSettings()
// Desc:
//-----------------------------------------------------------------------------
void CD3DVisualization::BuildPresentParamsFromSettings()
{
    m_d3dpp.Windowed               = m_d3dSettings.IsWindowed;
    m_d3dpp.BackBufferCount        = 1;
    m_d3dpp.MultiSampleType        = m_d3dSettings.MultisampleType();
    m_d3dpp.MultiSampleQuality     = m_d3dSettings.MultisampleQuality();
    m_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = m_d3dEnumeration.AppUsesDepthBuffer;
    m_d3dpp.hDeviceWindow          = m_hWnd;
    if( m_d3dEnumeration.AppUsesDepthBuffer )
    {
        m_d3dpp.Flags              = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
        m_d3dpp.AutoDepthStencilFormat = m_d3dSettings.DepthStencilBufferFormat();
    }
    else
    {
        m_d3dpp.Flags              = 0;
    }

    if ( m_d3dSettings.DeviceClip() )
        m_d3dpp.Flags |= D3DPRESENTFLAG_DEVICECLIP;

    if( m_bWindowed )
    {
        m_d3dpp.BackBufferWidth  = m_rcWindowClient.right - m_rcWindowClient.left;
        m_d3dpp.BackBufferHeight = m_rcWindowClient.bottom - m_rcWindowClient.top;
        m_d3dpp.BackBufferFormat = m_d3dSettings.PDeviceCombo()->BackBufferFormat;
        m_d3dpp.FullScreen_RefreshRateInHz = 0;
        m_d3dpp.PresentationInterval = m_d3dSettings.PresentInterval();
    }
    else
    {
        m_d3dpp.BackBufferWidth  = m_d3dSettings.DisplayMode().Width;
        m_d3dpp.BackBufferHeight = m_d3dSettings.DisplayMode().Height;
        m_d3dpp.BackBufferFormat = m_d3dSettings.PDeviceCombo()->BackBufferFormat;
        m_d3dpp.FullScreen_RefreshRateInHz = m_d3dSettings.Fullscreen_DisplayMode.RefreshRate;
        m_d3dpp.PresentationInterval = m_d3dSettings.PresentInterval();

        if( m_bAllowDialogBoxMode )
        {
            // Make the back buffers lockable in fullscreen mode
            // so we can show dialog boxes via SetDialogBoxMode() 
            // but since lockable back buffers incur a performance cost on 
            // some graphics hardware configurations we'll only 
            // enable lockable backbuffers where SetDialogBoxMode() would work.
            if ( (m_d3dpp.BackBufferFormat == D3DFMT_X1R5G5B5 || m_d3dpp.BackBufferFormat == D3DFMT_R5G6B5 || m_d3dpp.BackBufferFormat == D3DFMT_X8R8G8B8 ) &&
                ( m_d3dpp.MultiSampleType == D3DMULTISAMPLE_NONE ) &&
                ( m_d3dpp.SwapEffect == D3DSWAPEFFECT_DISCARD ) )
            {
                m_d3dpp.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
            }
        }
    }
}




//-----------------------------------------------------------------------------
// Name: Reset3DEnvironment()
// Desc: Usually this function is not overridden.  Here's what this function does:
//       - Sets the windowed flag to be either windowed or fullscreen
//       - Sets parameters for z-buffer depth and back buffer
//       - Creates the D3D device
//       - Sets the window position (if windowed, that is)
//       - Makes some determinations as to the abilites of the driver (HAL, etc)
//       - Sets up some cursor stuff
//       - Calls InitDeviceObjects()
//       - Calls RestoreDeviceObjects()
//       - If all goes well, m_bActive is set to TRUE, and the function returns
//       - Otherwise, initialization is reattempted using the reference device
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::Reset3DEnvironment()
{
    HRESULT hr;

    // Release all vidmem objects
    if( m_bDeviceObjectsRestored )
    {
        m_bDeviceObjectsRestored = false;
        
        ReleaseRenderTarget();
        InvalidateDeviceObjects();
    }
    // Reset the device
    if( FAILED( hr = m_pd3dDevice->Reset( &m_d3dpp ) ) )
        return hr;

    // Store render target surface desc
    LPDIRECT3DSURFACE9 pBackBuffer;
    m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
    pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
    pBackBuffer->Release();

    // Set up the fullscreen cursor
    if( m_bShowCursorWhenFullscreen && !m_bWindowed )
    {
        HCURSOR hCursor;
#ifdef _WIN64
        hCursor = (HCURSOR)GetClassLongPtr( m_hWnd, GCLP_HCURSOR );
#else
        hCursor = (HCURSOR)ULongToHandle( GetClassLong( m_hWnd, GCL_HCURSOR ) );
#endif
        D3DUtil_SetDeviceCursor( m_pd3dDevice, hCursor, true );
        m_pd3dDevice->ShowCursor( true );
    }

    // Confine cursor to fullscreen window
    if( m_bClipCursorWhenFullscreen )
    {
        if (!m_bWindowed )
        {
            RECT rcWindow;
            GetWindowRect( m_hWnd, &rcWindow );
            ClipCursor( &rcWindow );
        }
        else
        {
            ClipCursor( NULL );
        }
    }

    // If running in skin mode, create an alternate render target
    if( m_bSkinMode )
    {
        hr = CreateRenderTarget();
        if( FAILED(hr) )
            return hr;
    }

    // Initialize the app's device-dependent objects
    hr = RestoreDeviceObjects();
    if( FAILED(hr) )
    {
        ReleaseRenderTarget();
        InvalidateDeviceObjects();
        return hr;
    }
    m_bDeviceObjectsRestored = true;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ToggleFullScreen()
// Desc: Called when user toggles between fullscreen mode and windowed mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::ToggleFullscreen()
{
    HRESULT hr;
    int AdapterOrdinalOld = m_d3dSettings.AdapterOrdinal();
    D3DDEVTYPE DevTypeOld = m_d3dSettings.DevType();

    m_bIgnoreSizeChange = true;

    // Toggle the windowed state
    m_bWindowed = !m_bWindowed;
    m_d3dSettings.IsWindowed = m_bWindowed;

    
    // If AdapterOrdinal and DevType are the same, we can just do a Reset().
    // If they've changed, we need to do a complete device teardown/rebuild.
    if (m_d3dSettings.AdapterOrdinal() == AdapterOrdinalOld &&
        m_d3dSettings.DevType() == DevTypeOld)
    {
        // Reset the 3D device
        BuildPresentParamsFromSettings();
        hr = Reset3DEnvironment();
    }
    else
    {
        Cleanup3DEnvironment();
        hr = Initialize3DEnvironment();
    }
    if( FAILED( hr ) )
    {
        if( hr != D3DERR_OUTOFVIDEOMEMORY )
            hr = D3DAPPERR_RESETFAILED;
        m_bIgnoreSizeChange = false;
        if( !m_bWindowed )
        {
            // Restore window type to windowed mode
            m_bWindowed = !m_bWindowed;
            m_d3dSettings.IsWindowed = m_bWindowed;
        }
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    m_bIgnoreSizeChange = false;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render3DEnvironment()
// Desc: Here's what this function does:
//       - Checks to make sure app is still active (if fullscreen, etc)
//       - Checks to see if it is time to draw with DXUtil_Timer, if not, it just returns S_OK
//       - Calls FrameMove() to recalculate new positions
//       - Calls Render() to draw the new frame
//       - Updates some frame count statistics
//       - Calls m_pd3dDevice->Present() to display the rendered frame.
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::Render3DEnvironment()
{
    HRESULT hr;

    if( m_bDeviceLost )
    {
        // Test the cooperative level to see if it's okay to render
        if( FAILED( hr = m_pd3dDevice->TestCooperativeLevel() ) )
        {
            // If the device was lost, do not render until we get it back
            if( D3DERR_DEVICELOST == hr )
                return S_OK;

            // Check if the device needs to be reset.
            if( D3DERR_DEVICENOTRESET == hr )
            {
                // If we are windowed, read the desktop mode and use the same format for
                // the back buffer
                if( m_bWindowed )
                {
                    D3DAdapterInfo* pAdapterInfo = m_d3dSettings.PAdapterInfo();
                    m_pD3D->GetAdapterDisplayMode( pAdapterInfo->AdapterOrdinal, &m_d3dSettings.Windowed_DisplayMode );
                    m_d3dpp.BackBufferFormat = m_d3dSettings.Windowed_DisplayMode.Format;
                }

                if( FAILED( hr = Reset3DEnvironment() ) )
                    return hr;
            }
            return hr;
        }
        m_bDeviceLost = false;
    }

    // Get the app's time, in seconds. Skip rendering if no time elapsed
    FLOAT fAppTime        = DXUtil_Timer( TIMER_GETAPPTIME );
    FLOAT fElapsedAppTime = DXUtil_Timer( TIMER_GETELAPSEDTIME );
    if( ( 0.0f == fElapsedAppTime ) && m_bFrameMoving )
        return S_OK;

    // FrameMove (animate) the scene
    if( m_bFrameMoving )
    {
        m_fTime        = fAppTime;
        m_fElapsedTime = fElapsedAppTime;
        
        // Frame move the scene
        if( FAILED( hr = FrameMove() ) )
            return hr;
    }

    // Render the scene as normal
    if( FAILED( hr = Render() ) )
        return hr;

    UpdateStats();

    // Show the frame on the primary surface.
    hr = m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    if( D3DERR_DEVICELOST == hr )
        m_bDeviceLost = true;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: UpdateStats()
// Desc: 
//-----------------------------------------------------------------------------
void CD3DVisualization::UpdateStats()
{
    // Keep track of the frame count
    static FLOAT fLastTime = 0.0f;
    static DWORD dwFrames  = 0;
    FLOAT fTime = DXUtil_Timer( TIMER_GETABSOLUTETIME );
    ++dwFrames;

    // Update the scene stats once per second
    if( fTime - fLastTime > 1.0f )
    {
        m_fFPS    = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames  = 0;

        TCHAR strFmt[100];
        D3DFORMAT fmtAdapter = m_d3dSettings.DisplayMode().Format;
        if( fmtAdapter == m_d3dsdBackBuffer.Format )
        {
            lstrcpyn( strFmt, D3DUtil_D3DFormatToString( fmtAdapter, false ), 100 );
        }
        else
        {
            _sntprintf( strFmt, 100, TEXT("backbuf %s, adapter %s"), 
                D3DUtil_D3DFormatToString( m_d3dsdBackBuffer.Format, false ), 
                D3DUtil_D3DFormatToString( fmtAdapter, false ) );
        }
        strFmt[99] = TEXT('\0');

        TCHAR strDepthFmt[100];
        if( m_d3dEnumeration.AppUsesDepthBuffer )
        {
            _sntprintf( strDepthFmt, 100, TEXT(" (%s)"), 
                D3DUtil_D3DFormatToString( m_d3dSettings.DepthStencilBufferFormat(), false ) );
            strDepthFmt[99] = TEXT('\0');
        }
        else
        {
            // No depth buffer
            strDepthFmt[0] = TEXT('\0');
        }

        TCHAR* pstrMultiSample;
        switch( m_d3dSettings.MultisampleType() )
        {
        case D3DMULTISAMPLE_NONMASKABLE:  pstrMultiSample = TEXT(" (Nonmaskable Multisample)"); break;
        case D3DMULTISAMPLE_2_SAMPLES:  pstrMultiSample = TEXT(" (2x Multisample)"); break;
        case D3DMULTISAMPLE_3_SAMPLES:  pstrMultiSample = TEXT(" (3x Multisample)"); break;
        case D3DMULTISAMPLE_4_SAMPLES:  pstrMultiSample = TEXT(" (4x Multisample)"); break;
        case D3DMULTISAMPLE_5_SAMPLES:  pstrMultiSample = TEXT(" (5x Multisample)"); break;
        case D3DMULTISAMPLE_6_SAMPLES:  pstrMultiSample = TEXT(" (6x Multisample)"); break;
        case D3DMULTISAMPLE_7_SAMPLES:  pstrMultiSample = TEXT(" (7x Multisample)"); break;
        case D3DMULTISAMPLE_8_SAMPLES:  pstrMultiSample = TEXT(" (8x Multisample)"); break;
        case D3DMULTISAMPLE_9_SAMPLES:  pstrMultiSample = TEXT(" (9x Multisample)"); break;
        case D3DMULTISAMPLE_10_SAMPLES: pstrMultiSample = TEXT(" (10x Multisample)"); break;
        case D3DMULTISAMPLE_11_SAMPLES: pstrMultiSample = TEXT(" (11x Multisample)"); break;
        case D3DMULTISAMPLE_12_SAMPLES: pstrMultiSample = TEXT(" (12x Multisample)"); break;
        case D3DMULTISAMPLE_13_SAMPLES: pstrMultiSample = TEXT(" (13x Multisample)"); break;
        case D3DMULTISAMPLE_14_SAMPLES: pstrMultiSample = TEXT(" (14x Multisample)"); break;
        case D3DMULTISAMPLE_15_SAMPLES: pstrMultiSample = TEXT(" (15x Multisample)"); break;
        case D3DMULTISAMPLE_16_SAMPLES: pstrMultiSample = TEXT(" (16x Multisample)"); break;
        default:                        pstrMultiSample = TEXT(""); break;
        }

        const int cchMaxFrameStats = sizeof(m_strFrameStats) / sizeof(TCHAR);
        _sntprintf( m_strFrameStats, cchMaxFrameStats, _T("%.02f fps (%dx%d), %s%s%s"), m_fFPS,
                    m_d3dsdBackBuffer.Width, m_d3dsdBackBuffer.Height,
                    strFmt, strDepthFmt, pstrMultiSample );
        m_strFrameStats[cchMaxFrameStats - 1] = TEXT('\0');
    }
}





//-----------------------------------------------------------------------------
// Name: Cleanup3DEnvironment()
// Desc: Cleanup scene objects
//-----------------------------------------------------------------------------
void CD3DVisualization::Cleanup3DEnvironment()
{
    if( m_pd3dDevice != NULL )
    {
        if( m_bDeviceObjectsRestored )
        {
            m_bDeviceObjectsRestored = false;
            ReleaseRenderTarget();
            InvalidateDeviceObjects();
        }
        if( m_bDeviceObjectsInited )
        {
            m_bDeviceObjectsInited = false;
            DeleteDeviceObjects();
        }

        if( m_pd3dDevice->Release() > 0 )
            DisplayErrorMsg( D3DAPPERR_NONZEROREFCOUNT, MSGERR_APPMUSTEXIT );
        m_pd3dDevice = NULL;
    }
}




//-----------------------------------------------------------------------------
// Name: ReleaseRenderTarget()
// Desc: Free the alternate render target for visualizations running in skin
//       mode
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::ReleaseRenderTarget()
{
    SAFE_RELEASE( m_pTexRenderTarget );
    SAFE_RELEASE( m_pTexScratch );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DisplayErrorMsg()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
HRESULT CD3DVisualization::DisplayErrorMsg( HRESULT hr, DWORD dwType )
{
    TCHAR strMsg[512];

    switch( hr )
    {
        case D3DAPPERR_NODIRECT3D:
            _tcscpy( strMsg, _T("Could not initialize Direct3D. You may\n")
                             _T("want to check that the latest version of\n")
                             _T("DirectX is correctly installed on your\n")
                             _T("system.  Also make sure that this program\n")
                             _T("was compiled with header files that match\n")
                             _T("the installed DirectX DLLs.") );
            break;

        case D3DAPPERR_NOCOMPATIBLEDEVICES:
            _tcscpy( strMsg, _T("Could not find any compatible Direct3D\n")
                             _T("devices.") );
            break;

        case D3DAPPERR_NOWINDOWABLEDEVICES:
            _tcscpy( strMsg, _T("This sample cannot run in a desktop\n")
                             _T("window with the current display settings.\n")
                             _T("Please change your desktop settings to a\n")
                             _T("16- or 32-bit display mode and re-run this\n")
                             _T("sample.") );
            break;

        case D3DAPPERR_NOHARDWAREDEVICE:
            _tcscpy( strMsg, _T("No hardware-accelerated Direct3D devices\n")
                             _T("were found.") );
            break;

        case D3DAPPERR_HALNOTCOMPATIBLE:
            _tcscpy( strMsg, _T("This sample requires functionality that is\n")
                             _T("not available on your Direct3D hardware\n")
                             _T("accelerator.") );
            break;

        case D3DAPPERR_NOWINDOWEDHAL:
            _tcscpy( strMsg, _T("Your Direct3D hardware accelerator cannot\n")
                             _T("render into a window.\n")
                             _T("Press F2 while the app is running to see a\n")
                             _T("list of available devices and modes.") );
            break;

        case D3DAPPERR_NODESKTOPHAL:
            _tcscpy( strMsg, _T("Your Direct3D hardware accelerator cannot\n")
                             _T("render into a window with the current\n")
                             _T("desktop display settings.\n")
                             _T("Press F2 while the app is running to see a\n")
                             _T("list of available devices and modes.") );
            break;

        case D3DAPPERR_NOHALTHISMODE:
            _tcscpy( strMsg, _T("This sample requires functionality that is\n")
                             _T("not available on your Direct3D hardware\n")
                             _T("accelerator with the current desktop display\n")
                             _T("settings.\n")
                             _T("Press F2 while the app is running to see a\n")
                             _T("list of available devices and modes.") );
            break;

        case D3DAPPERR_MEDIANOTFOUND:
        case 0x80070002: // HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ):
            _tcscpy( strMsg, _T("Could not load required media." ) );
            break;

        case D3DAPPERR_RESETFAILED:
            _tcscpy( strMsg, _T("Could not reset the Direct3D device." ) );
            break;

        case D3DAPPERR_NONZEROREFCOUNT:
            _tcscpy( strMsg, _T("A D3D object has a non-zero reference\n")
                             _T("count (meaning things were not properly\n")
                             _T("cleaned up).") );
            break;

        case D3DAPPERR_NULLREFDEVICE:
            _tcscpy( strMsg, _T("Warning: Nothing will be rendered.\n")
                             _T("The reference rendering device was selected, but your\n")
                             _T("computer only has a reduced-functionality reference device\n")
                             _T("installed.  Install the DirectX SDK to get the full\n")
                             _T("reference device.\n") );
            break;

        case E_OUTOFMEMORY:
            _tcscpy( strMsg, _T("Not enough memory.") );
            break;

        case D3DERR_OUTOFVIDEOMEMORY:
            _tcscpy( strMsg, _T("Not enough video memory.") );
            break;

        case D3DERR_DRIVERINTERNALERROR:
            _tcscpy( strMsg, _T("A serious problem occured inside the display driver.") );
            break;

        default:
            _tcscpy( strMsg, _T("Generic application error. Enable\n")
                             _T("debug output for detailed information.") );
    }

    _tcsncpy( m_strErrorMessage, strMsg, MAX_PATH-1 );
    OutputDebugString( strMsg );

    return hr;
}




