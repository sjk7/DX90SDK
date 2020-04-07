//------------------------------------------------------------------------------
// File: AlphaStream.cpp
//
// Desc: DirectShow sample code - implementation of RGBFilters sample filters
//
// Copyright (c)  Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "..\iRGBFilters.h"
#include "AlphaSource.h"

// Constants
const long gScale = 1;
const long gRate = 15;
const long DEFAULT_WIDTH = 320;
const long DEFAULT_HEIGHT = 240;
const REFERENCE_TIME DEFAULT_DURATION = 768 * UNITS;

#define DBGLVL 3


LONGLONG inline Time2Frame( REFERENCE_TIME rt, long Scale, long Rate )
{
    return llMulDiv( rt, Rate, Scale * UNITS, 0 );
}

REFERENCE_TIME inline Frame2Time( LONGLONG Frame, long Scale, long Rate )
{
    return llMulDiv( Frame, Scale * UNITS, Rate, Rate - 1 );
}

//**************************************************************************
// 
//**************************************************************************
//
CAlphaSourceStream::CAlphaSourceStream(HRESULT *phr, CAlphaSource *pParent, LPCWSTR pPinName)
     : CSourceStream(NAME("SourceARGB"),phr, pParent, pPinName)
     , CSourceSeeking( NAME("SourceARGB"), (IPin*) this, phr, &m_SeekLock )
     , m_iDelivered( 0 )
     , m_nX( 0 )
     , m_nY( 0 )
     , m_nVx( 6 )
     , m_nVy( 8 )
     , m_nSize( 50 )

{
    // m_rtDuration is defined as the length of the source clip.
    // we default to the maximum amount of time.
    //
    m_rtDuration = DEFAULT_DURATION;
    m_rtStop = m_rtDuration;

    // no seeking to absolute positions and no seeking backwards!
    //
    m_dwSeekingCaps = 
        AM_SEEKING_CanSeekForwards |
        AM_SEEKING_CanGetStopPos   |
        AM_SEEKING_CanGetDuration  |
        AM_SEEKING_CanSeekAbsolute;

}

//**************************************************************************
// 
//**************************************************************************
//
CAlphaSourceStream::~CAlphaSourceStream()
{
}

//**************************************************************************
// 
//**************************************************************************
//
STDMETHODIMP CAlphaSourceStream::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
    if( riid == IID_IMediaSeeking ) 
    {
        return CSourceSeeking::NonDelegatingQueryInterface( riid, ppv );
    }
    return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

//**************************************************************************
// 
//**************************************************************************
//
HRESULT CAlphaSourceStream::ChangeRate( )
{
    return NOERROR;
}

//**************************************************************************
// GetMediaType. Return ARGB32, the only output format this one supports.
//**************************************************************************
//
HRESULT CAlphaSourceStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    CheckPointer(pmt,E_POINTER);

    if (iPosition < 0) 
    {
        return E_INVALIDARG;
    }

    // Have we run off the end of types

    if( iPosition > 0 ) 
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    VIDEOINFOHEADER vih;
    memset( &vih, 0, sizeof( vih ) );
    vih.bmiHeader.biCompression = BI_RGB;
    vih.bmiHeader.biBitCount    = 32;
    vih.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    vih.bmiHeader.biWidth        = DEFAULT_WIDTH;
    vih.bmiHeader.biHeight       = DEFAULT_HEIGHT;
    vih.bmiHeader.biPlanes       = 1;
    vih.bmiHeader.biSizeImage    = GetBitmapSize(&vih.bmiHeader);
    vih.bmiHeader.biClrImportant = 0;
    vih.AvgTimePerFrame          = UNITS * gScale / gRate;

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetFormat( (BYTE*) &vih, sizeof( vih ) );
    pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
    pmt->SetSampleSize(vih.bmiHeader.biSizeImage);

    return NOERROR;
}

