// [!output PROJECT_NAME].h : main header file for the [!output PROJECT_NAME] application
//

#pragma once

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
[!if DPLAY]




//-----------------------------------------------------------------------------
// Player context locking defines
//-----------------------------------------------------------------------------
CRITICAL_SECTION g_csPlayerContext;
#define PLAYER_LOCK()                   EnterCriticalSection( &g_csPlayerContext ); 
#define PLAYER_ADDREF( pPlayerInfo )    if( pPlayerInfo ) pPlayerInfo->lRefCount++;
#define PLAYER_RELEASE( pPlayerInfo )   if( pPlayerInfo ) { pPlayerInfo->lRefCount--; if( pPlayerInfo->lRefCount <= 0 ) SAFE_DELETE( pPlayerInfo ); } pPlayerInfo = NULL;
#define PLAYER_UNLOCK()                 LeaveCriticalSection( &g_csPlayerContext );

CRITICAL_SECTION g_csWorldStateContext;
#define WORLD_LOCK()                   EnterCriticalSection( &g_csWorldStateContext ); 
#define WORLD_UNLOCK()                 LeaveCriticalSection( &g_csWorldStateContext );
[!endif]




//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
// TODO: change "DirectX AppWizard Apps" to your name or the company name
#define DXAPP_KEY        TEXT("Software\\DirectX AppWizard Apps\\[!output PROJECT_NAME]")

[!if ACTIONMAPPER || DPLAY]
// This GUID must be unique for every game, and the same for 
// every instance of this app.  // [!output GUIDMSG]
[!if DPLAY]
// The GUID allows DirectPlay to find other instances of the same game on
// the network.  
[!endif]
[!if ACTIONMAPPER]
// The GUID allows DirectInput to remember input settings
[!endif]
GUID g_guidApp = [!output GUIDSTRUCT];


[!endif]
[!if DPLAY]
// Associate a structure with every network player
struct APP_PLAYER_INFO
{
    // TODO: change as needed
    LONG  lRefCount;                        // Ref count so we can cleanup when all threads 
                                            // are done w/ this object
    DPNID dpnidPlayer;                      // DPNID of player
    TCHAR strPlayerName[MAX_PATH];          // Player name

[!if ACTIONMAPPER]
    FLOAT fAxisRotateUD;                    // State of axis for this player
    FLOAT fAxisRotateLR;                    // State of axis for this player
[!else]
    BOOL  bRotateUp;                       // State of up button or this player
    BOOL  bRotateDown;                     // State of down button or this player
    BOOL  bRotateLeft;                     // State of left button or this player
    BOOL  bRotateRight;                    // State of right button or this player
[!endif]
[!if DPLAYVOICE]

    BOOL  bHalfDuplex;                      // TRUE if player is in half-duplex mode
    BOOL  bTalking;                         // TRUE if player is talking
[!endif]

    APP_PLAYER_INFO* pNext;
    APP_PLAYER_INFO* pPrev;
};


[!endif]
[!if ACTIONMAPPER]
// DirectInput action mapper reports events only when buttons/axis change
// so we need to remember the present state of relevant axis/buttons for 
// each DirectInput device.  The CInputDeviceManager will store a 
// pointer for each device that points to this struct
struct InputDeviceState
{
    // TODO: change as needed
    FLOAT fAxisRotateLR;
    BOOL  bButtonRotateLeft;
    BOOL  bButtonRotateRight;

    FLOAT fAxisRotateUD;
    BOOL  bButtonRotateUp;
    BOOL  bButtonRotateDown;
[!if DMUSIC || DSOUND]

    BOOL  bButtonPlaySoundButtonDown;
[!endif]
};


[!endif]
// Struct to store the current input state
struct UserInput
{
[!if KEYBOARD]
    BYTE diks[256];   // DirectInput keyboard state buffer 

[!endif]
    // TODO: change as needed
[!if ACTIONMAPPER]
    FLOAT fAxisRotateUD;
    FLOAT fAxisRotateLR;
[!else]
    BOOL bRotateUp;
    BOOL bRotateDown;
    BOOL bRotateLeft;
    BOOL bRotateRight;
[!endif]
[!if DMUSIC || DSOUND]
    BOOL bPlaySoundButtonDown;
[!endif]
[!if ACTIONMAPPER]
    BOOL bDoConfigureInput;
[!endif]
[!if DPLAYVOICE]
    BOOL bDoConfigureVoice;
[!endif]
};


