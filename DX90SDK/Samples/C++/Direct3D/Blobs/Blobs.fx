//-----------------------------------------------------------------------------
// File: Blobs.fx
//
// Desc: Effect file for the Blobs sample. 
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
static const float GAUSSIANTEXSIZE = 64;
static const float TEMPTEXSIZE = 1024;
static const float THRESHOLD = 0.08f;

float4 g_vMaterialAmbient  = { 0.0f, 0.0f, 0.0f, 0.0f };
float4 g_vMaterialDiffuse  = { 0.0f, 0.0f, 0.0f, 0.0f };
float4 g_vMaterialSpecular = { 1.0f, 1.0f, 1.0f, 1.0f };
float4 g_fMaterialPower    = 40.0f;

float3 g_vLightDir      = { 0.0f, 0.0f, 1.0f };
float4 g_vLightAmbient  = { 0.3f, 0.3f, 0.3f, 1.0f };    
float4 g_vLightDiffuse  = { 0.5f, 0.5f, 0.5f, 1.0f };    
float4 g_vLightSpecular = { 1.0f, 1.0f, 1.0f, 1.0f };    

float4x4 g_mWorldViewProjection;

// Textures
texture g_tSourceBlob;
texture g_tSurfaceBuffer;
texture g_tMatrixBuffer;
texture g_tEnvMap;


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
sampler SourceBlobSampler = 
sampler_state
{
    Texture = <g_tSourceBlob>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;

    AddressU = Clamp;
    AddressV = Clamp;
};

sampler SurfaceBufferSampler = 
sampler_state
{
    Texture = <g_tSurfaceBuffer>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;

    AddressU = Clamp;
    AddressV = Clamp;
};

sampler MatrixBufferSampler = 
sampler_state
{
    Texture = <g_tMatrixBuffer>;
    MinFilter = Point;
    MagFilter = Point;
    MipFilter = Point;

    AddressU = Clamp;
    AddressV = Clamp;
};

sampler EnvMapSampler =
sampler_state
{
    Texture = <g_tEnvMap>;
    MinFilter = Linear;
    MagFilter = Linear;
    MipFilter = Linear;

    AddressU = Clamp;
    AddressV = Clamp;
};

//-----------------------------------------------------------------------------
// Vertex/pixel shader output structures
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 vPosition : POSITION;
    float2 tCurr     : TEXCOORD0;
    float2 tBack     : TEXCOORD1;
    float2 sInfo     : TEXCOORD2;
    float3 vColor    : TEXCOORD3;
};


struct PS_OUTPUT
{
    float4 vColor[2] : COLOR;
};




//-----------------------------------------------------------------------------
// Name: DoLerp
// Type: Helper function                                      
// Desc: Peform a linear interpolation
//-----------------------------------------------------------------------------
void DoLerp( in float2 tCurr, out float4 outval )
{
    // Scale out into pixel space
    float2 pixelpos = GAUSSIANTEXSIZE * tCurr;
    
    // Determine the lerp amounts           
    float2 lerps = frac( pixelpos );
    
    // Get the upper left position                      
    float3 lerppos = float3((pixelpos-(lerps/GAUSSIANTEXSIZE))/GAUSSIANTEXSIZE,0);  

    float4 sourcevals[4];
    sourcevals[0] = tex2D( SourceBlobSampler, lerppos );  
    sourcevals[1] = tex2D( SourceBlobSampler, lerppos + float3(1.0/GAUSSIANTEXSIZE, 0,0) );
    sourcevals[2] = tex2D( SourceBlobSampler, lerppos + float3(0, 1.0/GAUSSIANTEXSIZE,0) );
    sourcevals[3] = tex2D( SourceBlobSampler, lerppos + float3(1.0/GAUSSIANTEXSIZE, 1.0/GAUSSIANTEXSIZE,0) );
    
    // Bilinear filtering
    outval = lerp( lerp( sourcevals[0], sourcevals[1], lerps.x ),
                   lerp( sourcevals[2], sourcevals[3], lerps.x ),
                   lerps.y );
}            




