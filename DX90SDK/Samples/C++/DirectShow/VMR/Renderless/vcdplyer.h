//------------------------------------------------------------------------------
// File: vcdplyer.h
//
// Desc: DirectShow sample code - header file for CMovie class
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <ddraw.h>
#define D3D_OVERLOADS
#include <d3d.h>

/* -------------------------------------------------------------------------
** CMovie - a DirectShow movie playback class.
** -------------------------------------------------------------------------
*/
enum EMovieMode { MOVIE_NOTOPENED = 0x00,
                  MOVIE_OPENED    = 0x01,
                  MOVIE_PLAYING   = 0x02,
                  MOVIE_STOPPED   = 0x03,
                  MOVIE_PAUSED    = 0x04 };



class CMovie :
    public CUnknown,
    public IVMRSurfaceAllocator,
    public IVMRImagePresenter,
    public IVMRSurfaceAllocatorNotify
{

private:
    // Our state variable - records whether we are opened, playing etc.
    EMovieMode      m_Mode;
    HANDLE          m_MediaEvent;
    HWND            m_hwndApp;
    BOOL            m_bFullScreen;
    BOOL            m_bFullScreenPoss;
    int             m_iDuration;
    GUID            m_TimeFormat;

    LPDIRECTDRAWSURFACE7    m_pddsPrimary;
    LPDIRECTDRAWSURFACE7    m_pddsPriText;
    LPDIRECTDRAWSURFACE7    m_pddsRenderT;

    IFilterGraph*               m_Fg;
    IGraphBuilder*              m_Gb;
    IMediaControl*              m_Mc;
    IMediaSeeking*              m_Ms;
    IMediaEvent*                m_Me;

    IVMRSurfaceAllocator*       m_lpDefSA;
    IVMRImagePresenter*         m_lpDefIP;
    IVMRWindowlessControl*      m_lpDefWC;
    IVMRSurfaceAllocatorNotify* m_lpDefSAN;

    HRESULT CreateDefaultAllocatorPresenter();
    HRESULT AddVideoMixingRendererToFG();
    HRESULT OnSetDDrawDevice(LPDIRECTDRAW7 pDD, HMONITOR hMon);


public:
     CMovie(HWND hwndApplication);
    ~CMovie();

    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID, void**);

    // IVMRSurfaceAllocator
    STDMETHODIMP AllocateSurface(DWORD_PTR dwUserID,
                                VMRALLOCATIONINFO* lpAllocInfo,
                                 DWORD* lpdwActualBackBuffers,
                                 LPDIRECTDRAWSURFACE7* lplpSurface);
    STDMETHODIMP FreeSurface(DWORD_PTR dwUserID);
    STDMETHODIMP PrepareSurface(DWORD_PTR dwUserID,
                                LPDIRECTDRAWSURFACE7 lplpSurface,
                                DWORD dwSurfaceFlags);
    STDMETHODIMP AdviseNotify(IVMRSurfaceAllocatorNotify* lpIVMRSurfAllocNotify);

    // IVMRSurfaceAllocatorNotify
    STDMETHODIMP AdviseSurfaceAllocator(DWORD_PTR dwUserID,
                                        IVMRSurfaceAllocator* lpIVRMSurfaceAllocator);
    STDMETHODIMP SetDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor);
    STDMETHODIMP ChangeDDrawDevice(LPDIRECTDRAW7 lpDDrawDevice,HMONITOR hMonitor);
    STDMETHODIMP RestoreDDrawSurfaces();
    STDMETHODIMP NotifyEvent(LONG EventCode, LONG_PTR lp1, LONG_PTR lp2);
    STDMETHODIMP SetBorderColor(COLORREF clr);

    // IVMRImagePresenter
    STDMETHODIMP StartPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP StopPresenting(DWORD_PTR dwUserID);
    STDMETHODIMP PresentImage(DWORD_PTR dwUserID,
                              VMRPRESENTATIONINFO* lpPresInfo);

    HRESULT         OpenMovie(TCHAR *lpFileName);
    DWORD           CloseMovie();

    BOOL            PlayMovie();
    BOOL            PauseMovie();
    BOOL            StopMovie();

    OAFilterState   GetStateMovie();

    HANDLE          GetMovieEventHandle();
    long            GetMovieEventCode();

    BOOL            PutMoviePosition(LONG x, LONG y, LONG cx, LONG cy);

    REFTIME         GetDuration();
    REFTIME         GetCurrentPosition();
    BOOL            SeekToPosition(REFTIME rt,BOOL bFlushData);

    BOOL            RepaintVideo(HWND hwnd, HDC hdc);

    void            SetFullScreenMode(BOOL bMode);
    BOOL            IsFullScreenMode();

    void            DisplayModeChanged() {
        if (m_lpDefWC) {
            m_lpDefWC->DisplayModeChanged();
        }
    }

};