[!if ACTIONMAPPER]
// Input semantics used by this app
enum INPUT_SEMANTICS
{
    // Gameplay semantics
    // TODO: change as needed
    INPUT_ROTATE_AXIS_LR=1, INPUT_ROTATE_AXIS_UD,       
    INPUT_ROTATE_LEFT,      INPUT_ROTATE_RIGHT,    
    INPUT_ROTATE_UP,        INPUT_ROTATE_DOWN,
    INPUT_CONFIG_INPUT,     
[!if DPLAYVOICE]
    INPUT_CONFIG_VOICE,     
[!endif]
[!if DMUSIC || DSOUND]
    INPUT_PLAY_SOUND,       
[!endif]
};

// Actions used by this app
DIACTION g_rgGameAction[] =
{
    // TODO: change as needed.  Be sure to delete user map files 
    // (C:\Program Files\Common Files\DirectX\DirectInput\User Maps\*.ini)
    // after changing this, otherwise settings won't reset and will be read 
    // from the out of date ini files 

    // Device input (joystick, etc.) that is pre-defined by DInput, according
    // to genre type. The genre for this app is space simulators.
    { INPUT_ROTATE_AXIS_LR,  DIAXIS_3DCONTROL_LATERAL,      0, TEXT("Rotate left/right"), },
    { INPUT_ROTATE_AXIS_UD,  DIAXIS_3DCONTROL_MOVE,         0, TEXT("Rotate up/down"), },
[!if DMUSIC || DSOUND]
    { INPUT_PLAY_SOUND,      DIBUTTON_3DCONTROL_SPECIAL,    0, TEXT("Play sound"), },
[!endif]

    // Keyboard input mappings
    { INPUT_ROTATE_LEFT,     DIKEYBOARD_LEFT,               0, TEXT("Rotate left"), },
    { INPUT_ROTATE_RIGHT,    DIKEYBOARD_RIGHT,              0, TEXT("Rotate right"), },
    { INPUT_ROTATE_UP,       DIKEYBOARD_UP,                 0, TEXT("Rotate up"), },
    { INPUT_ROTATE_DOWN,     DIKEYBOARD_DOWN,               0, TEXT("Rotate down"), },
[!if DMUSIC || DSOUND]
    { INPUT_PLAY_SOUND,      DIKEYBOARD_F5,                 0, TEXT("Play sound"), },
[!endif]
    { INPUT_CONFIG_INPUT,    DIKEYBOARD_F3,                 DIA_APPFIXED, TEXT("Configure Input"), },    
[!if DPLAYVOICE]
    { INPUT_CONFIG_VOICE,    DIKEYBOARD_F4,                 DIA_APPFIXED, TEXT("Configure Voice"), },    
[!endif]
};

#define NUMBER_OF_GAMEACTIONS    (sizeof(g_rgGameAction)/sizeof(DIACTION))


[!endif]
[!if DPLAY]
//-----------------------------------------------------------------------------
// App specific DirectPlay messages and structures 
//-----------------------------------------------------------------------------

// TODO: change or add app specific DirectPlay messages and structures as needed
#define GAME_MSGID_WORLDSTATE    1
#define GAME_MSGID_INPUTSTATE    2
#define GAME_MSGID_HOSTPAUSE     3

// Change compiler pack alignment to be BYTE aligned, and pop the current value
#pragma pack( push, 1 )

struct GAMEMSG_GENERIC
{
    // One of GAME_MSGID_* IDs so the app knows which GAMEMSG_* struct
    // to cast the msg pointer into.
    WORD nType; 
};

struct GAMEMSG_WORLDSTATE : public GAMEMSG_GENERIC
{
    FLOAT fWorldRotX;
    FLOAT fWorldRotY;
};

