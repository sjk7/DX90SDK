//------------------------------------------------------------------------------
// File: project.h
//
// Desc: DirectShow sample code - main header file for Cube sample
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <windows.h>   
#include <commdlg.h>

#include <streams.h>
#include <mmreg.h>

#include "app.h"
#include "vcdplyer.h"
#include "resource.h"

#ifndef __RELEASE_DEFINED
#define __RELEASE_DEFINED
template<typename T>
__inline void RELEASE( T* &p )
{
    if( p ) {
        p->Release();
        p = NULL;
    }
}
#endif

#ifndef CHECK_HR
    #define CHECK_HR(expr) { if (FAILED(expr)) __leave; };
#endif

