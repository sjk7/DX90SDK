//-----------------------------------------------------------------------------
// File: Bars.cpp
//
// Desc: An example of how to create a Windows Media Player visualization using
//       Direct3D graphics
//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "Bars.h"




//-----------------------------------------------------------------------------
// Name: StaticDialogProc()
// Desc: Dialog procedure helper function
//-----------------------------------------------------------------------------
INT_PTR CALLBACK StaticDialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    static CBars *pThis = NULL;
    
    if( msg == WM_INITDIALOG )
    {   
        PROPSHEETPAGE *pPage = (PROPSHEETPAGE*) lParam;
        pThis = (CBars*) pPage->lParam;
    }

    if( pThis )
        return pThis->DialogProc( hDlg, msg, wParam, lParam );

    return NULL;
}




//-----------------------------------------------------------------------------
// Name: CBars()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CBars::CBars()
{
    m_pVBBars = NULL;
    m_pIBBars = NULL;

    ZeroMemory( m_nLevels, sizeof(m_nLevels) );    

    // Load the user-configurable options from the registry
    HKEY hKey = NULL;
    RegOpenKeyEx( HKEY_CURRENT_USER, BARS_REGKEYNAME, 0, KEY_READ, &hKey );

    DXUtil_ReadIntRegKey(  hKey, TEXT("BarWidth"), &m_dwBarWidth, DEFAULT_BAR_WIDTH );
    DXUtil_ReadIntRegKey(  hKey, TEXT("BarSpacing"), &m_dwBarSpacing, DEFAULT_BAR_SPACING );

    RegCloseKey( hKey );
}