//**************************************************************************
// CheckMediaType
//**************************************************************************
//
HRESULT CAlphaSourceStream::CheckMediaType(const CMediaType *pMediaType)
{
    CheckPointer(pMediaType,E_POINTER);

    // we only want fixed size video
    //
    if( *(pMediaType->Type()) != MEDIATYPE_Video )
    {
        return E_INVALIDARG;
    }
    if( !pMediaType->IsFixedSize( ) ) 
    {
        return E_INVALIDARG;
    }
    if( *pMediaType->Subtype( ) != MEDIASUBTYPE_ARGB32 )
    {
        return E_INVALIDARG;
    }
    if( *pMediaType->FormatType( ) != FORMAT_VideoInfo )
    {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    //
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();

    if (pvi == NULL)
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

//**************************************************************************
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. We examine m_mt to see what size image we agreed on.
// Then we can ask for buffers of the correct size to contain them.
//**************************************************************************
//
HRESULT CAlphaSourceStream::DecideBufferSize(IMemAllocator *pAlloc,
                                             ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    ASSERT(pvi);
    
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) 
    {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) 
    {
        return E_FAIL;
    }

    ASSERT( Actual.cBuffers == 1 );

    return NOERROR;
}

//**************************************************************************
//
//**************************************************************************
//
HRESULT CAlphaSourceStream::OnThreadStartPlay( )
{
    DeliverNewSegment( m_rtStart, m_rtStop, 1.0 );
    return CSourceStream::OnThreadStartPlay( );
}

//**************************************************************************
// FillBuffer. This routine fills up the given IMediaSample
//**************************************************************************
//
HRESULT CAlphaSourceStream::FillBuffer(IMediaSample *pms)
{
    CheckPointer(pms,E_POINTER);

    HRESULT hr = 0;

    // you NEED to have a lock on the critsec you sent to CSourceSeeking, so
    // it doesn't fill while you're changing positions, because the timestamps
    // won't be right. m_iDelivered will be off.
    //
    CAutoLock Lock( &m_SeekLock );

    REFERENCE_TIME NextSampleStart = Frame2Time( m_iDelivered, gScale, gRate );
    REFERENCE_TIME NextSampleStop =  Frame2Time( m_iDelivered + 1, gScale, gRate );

    // calculate the second being delivered
    //
    double dSecondStart = double( NextSampleStart ) / UNITS;
    long SecondStart    = (long) dSecondStart;
    dSecondStart -= double( SecondStart );
    dSecondStart *= gRate;

    long Frame = long( dSecondStart );
    long ActualFrame = Frame;
    if( SecondStart % 2 == 0 )
    {
        Frame = gRate - 1 - Frame;
    }

    // return S_FALSE if we've hit EOS. Parent class will send EOS for us
    //
    if( NextSampleStart > m_rtStop )
    {
        return S_FALSE;
    }

    // get the buffer and the bits
    //
    DWORD * pData = NULL;
    hr = pms->GetPointer( (BYTE**) &pData );
    if( FAILED( hr ) )
    {
        return hr;
    }

    long lDataLen = pms->GetSize( );
    if( lDataLen == 0 )
    {
        return NOERROR;
    }

    long alpha = ( m_nX + m_nSize ) * 255 / DEFAULT_WIDTH;
    memset( pData, 0, DEFAULT_HEIGHT * DEFAULT_WIDTH * 4 );

    for( int x = m_nX ; x < m_nX + m_nSize ; x++ )
    {
        for( int y = m_nY ; y < m_nY + m_nSize ; y++ )
        {
            *( pData + y * DEFAULT_WIDTH + x ) = ( alpha << 24 ) | 
                ( 0xffffff & ( ( x * y * m_iDelivered ) & 0xFFFFFF ) );
        }
    }

    m_nX += m_nVx;
    m_nY += m_nVy;

    while( m_nX < 0 )
    {
        m_nVx = 3 * ( ( rand( ) % 3 ) + 1 );
        m_nX += m_nVx;
    }
    while( m_nX + m_nSize > DEFAULT_WIDTH )
    {
        m_nVx = - 3 * ( ( rand( ) % 3 ) + 1 );
        m_nX += m_nVx;
    }
    while( m_nY < 0 )
    {
        m_nVy = 3 * ( ( rand( ) % 3 ) + 1 );
        m_nY += m_nVy;
    }
    while( m_nY + m_nSize > DEFAULT_HEIGHT )
    {
        m_nVy = -3 * ( ( rand( ) % 3 ) + 1 );
        m_nY += m_nVy;
    }

    m_iDelivered++;

    // set the timestamp
    //
    pms->SetTime( &NextSampleStart, &NextSampleStop );

    // set the sync point
    //
    pms->SetSyncPoint(true);

    return NOERROR;
}

//**************************************************************************
// 
//**************************************************************************
//
HRESULT CAlphaSourceStream::ChangeStart( )
{
    m_iDelivered = 0;

    if (ThreadExists()) 
    {
        // next time round the loop the worker thread will
        // pick up the position change.
        // We need to flush all the existing data - we must do that here
        // as our thread will probably be blocked in GetBuffer otherwise
        DeliverBeginFlush();

        // make sure we have stopped pushing
        Stop();

        // complete the flush
        DeliverEndFlush();

        // restart
        Run();
    }

    return NOERROR;
}

//**************************************************************************
// 
//**************************************************************************
//
HRESULT CAlphaSourceStream::ChangeStop( )
{
    return NOERROR;
}

