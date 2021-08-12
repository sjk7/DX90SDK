//------------------------------------------------------------------------------
// File: TransNull555.h
//
// Desc: DirectShow sample code - header for RGBFilters component
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

extern const AMOVIESETUP_FILTER sudTransNull555;

class CTransNull555 : public CTransInPlaceFilter
{
public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    HRESULT Transform(IMediaSample *pSample);
    HRESULT CheckInputType(const CMediaType *mtIn);

private:

    // Constructor
    CTransNull555( LPUNKNOWN punk, HRESULT *phr );
};

