//-----------------------------------------------------------------------------
// File: SpectrumWheel.cpp
//
// Desc: An example of how to create a Windows Media Player visualization using
//       Direct3D graphics
//
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "SpectrumWheel.h"




//-----------------------------------------------------------------------------
// Name: CSpectrumWheel()
// Desc: Application constructor. Sets attributes for the app.
//-----------------------------------------------------------------------------
CSpectrumWheel::CSpectrumWheel()
{
    ZeroMemory( &m_vertices, sizeof(m_vertices) );
    ZeroMemory( &m_velocities, sizeof(m_velocities) );
    
    m_pVB = NULL;
    m_pIB = NULL;

    m_pCubeTex = NULL;

    m_vCameraPos = D3DXVECTOR3( 0, 0, 0 );

    m_colorInner = D3DXCOLOR(1.0f, 0.0f, 0.0f, 0.8f);
    m_colorOuter = D3DXCOLOR(1.0f, 1.0f, 1.0f, 0.8f);
    
    CalculateColors();
}





//-----------------------------------------------------------------------------
// Name: GetTitle()
// Desc: Retrieves the display name for this visualization preset
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::GetTitle(TCHAR* strTitle, DWORD dwSize)
{
    _tcsncpy( strTitle, TEXT("Spectrum Wheel"), dwSize );
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
HRESULT CSpectrumWheel::GetCapabilities(DWORD * pdwCapabilities)
{
    *pdwCapabilities = EFFECT_CANGOFULLSCREEN;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Called during initial app startup, this function performs all the
//       permanent initialization.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::OneTimeSceneInit()
{
    int p, s, t, i; // loop variables
    
    // For each pass
    for( p=0; p < RENDER_PASSES; p++ )
    {
        // For each track
        for( t=0; t < NUM_TRACKS; t++ )
        {
            double r1 = INNER_RADIUS + (t * DISTANCE_PER_TRACK) + SPACING_PER_TRACK;
            double r2 = INNER_RADIUS + ((t+1) * DISTANCE_PER_TRACK) - SPACING_PER_TRACK;

            // For each sector
            for( s=0; s < NUM_SECTORS; s++ )
            {
                double theta1 = (s * RADIANS_PER_SECTOR) + SPACING_PER_SECTOR;
                double theta2 = ((s+1) * RADIANS_PER_SECTOR) - SPACING_PER_SECTOR;    

                D3DXVECTOR3 FRONT_RIGHT    = D3DXVECTOR3( (FLOAT) (r1 * cos( theta1 )), 
                                                          0.0f, 
                                                          (FLOAT) (r1 * sin( theta1 )) );

                D3DXVECTOR3 FRONT_LEFT     = D3DXVECTOR3( (FLOAT) (r1 * cos( theta2 )), 
                                                          0.0f, 
                                                          (FLOAT) (r1 * sin( theta2 )) );

                D3DXVECTOR3 BACK_LEFT      = D3DXVECTOR3( (FLOAT) (r2 * cos( theta2 )), 
                                                          0.0f, 
                                                          (FLOAT) (r2 * sin( theta2 )) );

                D3DXVECTOR3 BACK_RIGHT     = D3DXVECTOR3( (FLOAT) (r2 * cos( theta1 )), 
                                                          0.0f, 
                                                          (FLOAT) (r2 * sin( theta1 )) );


                CopyMemory( &m_vertices[p][t][s][0],   &FRONT_RIGHT,    sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][1],   &FRONT_LEFT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][2],   &BACK_LEFT,      sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][3],   &BACK_RIGHT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][4],   &FRONT_RIGHT,    sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][5],   &FRONT_LEFT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][6],   &FRONT_LEFT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][7],   &FRONT_RIGHT,    sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][8],   &FRONT_LEFT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][9],   &BACK_LEFT,      sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][10],  &BACK_LEFT,      sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][11],  &FRONT_LEFT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][12],  &BACK_LEFT,      sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][13],  &BACK_RIGHT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][14],  &BACK_RIGHT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][15],  &BACK_LEFT,      sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][16],  &BACK_RIGHT,     sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][17],  &FRONT_RIGHT,    sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][18],  &FRONT_RIGHT,    sizeof(D3DXVECTOR3) );
                CopyMemory( &m_vertices[p][t][s][19],  &BACK_RIGHT,     sizeof(D3DXVECTOR3) );

                for( i=3; i < VERTS_PER_CUBE; i += 4 )
                {
                    m_vertices[p][t][s][i-3].u = 1.0f;
                    m_vertices[p][t][s][i-3].v = 1.0f;

                    m_vertices[p][t][s][i-2].u = 0.0f;
                    m_vertices[p][t][s][i-2].v = 1.0f;

                    m_vertices[p][t][s][i-1].u = 0.0f;
                    m_vertices[p][t][s][i-1].v = 0.0f;

                    m_vertices[p][t][s][i].u = 1.0f;
                    m_vertices[p][t][s][i].v = 0.0f;
                }
                

                for( i=0; i < VERTS_PER_CUBE; i++ )
                {
                    m_vertices[p][t][s][i].color = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f);;
                }

            }
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: This creates all device-dependent managed objects, such as managed
//       textures and managed vertex buffers.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::InitDeviceObjects()
{
    HRESULT hr = S_OK;

    
    // Create a texture
    hr = D3DXCreateTextureFromResource( m_pd3dDevice, g_hInstance, MAKEINTRESOURCE(IDR_CUBEFACE), &m_pCubeTex );
    if( FAILED(hr) )
    {
        return DXTRACE_ERR( TEXT("D3DXCreateTextureFromFile"), hr );
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Restore device-memory objects and state after a device is created or
//       resized.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::RestoreDeviceObjects()
{
    HRESULT hr = S_OK;
    UINT i;
    int t, s, p; // Loop variables

    D3DXMATRIXA16 matWorld;
    D3DXMatrixIdentity( &matWorld );
    m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    D3DXMATRIXA16 matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 0.5f, 10.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

    const WORD anIndices[] = { 0,  1,  2,  0,  2,  3,
                               4,  5,  6,  4,  6,  7,
                               8,  9,  10, 8,  10, 11,
                               12, 13, 14, 12, 14, 15,
                               16, 17, 18, 16, 18, 19, };
    const WORD nNumIndices = sizeof(anIndices) / sizeof(anIndices[0]);

    // Create the vertex buffer.
    hr = m_pd3dDevice->CreateVertexBuffer( sizeof(m_vertices),
                                           D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC, D3DFVF_SPECTRUMVERTEX,
                                           D3DPOOL_DEFAULT, &m_pVB, NULL );

    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("CreateVertexBuffer"), hr );
  
    

    // Create the index buffer
    hr = m_pd3dDevice->CreateIndexBuffer( RENDER_PASSES * 3 * TRIS_PER_CUBE * sizeof(WORD) * NUM_SECTORS * NUM_TRACKS,
                                          D3DUSAGE_WRITEONLY,
                                          D3DFMT_INDEX16,
                                          D3DPOOL_DEFAULT, 
                                          &m_pIB, NULL );
    
    if( FAILED(hr) )
        return DXTRACE_ERR( TEXT("CreateIndexBuffer"), hr );


    // Fill the index buffer
    WORD* pIndices;
    if( FAILED( hr = m_pIB->Lock( 0, 0, 
                             (void**)&pIndices, 0 ) ) )
    {
        return DXTRACE_ERR( TEXT("m_pIB->Lock"), hr );
    }

    WORD curIndex = 0;

    // For each track
    for( t=0; t < NUM_TRACKS; t++ )
    {
        // For each sector
        for( s=0; s < NUM_SECTORS; s++ )
        {
            // For each render pass
            for( p=RENDER_PASSES-1; p >= 0; p-- )
            {
                for( i=0; i < nNumIndices; i++ )
                {
                    *pIndices++ = curIndex + anIndices[i];
                }

                curIndex += VERTS_PER_CUBE;
            }
        }
    }
    
    m_pIB->Unlock();

    // Turn off culling, so we see the front and back of the triangle
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE);

    // Turn off D3D lighting, since we are providing our own vertex colors
    m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE);
    
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE);

    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, D3DZB_FALSE );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Called when the device-dependent objects are about to be lost.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::InvalidateDeviceObjects()
{
    SAFE_RELEASE( m_pIB );
    SAFE_RELEASE( m_pVB );
    
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::DeleteDeviceObjects()
{
    SAFE_RELEASE( m_pCubeTex );
    
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::FinalCleanup()
{
   return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::FrameMove()
{
    HRESULT hr = S_OK;
    int i, s, t; // loop variables

    const float fAcceleration = -2.8f; // Meters per millisecond
    
    // Change all the colors based on the current levels
    int freqPerSector = (SA_BUFFER_SIZE-1) / (NUM_SECTORS+4);
    float levelPerTrack = (MAX_LEVEL - MIN_LEVEL) / NUM_TRACKS;

    RotateColors();
    CalculateColors();


    D3DXMATRIXA16 matView;
    D3DXVECTOR3 vEyePt( cosf( m_fTime/10.0f ), 0.7f, sinf( m_fTime/10.0f ) );
    D3DXVECTOR3 vLookAt( cosf( m_fTime/10.0f )/3.0f, 0.0f, sinf( m_fTime/10.0f )/3.0f );
    D3DXVECTOR3 vUp( 0.0f, 1.0f, 0.0f );
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookAt, &vUp );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    const int anActiveVertices[] = { 0, 1, 2, 3, 6, 7, 10, 11, 14, 15, 18, 19 };
    const int nNumActiveVertices = sizeof(anActiveVertices) / sizeof(anActiveVertices[0]);
            
    // For each track
    for( t=0; t < NUM_TRACKS; t++ )
    {
        // For each sector
        for( s=0; s < NUM_SECTORS; s++ )
        {
            
            D3DXCOLOR curColor = m_vertices[1][t][s][0].color;
            FLOAT yValue = m_vertices[1][t][s][0].y;
            
            // Decrement the alpha
            // Based on 30-frames per second, persist 97% of current alpha
            // each frame
            curColor.a *= powf( .97f, 30 * m_fElapsedTime );

            // Emit a new cube in the frequency is above the threshold, there isn't already
            // an active cube, and the reserve falling cube has already fallen a certain
            // distance
            if( m_pAudioLevels->frequency[1][(s+2) * freqPerSector] > t * levelPerTrack + MIN_LEVEL && 
                yValue < 0.0f && 
                m_vertices[0][t][s][0].y < -0.75f )
            {   
                CopyMemory( &m_vertices[0][t][s][0], &m_vertices[1][t][s][0], 4 * sizeof(SPECTRUMVERTEX) );
                m_velocities[0][t][s] = m_velocities[1][t][s];
                yValue = 0.01f;
                m_velocities[1][t][s] = 1.5f * (m_pAudioLevels->frequency[1][(s+2) * freqPerSector] / 256.0f);
                curColor = m_colorsOn[t];   
            }
         
            
            // Refresh active cubes
            yValue += m_velocities[1][t][s] * m_fElapsedTime;
            m_velocities[1][t][s] += fAcceleration * m_fElapsedTime;

            for( i=0; i < nNumActiveVertices; i++ )
            {
                m_vertices[1][t][s][ anActiveVertices[i] ].color = curColor;
                m_vertices[1][t][s][ anActiveVertices[i] ].y = yValue;
            }
            
            // Refresh falling cubes
            curColor = m_vertices[0][t][s][0].color;
            yValue = m_vertices[0][t][s][0].y;

            // Decrement the alpha
            // Based on 30-frames per second, persist 85% of current alpha
            // each frame
            curColor.a *= powf( 0.85f, 30 * m_fElapsedTime );

            yValue += m_velocities[0][t][s] * m_fElapsedTime;
            m_velocities[0][t][s] += fAcceleration * m_fElapsedTime;
            
            for( i=0; i < nNumActiveVertices; i++ )
            {
                m_vertices[0][t][s][ anActiveVertices[i] ].color = curColor;
                m_vertices[0][t][s][ anActiveVertices[i] ].y = yValue;
            }
        }
    }
    
    
    // Fill the vertex buffer.
    VOID* pVertices;
    if( FAILED( hr = m_pVB->Lock( 0, 0, (void**)&pVertices, D3DLOCK_DISCARD ) ) )
    {
        return DXTRACE_ERR( TEXT("m_pVB->Lock"), hr );
    }
    memcpy( pVertices, m_vertices, sizeof(m_vertices) );
    m_pVB->Unlock();

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::Render()
{
    // Clear the backbuffer to a black color
    m_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_RGBA(0,0,0,0), 1.0f, 0 );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
        m_pd3dDevice->SetStreamSource( 0, m_pVB, 0, sizeof(SPECTRUMVERTEX) );
        m_pd3dDevice->SetIndices( m_pIB );
        m_pd3dDevice->SetFVF( D3DFVF_SPECTRUMVERTEX );

        m_pd3dDevice->SetTexture( 0, m_pCubeTex );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
        m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

        m_pd3dDevice->DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0,
                                            sizeof(m_vertices) / sizeof(SPECTRUMVERTEX),
                                            0, RENDER_PASSES * NUM_SECTORS * NUM_TRACKS * TRIS_PER_CUBE );


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
HRESULT CSpectrumWheel::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
                                       D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CalculateColors()
// Desc: Perform a gradient on the vertex colors between the center color and
//       outer color
//-----------------------------------------------------------------------------
void CSpectrumWheel::CalculateColors()
{   
    // For each track
    for( int t=0; t < NUM_TRACKS; t++ )
    {
        float radius = INNER_RADIUS + (t * DISTANCE_PER_TRACK) + SPACING_PER_TRACK;

        m_colorsOn[t] = ((m_colorInner * (WHEEL_RADIUS - radius)) + 
                         (m_colorOuter * radius)) / WHEEL_RADIUS;
    }
}




//-----------------------------------------------------------------------------
// Name: ShiftColor()
// Desc: Per color channel helper for RotateColors
//-----------------------------------------------------------------------------
void CSpectrumWheel::ShiftColor( DWORD& dwColor, int& offset )
{
    // Have a new offset ready if the current one will send the color out of
    // bounds. This offset must be at least 1 to make sure the color is
    // changed and should take into account framerate to make sure colors
    // change at consistent rates across different hardware.
    int newOffset = 1 + (int)( rand()%5 * 30*m_fElapsedTime );
    
    if( (int)dwColor + offset > 255 )
        offset = newOffset;
    else if( (int)dwColor + offset < 0 )
        offset = -newOffset;

    dwColor += offset;
}




//-----------------------------------------------------------------------------
// Name: RotateColors()
// Desc: Simple color cycling between frames
//-----------------------------------------------------------------------------
HRESULT CSpectrumWheel::RotateColors()
{
    static offset[3] = {256, 256, 256};

    DWORD dwColor = m_colorInner;
    DWORD dwRed = (dwColor & 0x00ff0000) >> 16;
    DWORD dwGreen = (dwColor & 0x0000ff00) >> 8;
    DWORD dwBlue = (dwColor & 0x000000ff);

    ShiftColor( dwRed, offset[0] );
    ShiftColor( dwGreen, offset[1] );
    ShiftColor( dwBlue, offset[2] );

    m_colorInner = D3DCOLOR_ARGB( 255, dwRed, dwGreen, dwBlue );
    
    return S_OK;
}
