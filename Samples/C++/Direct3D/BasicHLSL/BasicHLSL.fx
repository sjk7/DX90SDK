//-----------------------------------------------------------------------------
// File: BasicHLSL.fx
//
// Desc: The effect file for the BasicHLSL sample.  It implemetns 
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
float4 g_MaterialAmbientColor;	// Material's ambient color
float4 g_MaterialDiffuseColor;	// Material's diffuse color

// This effect file uses a single directional light
float3 g_LightDir = normalize(float3(1.0f, 1.0f, 1.0f));	// Light's direction in world space
float4 g_LightAmbient = { 1.0f, 1.0f, 1.0f, 1.0f };			// Light's ambient color
float4 g_LightDiffuse = { 1.0f, 1.0f, 1.0f, 1.0f };			// Light's diffuse color

texture g_RenderTargetTexture;	// Full screen render target texture 
texture g_MeshTexture;			// Color texture for mesh

float	 g_fTime;					// App's time in seconds
float4x4 g_mWorld;					// World matrix for object
float4x4 g_mWorldViewProjection;	// World * View * Projection matrix



//-----------------------------------------------------------------------------
// Texture samplers
//-----------------------------------------------------------------------------
sampler RenderTargetSampler = 
sampler_state
{
    Texture = <g_RenderTargetTexture>;
    MinFilter = POINT;  
    MagFilter = POINT;
    MipFilter = NONE;
    AddressU = Clamp;
    AddressV = Clamp;
};

sampler MeshTextureSampler = 
sampler_state
{
    Texture = <g_MeshTexture>;    
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};


//-----------------------------------------------------------------------------
// Vertex shader output structure
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position   : POSITION;   // vertex position 
    float4 Diffuse    : COLOR0;     // vertex diffuse color
    float2 TextureUV  : TEXCOORD0;  // vertex texture coords 
};


//-----------------------------------------------------------------------------
// Name: RenderSceneVS
// Type: Vertex shader                                      
// Desc: This shader computes standard transform and lighting
//-----------------------------------------------------------------------------
VS_OUTPUT RenderSceneVS( float4 vPos : POSITION, 
                         float3 vNormal : NORMAL,
                         float2 vTexCoord0 : TEXCOORD0 )
{
    VS_OUTPUT Output;
    float3 vNormalWorldSpace;
  
	// Animation the vertex based on time and the vertex's object space position
    float4 vAnimatedPos = vPos + float4( sin(g_fTime) * vPos.x/2, 0, 0, 0 );
    
    // Transform the position from object space to homogeneous projection space
    Output.Position = mul(vAnimatedPos, g_mWorldViewProjection);
    
    // Transform the normal from object space to world space	
    vNormalWorldSpace = normalize(mul(vNormal, (float3x3)g_mWorld)); // normal (world space)
    
    // Compute simple directional lighting equation
    Output.Diffuse.rgb = g_MaterialDiffuseColor * g_LightDiffuse * max(0,dot(vNormalWorldSpace, g_LightDir)) + 
                         g_MaterialAmbientColor * g_LightAmbient;   
    Output.Diffuse.a = 1.0f; 
    
    // Just copy the texture coordinate through
    Output.TextureUV = vTexCoord0; 
    
    return Output;    
}


//-----------------------------------------------------------------------------
// Pixel shader output structure
//-----------------------------------------------------------------------------
struct PS_OUTPUT
{
    float4 RGBColor : COLOR0;  // Pixel color    
};


//-----------------------------------------------------------------------------
// Name: RenderScenePS                                        
// Type: Pixel shader
// Desc: This shader outputs the pixel's color by modulating the texture's
//		 color with diffuse material color
//-----------------------------------------------------------------------------
PS_OUTPUT RenderScenePS( VS_OUTPUT In ) 
{ 
    PS_OUTPUT Output;

    // Lookup mesh texture and modulate it with diffuse
    Output.RGBColor = tex2D(MeshTextureSampler, In.TextureUV) * In.Diffuse;

    return Output;
}



//-----------------------------------------------------------------------------
// Name: RenderScene
// Type: Technique                                     
// Desc: Renders scene to render target
//-----------------------------------------------------------------------------
technique RenderScene
{
    pass P0
    {          
        VertexShader = compile vs_1_1 RenderSceneVS();
        PixelShader  = compile ps_1_1 RenderScenePS(); // trival pixel shader (could use FF instead if desired)
    }
}
