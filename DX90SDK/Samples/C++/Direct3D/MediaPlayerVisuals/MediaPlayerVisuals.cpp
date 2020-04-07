/////////////////////////////////////////////////////////////////////////////
//
// MediaPlayerVisuals.cpp : Implementation of CMediaPlayerVisuals
//
// Execution of the plug-in requires the Windows Media Player 9 Series.
// Compilation of the source code requires the Windows Media 9 Series SDK.
//
// Each of these components is available as a free download from the Windows
// Media Player website: 
//    http://www.microsoft.com/windows/windowsmedia
//
// To compile the source code:
//    1: Locate the directory search path settings inside your build environment.
//          For Visual Studio 6: "Tools->Options->Directories"
//          For Visual Studio.NET: "Tools->Options->Projects->VC++ Directories"
//    2: Add the Windows Media 9 Series SDK include directory to the include
//       directories search path (commonly "C:\WMSDK\WMPSDK9\include")
//
// Copyright (c) Microsoft Corporation. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <stdio.h>
#include "MediaPlayerVisuals.h"
#include "SpectrumWheel.h"
#include "Bars.h"

extern HINSTANCE g_hInstance;

BOOL g_bSkinMode = FALSE;

// Global list of visualizations. To add more visualizations to this collection,
// place the instantiation here.
CD3DVisualization* g_rpVisualizations[] =
{
    new CSpectrumWheel(),
    new CBars(),
};

/////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::CMediaPlayerVisuals
// Constructor

