//-----------------------------------------------------------------------------
// File: D3DVisualization.h
//
// Desc: Modified application class for the Direct3D samples framework library. 
//       This class is extended for use as a base class for Windows Media 
//       Player visualizations.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef D3DAPP_H
#define D3DAPP_H

#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include "prsht.h"
#include "effects.h"
#include "DXUtil.h"
#include "D3DUtil.h"
#include "D3DEnumeration.h"
#include "D3DSettings.h"
#include "D3DRes.h"

extern HINSTANCE g_hInstance;

//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------
enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHEDTOREF };

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOWINDOW            0x82000002
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_MEDIANOTFOUND       0x8200000b
#define D3DAPPERR_RESETFAILED         0x8200000c
#define D3DAPPERR_NULLREFDEVICE       0x8200000d




//-----------------------------------------------------------------------------
// Name: class CD3DVisualization
// Desc: A base class for creating sample D3D9 applications. To create a simple
//       Direct3D application, simply derive this class into a class (such as
//       class CMyD3DApplication) and override the following functions, as 
//       needed:
//          OneTimeSceneInit()    - To initialize app data (alloc mem, etc.)
//          InitDeviceObjects()   - To initialize the 3D scene objects
//          FrameMove()           - To animate the scene
//          Render()              - To render the scene
//          DeleteDeviceObjects() - To cleanup the 3D scene objects
//          FinalCleanup()        - To cleanup app data (for exitting the app)
//          MsgProc()             - To handle Windows messages
//-----------------------------------------------------------------------------
class CD3DVisualization
{
protected:
    CD3DEnumeration   m_d3dEnumeration;
    CD3DSettings      m_d3dSettings;

    // Internal variables for the state of the app
    bool              m_bWindowed;
    bool              m_bDeviceLost;
    bool              m_bMinimized;
    bool              m_bMaximized;
    bool              m_bIgnoreSizeChange;
    bool              m_bDeviceObjectsInited;
    bool              m_bDeviceObjectsRestored;

    // Internal variables used for timing
    bool              m_bFrameMoving;
    
    // Internal error handling function
    HRESULT DisplayErrorMsg( HRESULT hr, DWORD dwType );

    // Internal functions to manage and render the 3D scene
    static bool ConfirmDeviceHelper( D3DCAPS9* pCaps, VertexProcessingType vertexProcessingType, 
		D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat );
    bool    FindBestWindowedMode( bool bRequireHAL, bool bRequireREF );
    bool    FindBestFullscreenMode( bool bRequireHAL, bool bRequireREF );
    HRESULT ChooseInitialD3DSettings();
    virtual HRESULT Initialize3DEnvironment();
    HRESULT HandlePossibleSizeChange();
    HRESULT Reset3DEnvironment();
    HRESULT ToggleFullscreen();
    void    Cleanup3DEnvironment();
    HRESULT Render3DEnvironment();
    virtual void BuildPresentParamsFromSettings();
    virtual void UpdateStats();

    HRESULT CreateRenderTarget();
    HRESULT ReleaseRenderTarget();

protected:
    // Members specific to the visualizations
    TimedLevel *m_pAudioLevels;
    BOOL m_bInitialized;
    BOOL m_bSkinMode;
    BOOL m_bErrorInitializing;
    TCHAR m_strErrorMessage[MAX_PATH];

    RECT m_RectTarget;
    
    FLOAT m_fTimeLastResizeCheck;

    PDIRECT3DTEXTURE9 m_pTexRenderTarget;
    PDIRECT3DTEXTURE9 m_pTexScratch;