struct GAMEMSG_INPUTSTATE : public GAMEMSG_GENERIC
{
[!if ACTIONMAPPER]
    FLOAT fAxisRotateUD;
    FLOAT fAxisRotateLR;
[!else]
    BOOL  bRotateUp;   
    BOOL  bRotateDown; 
    BOOL  bRotateLeft; 
    BOOL  bRotateRight;
[!endif]
};

struct GAMEMSG_HOSTPAUSE : public GAMEMSG_GENERIC
{
    BOOL bHostPause;
};

// Pop the old pack alignment
#pragma pack( pop )


[!endif]


//-----------------------------------------------------------------------------
// Name: class C[!output SAFE_PROJECT_NAME]Dlg
// Desc: Dialog class derived from CDialog
//-----------------------------------------------------------------------------
class C[!output SAFE_PROJECT_NAME]Dlg : public CDialog
{
public:
    C[!output SAFE_PROJECT_NAME]Dlg(CWnd* pParent = NULL);    // standard constructor
    virtual ~C[!output SAFE_PROJECT_NAME]Dlg();

    FLOAT                   m_fTime;                // Current time in seconds
    FLOAT                   m_fElapsedTime;         // Time elapsed since last frame

protected:
    BOOL                    m_bLoadingApp;          // TRUE, if the app is loading
    BOOL              	    m_bHasFocus;	    // TRUE, if the app has focus
    TCHAR*                  m_strWindowTitle;       // Title for the app's window
    DWORD                   m_dwCreationWidth;      // Width used to create window
    DWORD                   m_dwCreationHeight;     // Height used to create window

[!if KEYBOARD]
    LPDIRECTINPUT8          m_pDI;                  // DirectInput object
    LPDIRECTINPUTDEVICE8    m_pKeyboard;            // DirectInput keyboard device
[!endif]
[!if ACTIONMAPPER]
    CInputDeviceManager*    m_pInputDeviceManager;  // DirectInput device manager
    DIACTIONFORMAT          m_diafGame;             // Action format for game play
[!endif]
    UserInput               m_UserInput;            // Struct for storing user input 

[!if DMUSIC || DSOUND]
    FLOAT                   m_fSoundPlayRepeatCountdown; // Sound repeat timer
[!if DMUSIC]
    CMusicManager*          m_pMusicManager;        // DirectMusic manager class
    CMusicSegment*          m_pBounceSound;         // Bounce sound
[!else]
    CSoundManager*          m_pSoundManager;        // DirectSound manager class
    CSound*                 m_pBounceSound;         // Bounce sound
[!endif]

[!endif]
[!if DPLAY]
    IDirectPlay8Peer*       m_pDP;                  // DirectPlay peer object
    CNetConnectWizard*      m_pNetConnectWizard;    // Connection wizard
    IDirectPlay8LobbiedApplication* m_pLobbiedApp;  // DirectPlay lobbied app 
    BOOL                    m_bWasLobbyLaunched;    // TRUE if lobby launched
    DPNID                   m_dpnidLocalPlayer;     // DPNID of local player
    LONG                    m_lNumberOfActivePlayers;        // Number of players currently in game
    TCHAR                   m_strLocalPlayerName[MAX_PATH];  // Local player name
    TCHAR                   m_strSessionName[MAX_PATH];      // Session name
    TCHAR                   m_strPreferredProvider[MAX_PATH];// Provider string
    APP_PLAYER_INFO         m_PlayInfoList;         // List of players
    APP_PLAYER_INFO*        m_pLocalPlayerInfo;     // APP_PLAYER_INFO struct for local player
    HRESULT                 m_hrNet;                // HRESULT of DirectPlay events
    FLOAT                   m_fWorldSyncTimer;      // Timer for syncing world state between players
    BOOL                    m_bHostPausing;         // Has the host paused the app?
    UserInput               m_CombinedNetworkInput; // Combined input from all network players

[!endif]
[!if DPLAYVOICE]
    CNetVoice*              m_pNetVoice;            // DirectPlay voice helper class
    DVCLIENTCONFIG          m_dvClientConfig;       // Voice client config
    GUID                    m_guidDVSessionCT;      // GUID for chosen voice compression
    BOOL                    m_bNetworkPlayersTalking; // TRUE if any of the network players are talking
    BOOL                    m_bLocalPlayerTalking;  // TRUE if the local player is talking

[!endif]
    FLOAT                   m_fWorldRotX;           // World rotation state X-axis
    FLOAT                   m_fWorldRotY;           // World rotation state Y-axis

// Dialog Data
    //{{AFX_DATA(C[!output SAFE_PROJECT_NAME]Dlg)
    enum { IDD = IDD_[!output SAFE_PROJECT_NAME]_DIALOG };
        // NOTE: the ClassWizard will add data members here
    //}}AFX_DATA

public:
    virtual HRESULT OneTimeSceneInit();
    virtual HRESULT Render();
    virtual HRESULT FrameMove();
    virtual HRESULT FinalCleanup();
    virtual VOID    Pause( BOOL bPause );

protected:
    HRESULT RenderText();

[!if DINPUT]
    HRESULT InitInput( HWND hWnd );
[!endif]
    void    UpdateInput( UserInput* pUserInput );
[!if DPLAY]
    HRESULT CombineInputFromAllPlayers( UserInput* pCombinedUserInput );
[!endif]
[!if DINPUT]
    void    CleanupDirectInput();
[!endif]

[!if DMUSIC || DSOUND]
    HRESULT InitAudio( HWND hWnd );

[!endif]
[!if DPLAY]
    HRESULT InitDirectPlay();
    void    CleanupDirectPlay();
    HRESULT ConnectViaDirectPlay();
    HRESULT SendLocalInputIfChanged();
    HRESULT SendWorldStateToAll();
    HRESULT SendPauseMessageToAll( BOOL bPause );

[!endif]
[!if DPLAYVOICE]
    HRESULT InitDirectPlayVoice();
    VOID    UpdateTalkingVariables();
    HRESULT UserConfigVoice();

[!endif]
    VOID    ReadSettings();
    VOID    WriteSettings();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(C[!output SAFE_PROJECT_NAME]Dlg)
    public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    HICON m_hIcon;
    HACCEL m_hAccel;

