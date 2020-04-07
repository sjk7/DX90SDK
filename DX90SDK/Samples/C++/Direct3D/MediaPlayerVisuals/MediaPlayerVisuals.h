/////////////////////////////////////////////////////////////////////////////
//
// MediaPlayerVisuals.h : Declaration of the CMediaPlayerVisuals
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __MediaPlayerVisuals_H_
#define __MediaPlayerVisuals_H_

#include "resource.h"
#include "effects.h"
#include "iMediaPlayerVisuals.h"


class CD3DVisualization;
CD3DVisualization* g_rpVisualizations[];
#define NUM_VISUALIZATIONS ( sizeof(g_rpVisualizations) / sizeof(g_rpVisualizations[0]) )

/////////////////////////////////////////////////////////////////////////////
// CMediaPlayerVisuals
class ATL_NO_VTABLE CMediaPlayerVisuals : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CMediaPlayerVisuals, &CLSID_MediaPlayerVisuals>,
    public IDispatchImpl<IMediaPlayerVisuals, &IID_IMediaPlayerVisuals, &LIBID_MediaPlayerVisualsLib>,
    public IWMPEffects2
{
private:
    COLORREF    m_clrForeground;    // foreground color
    LONG        m_nPreset;          // current preset

    HRESULT WzToColor(const WCHAR *pwszColor, COLORREF *pcrColor);
    HRESULT ColorToWz( BSTR* pbstrColor, COLORREF crColor);
    DWORD SwapBytes(DWORD dwRet);

public:
    CMediaPlayerVisuals();
    ~CMediaPlayerVisuals();

DECLARE_REGISTRY_RESOURCEID(IDR_MEDIAPLAYERVISUALS)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMediaPlayerVisuals)
    COM_INTERFACE_ENTRY(IMediaPlayerVisuals)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IWMPEffects)
    COM_INTERFACE_ENTRY(IWMPEffects2)
END_COM_MAP()

public:

    // CComCoClass Overrides
    HRESULT FinalConstruct();
    void FinalRelease();

    // IMediaPlayerVisuals
    STDMETHOD(get_foregroundColor)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_foregroundColor)(/*[in]*/ BSTR newVal);

    // IWMPEffects
    STDMETHOD(Render)(TimedLevel *pLevels, HDC hdc, RECT *rc);
    STDMETHOD(MediaInfo)(LONG lChannelCount, LONG lSampleRate, BSTR bstrTitle);
    STDMETHOD(GetCapabilities)(DWORD * pdwCapabilities);
    STDMETHOD(GoFullscreen)(BOOL fFullScreen) { return E_NOTIMPL; };
    STDMETHOD(RenderFullScreen)(TimedLevel *pLevels) { return E_NOTIMPL; };
    STDMETHOD(DisplayPropertyPage)(HWND hwndOwner);
    STDMETHOD(GetTitle)(BSTR *bstrTitle);
    STDMETHOD(GetPresetTitle)(LONG nPreset, BSTR *bstrPresetTitle);
    STDMETHOD(GetPresetCount)(LONG *pnPresetCount);
    STDMETHOD(SetCurrentPreset)(LONG nPreset);
    STDMETHOD(GetCurrentPreset)(LONG *pnPreset);

    // IWMPEffects2
    STDMETHOD(SetCore)(IWMPCore * pCore);
    STDMETHOD(Create)(HWND hwndParent);
    STDMETHOD(Destroy)();
    STDMETHOD(NotifyNewMedia)(IWMPMedia *pMedia);
    STDMETHOD(OnWindowMessage)(UINT msg, WPARAM WParam, LPARAM LParam, LRESULT *plResultParam );
    STDMETHOD(RenderWindowed)(TimedLevel *pLevels, BOOL fRequiredRender );

    
    TCHAR        m_szPluginText[MAX_PATH];

private:
    void         ReleaseCore();

    HWND                        m_hwndParent;
    CComPtr<IWMPCore>           m_spCore;
    CComPtr<IConnectionPoint>   m_spConnectionPoint;
    DWORD                       m_dwAdviseCookie;
};

#endif //__MediaPlayerVisuals_H_