    // Main objects used for creating and rendering the 3D scene
    D3DPRESENT_PARAMETERS m_d3dpp;         // Parameters for CreateDevice/Reset
    HWND              m_hWnd;              // The main app window
    HWND              m_hWndFocus;         // The D3D focus window (usually same as m_hWnd)
    HMENU             m_hMenu;             // App menu bar (stored here when fullscreen)
    HMONITOR          m_hMonitor;          // Current render monitor on a multimon system
    LPDIRECT3D9       m_pD3D;              // The main D3D object
    LPDIRECT3DDEVICE9 m_pd3dDevice;        // The D3D rendering device
    D3DCAPS9          m_d3dCaps;           // Caps for the device
    D3DSURFACE_DESC   m_d3dsdBackBuffer;   // Surface desc of the backbuffer
    DWORD             m_dwCreateFlags;     // Indicate sw or hw vertex processing
    DWORD             m_dwWindowStyle;     // Saved window style for mode switches
    RECT              m_rcWindowBounds;    // Saved window bounds for mode switches
    RECT              m_rcWindowClient;    // Saved client area size for mode switches
    
    // Variables for timing
    FLOAT             m_fTime;             // Current time in seconds
    FLOAT             m_fElapsedTime;      // Time elapsed since last frame
    FLOAT             m_fFPS;              // Instanteous frame rate
    TCHAR             m_strDeviceStats[90];// String to hold D3D device stats
    TCHAR             m_strFrameStats[90]; // String to hold frame stats

    // Overridable variables for the app
    TCHAR*            m_strWindowTitle;    // Title for the app's window
    DWORD             m_dwCreationWidth;   // Width used to create window
    DWORD             m_dwCreationHeight;  // Height used to create window
    bool              m_bShowCursorWhenFullscreen; // Whether to show cursor when fullscreen
    bool              m_bClipCursorWhenFullscreen; // Whether to limit cursor pos when fullscreen
    bool              m_bStartFullscreen;  // Whether to start up the app in fullscreen mode
    bool              m_bCreateMultithreadDevice; // Whether to create a multithreaded device
    bool              m_bAllowDialogBoxMode; // If enabled the framework will try to enable GDI dialogs while in fullscreen

    // Overridable functions for the 3D scene created by the app
    virtual HRESULT ConfirmDevice(D3DCAPS9*,DWORD,D3DFORMAT,D3DFORMAT) { return S_OK; }
    virtual HRESULT OneTimeSceneInit()                         { return S_OK; }
    virtual HRESULT InitDeviceObjects()                        { return S_OK; }
    virtual HRESULT RestoreDeviceObjects()                     { return S_OK; }
    virtual HRESULT FrameMove()                                { return S_OK; }
    virtual HRESULT Render()                                   { return S_OK; }
    virtual HRESULT InvalidateDeviceObjects()                  { return S_OK; }
    virtual HRESULT DeleteDeviceObjects()                      { return S_OK; }
    virtual HRESULT FinalCleanup()                             { return S_OK; }

public:
    // Functions to create, run, pause, and clean up the application
    virtual HRESULT CreateFullMode( HWND hWnd );
    virtual HRESULT CreateSkinMode();
    virtual VOID    Destroy();
    virtual HRESULT GetPropSheetPage( PROPSHEETPAGE* pPage ) { return E_NOTIMPL; }
    virtual HRESULT Create();
    
    BOOL IsInitialized() { return m_bInitialized; }

    virtual         ~CD3DVisualization()                         { }

    // IWMPEffects
    virtual HRESULT Render(TimedLevel *pLevels, HDC hdc, RECT *rc);
    virtual HRESULT MediaInfo(LONG lChannelCount, LONG lSampleRate, BSTR bstrTitle) { return S_OK; }
    virtual HRESULT GetCapabilities(DWORD * pdwCapabilities) { *pdwCapabilities = 0; return S_OK; }
    virtual HRESULT GoFullscreen(BOOL fFullScreen);
    virtual HRESULT RenderFullScreen(TimedLevel *pLevels);
    virtual HRESULT GetTitle(TCHAR* strTitle, DWORD dwSize) { return E_NOTIMPL; }

    // IWMPEffects2
    virtual HRESULT RenderWindowed(TimedLevel *pLevels, BOOL fRequiredRender );
    virtual HRESULT NotifyNewMedia(IWMPMedia *pMedia) { return S_OK; }
    virtual HRESULT OnWindowMessage(UINT msg, WPARAM WParam, LPARAM LParam, LRESULT *plResultParam ) { return S_OK; }

    // Internal constructor/destructor
    CD3DVisualization();
};




#endif