    // Generated message map functions
    //{{AFX_MSG(C[!output SAFE_PROJECT_NAME]Dlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

public:
[!if ACTIONMAPPER]

    HRESULT InputAddDeviceCB( CInputDeviceManager::DeviceInfo* pDeviceInfo, const DIDEVICEINSTANCE* pdidi );
    static HRESULT CALLBACK StaticInputAddDeviceCB( CInputDeviceManager::DeviceInfo* pDeviceInfo, const DIDEVICEINSTANCE* pdidi, LPVOID pParam );   
[!endif]
[!if DPLAY]

    static HRESULT WINAPI StaticDirectPlayMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
    HRESULT DirectPlayMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
    static HRESULT WINAPI StaticDirectPlayLobbyMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
    HRESULT DirectPlayLobbyMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
[!endif]
[!if DPLAYVOICE]

    static HRESULT WINAPI StaticDirectPlayVoiceServerMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
    HRESULT DirectPlayVoiceServerMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
    static HRESULT WINAPI StaticDirectPlayVoiceClientMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
    HRESULT DirectPlayVoiceClientMessageHandler( PVOID pvUserContext, DWORD dwMessageId, PVOID pMsgBuffer );
[!endif]
};




//-----------------------------------------------------------------------------
// Name: class CApp
// Desc: Main MFC application class derived from CWinApp.
//-----------------------------------------------------------------------------
class C[!output SAFE_PROJECT_NAME]App : public CWinApp
{
public:
    C[!output SAFE_PROJECT_NAME]App();

    C[!output SAFE_PROJECT_NAME]Dlg* m_pDlg;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(C[!output SAFE_PROJECT_NAME]App)
    public:
    virtual BOOL InitInstance();
    virtual BOOL OnIdle(LONG lCount);
    virtual int ExitInstance();
    //}}AFX_VIRTUAL

// Implementation

    //{{AFX_MSG(C[!output SAFE_PROJECT_NAME]App)
        // NOTE - the ClassWizard will add and remove member functions here.
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