//-----------------------------------------------------------------------------
// Name: GetTitle()
// Desc: Retrieves the display name for this visualization preset
//-----------------------------------------------------------------------------
HRESULT CBars::GetTitle(TCHAR* strTitle, DWORD dwSize)
{
    _tcsncpy( strTitle, TEXT("Bars"), dwSize );
    strTitle[ dwSize-1 ] = 0;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetCapabilities()
// Desc: Retrieves the capabilities for this visualization preset
// Flags that can be returned are:
//	EFFECT_CANGOFULLSCREEN		-- effect supports full-screen rendering
//	EFFECT_HASPROPERTYPAGE		-- effect supports a property page
//-----------------------------------------------------------------------------
HRESULT CBars::GetCapabilities(DWORD * pdwCapabilities)
{
    *pdwCapabilities = EFFECT_CANGOFULLSCREEN | EFFECT_HASPROPERTYPAGE;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetPropSheetPage()
// Desc: Fills the PROPSHEETPAGE structure to define the property sheet page
//       for this visualization's user-configurable options
//-----------------------------------------------------------------------------
HRESULT CBars::GetPropSheetPage( PROPSHEETPAGE* pPage )
{
    ZeroMemory( pPage, sizeof(PROPSHEETPAGE) );

    // Initialize the property sheet page values. This structure will
    // be used to add this visualization's property page to the property
    // sheet collection displayed when the user displays the visualization
    // properties.
    pPage->dwSize = sizeof(PROPSHEETPAGE);
    pPage->dwFlags = PSP_DEFAULT;
    pPage->hInstance = g_hInstance;
    pPage->pszTemplate = MAKEINTRESOURCE( IDD_BARS );
    pPage->pfnDlgProc = StaticDialogProc;
    pPage->lParam = (LPARAM) this;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CBars::OneTimeSceneInit()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: This creates all device-dependent managed objects, such as managed
//       textures and managed vertex buffers.
//-----------------------------------------------------------------------------
HRESULT CBars::InitDeviceObjects()
{
    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Restore device-memory objects and state after a device is created or
//       resized.
//-----------------------------------------------------------------------------
HRESULT CBars::RestoreDeviceObjects()
{
    HRESULT hr = S_OK;
    
    // Release previous resources
    SAFE_RELEASE( m_pVBBars );
    SAFE_RELEASE( m_pIBBars );

    int nNumOfBars = GetNumOfBars();

    // Create the vertex buffer
    hr = m_pd3dDevice->CreateVertexBuffer( sizeof(BARVERTEX) * 4 * nNumOfBars, 
                                           D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                           D3DFVF_BARVERTEX,
                                           D3DPOOL_DEFAULT,
                                           &m_pVBBars,
                                           NULL );

    if( FAILED(hr) )
        return hr;

    // Create the index buffer
    hr = m_pd3dDevice->CreateIndexBuffer( sizeof(BARVERTEX) * 6 * nNumOfBars, 
                                          D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                                          D3DFMT_INDEX16,
                                          D3DPOOL_DEFAULT,
                                          &m_pIBBars,
                                          NULL );

    if( FAILED(hr) )
        return hr;

    // Fill the index buffer
    WORD *pIndex;
    hr = m_pIBBars->Lock( 0, 0, (VOID**) &pIndex, D3DLOCK_DISCARD );
    if( FAILED(hr) )
        return hr;

    for( int i=0; i < nNumOfBars; i++ )
    {
        *pIndex++ = ( i * 4 ) + 0;
        *pIndex++ = ( i * 4 ) + 1;
        *pIndex++ = ( i * 4 ) + 2;
        *pIndex++ = ( i * 4 ) + 2;
        *pIndex++ = ( i * 4 ) + 1;
        *pIndex++ = ( i * 4 ) + 3;
    }

    hr = m_pIBBars->Unlock();
    if( FAILED(hr) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the device-dependent objects are about to be lost.
//-----------------------------------------------------------------------------
HRESULT CBars::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pVBBars );
    SAFE_RELEASE( m_pIBBars );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT CBars::DeleteDeviceObjects()
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CBars::FinalCleanup()
{
    // Save the user-configurable options
    HKEY hKey = NULL;

    RegCreateKeyEx( HKEY_CURRENT_USER, BARS_REGKEYNAME, 0, NULL, 
                    REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
                    &hKey, NULL );

    DXUtil_WriteIntRegKey(  hKey, TEXT("BarWidth"), m_dwBarWidth );
    DXUtil_WriteIntRegKey(  hKey, TEXT("BarSpacing"), m_dwBarSpacing );

    RegCloseKey( hKey );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CBars::FrameMove()
{
    HRESULT hr = S_OK;
    
    int nNumOfBars = GetNumOfBars();

    // Fill the vertex buffer
    BARVERTEX *pVertex;
    hr = m_pVBBars->Lock( 0, 0, (VOID**) &pVertex, D3DLOCK_DISCARD );
    if( FAILED(hr) )
        return hr;

    // For each bar were drawing...
    for( int i=0; i < nNumOfBars; i++ )
    {
        // Fetch the nearest frequency level. This visualization simply uses only
        // the right-channel value.
        int level = m_pAudioLevels->frequency[1][ i * (SA_BUFFER_SIZE-1) / nNumOfBars ];
        
        // As a simple visual enhancement, force the bar to fall at a constant
        // rate when the current level is less than the currently displayed level
        if( level < m_nLevels[i] )
            level = m_nLevels[i] - (int) (150 * m_fElapsedTime);
     
        // Save this level for the next pass
        m_nLevels[i] = level;

        // Determine the top x,y coordinate of the current bar
        float x = (float) i * ( m_dwBarWidth + m_dwBarSpacing );
        float y = m_d3dsdBackBuffer.Height - ( m_d3dsdBackBuffer.Height / 256.0f ) * level;

        // Set the top left vertex for the current bar
        pVertex->x = x;
        pVertex->y = y;
        pVertex->z = 0.5f;
        pVertex->rhw = 1.0f;
        pVertex->color = D3DCOLOR_RGBA( 255, 255, 255, 255 );
        pVertex++;

        // Set the top right vertex for the current bar
        pVertex->x = x + m_dwBarWidth;
        pVertex->y = y;
        pVertex->z = 0.5f;
        pVertex->rhw = 1.0f;
        pVertex->color = D3DCOLOR_RGBA( 255, 255, 255, 255 );
        pVertex++;

        // Set the bottom left vertex for the current bar
        pVertex->x = x;
        pVertex->y = (float) m_d3dsdBackBuffer.Height;
        pVertex->z = 0.5f;
        pVertex->rhw = 1.0f;
        pVertex->color = D3DCOLOR_RGBA( 255, level, level, 255 );
        pVertex++;

        // Set the bottom right vertex for the current bar
        pVertex->x = x + m_dwBarWidth;
        pVertex->y = (float) m_d3dsdBackBuffer.Height;
        pVertex->z = 0.5f;
        pVertex->rhw = 1.0f;
        pVertex->color = D3DCOLOR_RGBA( 255, level, level, 255 );
        pVertex++;
        
    }

    hr = m_pVBBars->Unlock();
    if( FAILED(hr) )
        return hr;

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CBars::Render()
{
    HRESULT hr = S_OK;
    
    // Clear the backbuffer to a black color
    m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0,0,0,0), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        
        m_pd3dDevice->SetIndices( m_pIBBars );
        m_pd3dDevice->SetStreamSource( 0, m_pVBBars, 0, sizeof(BARVERTEX) );
        m_pd3dDevice->SetFVF( D3DFVF_BARVERTEX );
        
        hr = m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 
                                                 4 * GetNumOfBars(), 0, 2 * GetNumOfBars() );

        // End the scene
        m_pd3dDevice->EndScene();
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device initialization, this code checks the device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT CBars::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
                                       D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DialogProc()
// Desc: Handles dialog messages for this visualization's property sheet page
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CBars::DialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    TCHAR strBuf[256] = {0};

    switch( msg ) 
    {
        case WM_COMMAND:
        {
            switch( HIWORD(wParam) )
            {
                case EN_CHANGE:
                {
                    // Validate the current configuration values
                    GetDlgItemText( hDlg, IDC_BAR_WIDTH, strBuf, 256 );
                    int nBarWidth = _ttoi( strBuf );

                    if( nBarWidth < 1 )
                        SetDlgItemText( hDlg, IDC_BAR_WIDTH, TEXT("1") );
                    else if( nBarWidth > 50 )
                        SetDlgItemText( hDlg, IDC_BAR_WIDTH, TEXT("50") );

                    GetDlgItemText( hDlg, IDC_BAR_SPACING, strBuf, 256 );
                    int nBarSpacing = _ttoi( strBuf );

                    if( nBarSpacing < 0 )
                        SetDlgItemText( hDlg, IDC_BAR_SPACING, TEXT("0") );
                    else if( nBarSpacing > 20 )
                        SetDlgItemText( hDlg, IDC_BAR_SPACING, TEXT("20") );

                    return TRUE;
                }
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
            switch( pnmh->code )
            {
                case PSN_SETACTIVE:
                {
                    // Initialize the values
                    _itot( m_dwBarWidth, strBuf, 10 );
                    SetDlgItemText( hDlg, IDC_BAR_WIDTH, strBuf );
                    
                    _itot( m_dwBarSpacing, strBuf, 10 );
                    SetDlgItemText( hDlg, IDC_BAR_SPACING, strBuf );
                    
#ifdef _WIN64
                        SetWindowLongPtr( hDlg, DWLP_MSGRESULT, 0 );
#else
                        SetWindowLong( hDlg, DWL_MSGRESULT, 0 );
#endif //_WIN64
                    return TRUE;
                }

                case PSN_APPLY:
                {
                    // Apply the user's settings
                    GetDlgItemText( hDlg, IDC_BAR_WIDTH, strBuf, 256 );
                    m_dwBarWidth = _ttoi( strBuf );

                    GetDlgItemText( hDlg, IDC_BAR_SPACING, strBuf, 256 );
                    m_dwBarSpacing = _ttoi( strBuf );

                    if( NULL != m_pd3dDevice )
                        Reset3DEnvironment();
#ifdef _WIN64
                    SetWindowLongPtr( hDlg, DWLP_MSGRESULT, FALSE );
#else  
                    SetWindowLong( hDlg, DWL_MSGRESULT, FALSE );
#endif //_WIN64
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