CMediaPlayerVisuals::CMediaPlayerVisuals() :
m_hwndParent(NULL),
m_clrForeground(0x0000FF),
m_nPreset(0)
{
    lstrcpyn(m_szPluginText, _T("Microsoft Direct3D"), sizeof(m_szPluginText) / sizeof(m_szPluginText[0]));
    m_dwAdviseCookie = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::~CMediaPlayerVisuals
// Destructor

CMediaPlayerVisuals::~CMediaPlayerVisuals()
{
    Destroy();
}

/////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals:::FinalConstruct
// Called when an effect is first loaded. Use this function to do one-time
// intializations that could fail (i.e. creating offscreen buffers) instead
// of doing this in the constructor, which cannot return an error.

HRESULT CMediaPlayerVisuals::FinalConstruct()
{
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals:::FinalRelease
// Called when an effect is unloaded. Use this function to free any
// resources allocated in FinalConstruct.

void CMediaPlayerVisuals::FinalRelease()
{
    ReleaseCore();
}


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::Render
// Called when an effect should render itself to the screen.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::Render(TimedLevel *pLevels, HDC hdc, RECT *prc)
{
    HRESULT hr = S_OK;

    // If we just switched the skin mode, destroy the current visualization
    // so it can be recreated with the correct parent window
    if( !g_bSkinMode )
    {
        g_bSkinMode = TRUE;
        g_rpVisualizations[m_nPreset]->Destroy();
    }
  
    // Initialize if needed
    if( g_rpVisualizations[m_nPreset]->IsInitialized() == FALSE )
    {
        hr = g_rpVisualizations[m_nPreset]->CreateSkinMode();
        if( FAILED(hr) )
            return hr;
    }

    return g_rpVisualizations[m_nPreset]->Render( pLevels, hdc, prc );
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::MediaInfo
// Everytime new media is loaded, this method is called to pass the
// number of channels (mono/stereo), the sample rate of the media, and the
// title of the media
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::MediaInfo(LONG lChannelCount, LONG lSampleRate, BSTR bstrTitle )
{
    return g_rpVisualizations[m_nPreset]->MediaInfo( lChannelCount, lSampleRate, bstrTitle );
}


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::GetCapabilities
// Returns the capabilities of this effect. Flags that can be returned are:
//	EFFECT_CANGOFULLSCREEN		-- effect supports full-screen rendering
//	EFFECT_HASPROPERTYPAGE		-- effect supports a property page
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::GetCapabilities(DWORD * pdwCapabilities)
{
    if (NULL == pdwCapabilities)
    {
        return E_POINTER;
    }

    *pdwCapabilities = 0;

    for( int i=0; i < NUM_VISUALIZATIONS; i++ )
    {
        DWORD dwCaps = 0;
        g_rpVisualizations[i]->GetCapabilities( &dwCaps );

        *pdwCapabilities |= dwCaps;
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::DisplayPropertyPage
// Invoked when a host wants to display the property page for the effect
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::DisplayPropertyPage(HWND hwndOwner)
{
    HPROPSHEETPAGE rhPages[NUM_VISUALIZATIONS] = {0};
    PROPSHEETPAGE psp = {0};

    // For each visualization, attempt to get the property sheet page
    // header from the visualization and create the page. If successful,
    // add the page to the list passed to the sheet.
    int numPages = 0;
    for( int i=0; i < NUM_VISUALIZATIONS; i++ )
    {
        HRESULT hr = g_rpVisualizations[i]->GetPropSheetPage( &psp );
        if( FAILED(hr) )
            continue;

        rhPages[numPages] = CreatePropertySheetPage( &psp );
        if( rhPages[numPages] != NULL )
            numPages++;
    }
    
    // Create the property sheet header, which describes the property sheet
    // displayed when the user accesses the visualization properties.
    PROPSHEETHEADER psh = {0};
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_DEFAULT | PSH_PROPTITLE;
    psh.hwndParent = hwndOwner;
    psh.hInstance = g_hInstance;
    psh.pszCaption = m_szPluginText;
    psh.nPages = numPages;
    psh.phpage = rhPages; 

    // Display the property sheet
    PropertySheet( &psh );
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::GetTitle
// Invoked when a host wants to obtain the title of the effect
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::GetTitle(BSTR* bstrTitle)
{
    USES_CONVERSION;

    if (NULL == bstrTitle)
    {
        return E_POINTER;
    }

    TCHAR strTitle[256] = {0};
    if( 0 == LoadString( g_hInstance, IDS_EFFECTNAME, strTitle, 256 ) )
        return E_FAIL;

 
    WCHAR wstrTitle[256] = {0};
    DXUtil_ConvertGenericStringToWideCch( wstrTitle, strTitle, 256 );

    *bstrTitle = SysAllocString( wstrTitle );

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::GetPresetTitle
// Invoked when a host wants to obtain the title of the given preset
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::GetPresetTitle(LONG nPreset, BSTR *bstrPresetTitle)
{
    USES_CONVERSION;

    HRESULT hr = S_OK;
    
    if (NULL == bstrPresetTitle)
    {
        return E_POINTER;
    }

    if ((nPreset < 0) || (nPreset >= NUM_VISUALIZATIONS))
    {
        return E_INVALIDARG;
    }

    TCHAR strTitle[256] = {0};
    
    hr = g_rpVisualizations[nPreset]->GetTitle( strTitle, 256 );
    if( FAILED(hr) )
        return hr;

    WCHAR wstrTitle[256] = {0};
    DXUtil_ConvertGenericStringToWideCch( wstrTitle, strTitle, 256 );

    *bstrPresetTitle = SysAllocString( wstrTitle );

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::GetPresetCount
// Invoked when a host wants to obtain the number of supported presets
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::GetPresetCount(LONG *pnPresetCount)
{
    if (NULL == pnPresetCount)
    {
        return E_POINTER;
    }

    *pnPresetCount = NUM_VISUALIZATIONS;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::SetCurrentPreset
// Invoked when a host wants to change the index of the current preset
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::SetCurrentPreset(LONG nPreset)
{
    if ((nPreset < 0) || (nPreset >= NUM_VISUALIZATIONS))
    {
        return E_INVALIDARG;
    }

    // Current preset already selected
    if( nPreset == m_nPreset )
        return S_OK;

    // Destroy the previous preset
    g_rpVisualizations[ m_nPreset ]->Destroy();

    // Set the new preset
    m_nPreset = nPreset;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::GetCurrentPreset
// Invoked when a host wants to obtain the index of the current preset
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::GetCurrentPreset(LONG *pnPreset)
{
    if (NULL == pnPreset)
    {
        return E_POINTER;
    }

    *pnPreset = m_nPreset;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::SetCore
// Set WMP core interface
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::SetCore(IWMPCore * pCore)
{
    HRESULT hr = S_OK;

    // release any existing WMP core interfaces
    ReleaseCore();

    // If we get passed a NULL core, this  means
    // that the plugin is being shutdown.

    if (pCore == NULL)
    {
        return S_OK;
    }

    m_spCore = pCore;

    // connect up the event interface
    CComPtr<IConnectionPointContainer>  spConnectionContainer;

    hr = m_spCore->QueryInterface( &spConnectionContainer );

    if (SUCCEEDED(hr))
    {
        hr = spConnectionContainer->FindConnectionPoint( __uuidof(IWMPEvents), &m_spConnectionPoint );
    }

    if (SUCCEEDED(hr))
    {
        hr = m_spConnectionPoint->Advise( GetUnknown(), &m_dwAdviseCookie );

        if ((FAILED(hr)) || (0 == m_dwAdviseCookie))
        {
            m_spConnectionPoint = NULL;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::Create
// Invoked when the visualization should be initialized.
//
// If hwndParent != NULL, RenderWindowed() will be called and the visualization
// should draw into the window specified by hwndParent. This will be the
// behavior when the visualization is hosted in a window.
//
// If hwndParent == NULL, Render() will be called and the visualization
// should draw into the DC passed to Render(). This will be the behavior when
// the visualization is hosted windowless (like in a skin for example).
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::Create(HWND hwndParent)
{
    m_hwndParent = hwndParent;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::Destroy
// Invoked when the visualization should be released.
//
// Any resources allocated for rendering should be released.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::Destroy()
{
    m_hwndParent = NULL;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::NotifyNewMedia
// Invoked when a new media stream begins playing.
//
// The visualization can inspect this object for properties (like name or artist)
// that might be interesting for visualization.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::NotifyNewMedia(IWMPMedia *pMedia)
{
    return g_rpVisualizations[m_nPreset]->NotifyNewMedia( pMedia );
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::OnWindowMessage
// Window messages sent to the parent window.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::OnWindowMessage(UINT msg, WPARAM WParam, LPARAM LParam, LRESULT *plResultParam )
{
    return g_rpVisualizations[m_nPreset]->OnWindowMessage( msg, WParam, LParam, plResultParam );
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::RenderWindowed
// Called when an effect should render itself to the screen.
//
// The fRequiredRender flag specifies if an update is required, otherwise the
// update is optional. This allows visualizations that are fairly static (for example,
// album art visualizations) to only render when the parent window requires it,
// instead of n times a second for dynamic visualizations.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::RenderWindowed(TimedLevel *pLevels, BOOL fRequiredRender )
{
    HRESULT hr = S_OK;

    // If we just switched the full mode, destroy the current visualization
    // so it can be recreated with the correct parent window
    if( g_bSkinMode )
    {
        g_bSkinMode = FALSE;
        g_rpVisualizations[m_nPreset]->Destroy();
    }
    
        
    // Initialize if needed
    if( g_rpVisualizations[m_nPreset]->IsInitialized() == FALSE )
    {
        hr = g_rpVisualizations[m_nPreset]->CreateFullMode( m_hwndParent );
        if( FAILED(hr) )
            return hr;
    }

    return g_rpVisualizations[m_nPreset]->RenderWindowed( pLevels, fRequiredRender );
}

/////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::ReleaseCore
// Release WMP core interfaces
//////////////////////////////////////////////////////////////////////////////
void CMediaPlayerVisuals::ReleaseCore()
{
    if (m_spConnectionPoint)
    {
        if (0 != m_dwAdviseCookie)
        {
            m_spConnectionPoint->Unadvise(m_dwAdviseCookie);
            m_dwAdviseCookie = 0;
        }
        m_spConnectionPoint = NULL;
    }

    if (m_spCore)
    {
        m_spCore = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::get_foregroundColor
// Property get to retrieve the foregroundColor prop via the public interface.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::get_foregroundColor(BSTR *pVal)
{
	return ColorToWz( pVal, m_clrForeground);
}


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::put_foregroundColor
// Property put to set the foregroundColor prop via the public interface.
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMediaPlayerVisuals::put_foregroundColor(BSTR newVal)
{
	return WzToColor(newVal, &m_clrForeground);
}


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::WzToColor
// Helper function used to convert a string into a COLORREF.
//////////////////////////////////////////////////////////////////////////////
HRESULT CMediaPlayerVisuals::WzToColor(const WCHAR *pwszColor, COLORREF *pcrColor)
{ 
    if (NULL == pwszColor)
    {
        //NULL color string passed in
        return E_POINTER;
    }

    if (0 == lstrlenW(pwszColor))
    {
        //Empty color string passed in
        return E_INVALIDARG;
    }

    if (NULL == pcrColor)
    {
        //NULL output color DWORD passed in
        return E_POINTER;
    }
    
    if (lstrlenW(pwszColor) != 7)
    {
        //hex color string is not of the correct length
        return E_INVALIDARG;
    }

    DWORD dwRet = 0;
    for (int i = 1; i < 7; i++)
    {
        // shift dwRet by 4
        dwRet <<= 4;
        // and add in the value of this string

        if ((pwszColor[i] >= L'0') && (pwszColor[i] <= L'9'))
        {
            dwRet += pwszColor[i] - '0';
        }
        else if ((pwszColor[i] >= L'A') && (pwszColor[i] <= L'F'))
        {
            dwRet += 10 + (pwszColor[i] - L'A');
        }
        else if ((pwszColor[i] >= L'a') && (pwszColor[i] <= L'f'))
        {
            dwRet += 10 + (pwszColor[i] - L'a');
        }
        else
        {
           //Invalid hex digit in color string
            return E_INVALIDARG;
        }
    }

    *pcrColor = SwapBytes(dwRet);

    return S_OK;
} 


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::ColorToWz
// Helper function used to convert a COLORREF to a BSTR.
//////////////////////////////////////////////////////////////////////////////
HRESULT CMediaPlayerVisuals::ColorToWz( BSTR* pbstrColor, COLORREF crColor)
{
    _ASSERT( NULL != pbstrColor );
    _ASSERT( (crColor & 0x00FFFFFF) == crColor );

    *pbstrColor = NULL;

    WCHAR wsz[8];
    HRESULT hr  = S_OK;

    wsprintfW( wsz, L"#%06X", SwapBytes(crColor) );
    
    *pbstrColor = ::SysAllocString( wsz );

    if (!pbstrColor)
    {
        hr = E_FAIL;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals::SwapBytes
// Used to convert between a DWORD and COLORREF.  Simply swaps the lowest 
// and 3rd order bytes.
//////////////////////////////////////////////////////////////////////////////
inline DWORD CMediaPlayerVisuals::SwapBytes(DWORD dwRet)
{
    return ((dwRet & 0x0000FF00) | ((dwRet & 0x00FF0000) >> 16) | ((dwRet & 0x000000FF) << 16));
}