//-----------------------------------------------------------------------------
// Name: BlobVS
// Type: Vertex shader                                      
// Desc: Transforms the vertex from object to screen space
//-----------------------------------------------------------------------------
VS_OUTPUT BlobVS( float4 vPos : POSITION, 
                  float2 t0   : TEXCOORD0,
                  float2 t1   : TEXCOORD1,
                  float2 t2   : TEXCOORD2,
                  float3 t3   : TEXCOORD3 )
{
    VS_OUTPUT Output;
    

    // Tranform vertex position from object space into screen space
    Output.vPosition = mul( vPos, g_mWorldViewProjection );
    
    // Fill the remaining structure fields as passed to the shader
    Output.tCurr  = t0;
    Output.tBack  = t1;
    Output.sInfo  = t2;
    Output.vColor = t3;
    
    return Output;    
}




//-----------------------------------------------------------------------------
// Name: BlobBlenderPS
// Type: Pixel shader
// Desc: 
//-----------------------------------------------------------------------------
PS_OUTPUT BlobBlenderPS( VS_OUTPUT Input )
{ 
    PS_OUTPUT Output;
    float4 weight;
   
    // Get the new blob weight
    DoLerp( Input.tCurr, weight );

    // Get the old data
    float4 oldsurfdata = tex2D( SurfaceBufferSampler, Input.tBack );
    float4 oldmatdata  = tex2D( MatrixBufferSampler, Input.tBack );
    
    // Generate new surface data
    float4 newsurfdata = float4((Input.tCurr.x-0.5) * Input.sInfo.y,
                                (Input.tCurr.y-0.5) * Input.sInfo.y,
                                0,
                                1);
    newsurfdata *= weight.r;
    
    //generate new material properties
    float4 newmatdata = float4(Input.vColor.r,
                               Input.vColor.g,
                               Input.vColor.b,
                               0);
    newmatdata *= weight.r;
    
    // Additive blending
    Output.vColor[0] = newsurfdata + oldsurfdata; 
    Output.vColor[1] = newmatdata + oldmatdata;

    return Output;
}




//-----------------------------------------------------------------------------
// Name: BlobLightPS
// Type: Pixel shader
// Desc: 
//-----------------------------------------------------------------------------
float4 BlobLightPS( VS_OUTPUT Input ) : COLOR
{
    static const float aaval = THRESHOLD * 0.07f;

    float4 blobdata = tex2D( SourceBlobSampler, Input.tCurr);
    float4 color = tex2D( MatrixBufferSampler, Input.tCurr);
    
    color /= blobdata.w;
    
    float3 surfacept = float3(blobdata.x/blobdata.w,
                              blobdata.y/blobdata.w, 
                              blobdata.w-THRESHOLD); 
    float3 thenorm = normalize(-surfacept);
    thenorm.z = -thenorm.z;

    float4 Output;  
    Output.rgb = color.rgb + texCUBE( EnvMapSampler, thenorm );
    Output.rgb *= saturate ((blobdata.a - THRESHOLD)/aaval);
    Output.a=1;
    
    return Output;
}




//-----------------------------------------------------------------------------
// Name: BlobBlend
// Type: Technique                                     
// Desc: 
//-----------------------------------------------------------------------------
technique BlobBlend
{
    pass P0
    {   
        CullMode = NONE;
        ZEnable = FALSE;

             
        VertexShader = compile vs_1_1 BlobVS();
        PixelShader  = compile ps_2_0 BlobBlenderPS();
    }
}


//-----------------------------------------------------------------------------
// Name: BlobLight
// Type: Technique                                     
// Desc: 
//-----------------------------------------------------------------------------
technique BlobLight
{
    pass P0
    {   
		CullMode = NONE;
        ZEnable = FALSE;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        AlphaBlendEnable = TRUE;
           
		     
        VertexShader = compile vs_1_1 BlobVS();
        PixelShader  = compile ps_2_0 BlobLightPS();
    }
}


