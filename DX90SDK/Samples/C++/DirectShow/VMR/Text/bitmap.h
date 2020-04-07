//------------------------------------------------------------------------------
// File: Bitmap.h
//
// Desc: DirectShow sample code - header file for VMR bitmap manipulation
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

//
// Constants
//
#define TRANSPARENCY_VALUE   (0.5f)

#define PURE_WHITE          RGB(255,255,255)
#define ALMOST_WHITE        RGB(250,250,250)

#define BLEND_TEXT          TEXT("This is a demonstration of alpha-blended dynamic text.\0")
#define DYNAMIC_TEXT_SIZE   255

#define DEFAULT_FONT_NAME   TEXT("Impact\0")
#define DEFAULT_FONT_STYLE  TEXT("Regular\0")
#define DEFAULT_FONT_SIZE   12
#define DEFAULT_FONT_COLOR  RGB(255,0,0)
#define MAX_FONT_SIZE		25

#define STR_VMR_DISPLAY_WARNING  \
    TEXT("The VMR requires Direct3D in order to perform alpha blending.  ") \
    TEXT("Therefore, this sample requires that your display be set to a mode ") \
    TEXT("which is compatible with your computer's video card.  ") \
    TEXT("Most video cards support Direct3D in 16-bit and 32-bit RGB modes, ") \
    TEXT("and some newer cards support 16, 24 and 32-bit display modes.\r\n\r\n") \
    TEXT("To correct this problem, try changing your display to use ") \
    TEXT("16-bit or 32-bit color depth in the Display Control Panel applet.\0")

//
// Function prototypes
//
HRESULT BlendText(HWND hwndApp, TCHAR *szNewText);

HFONT UserSelectFont(void);
HFONT SetTextFont(BOOL bShowDialog);

void SetColorRef(VMRALPHABITMAP& bmpInfo);
void UpdateText(void);
void StartTimer(void);
void StopTimer(void);

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

//
// Global data
//
extern IVMRMixerBitmap *pBMP;
extern HFONT g_hFont;
extern TCHAR g_szAppText[DYNAMIC_TEXT_SIZE];
