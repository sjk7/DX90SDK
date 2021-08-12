//-----------------------------------------------------------------------------
// File: SimpleLighting.fx
//
// Desc: The technique SimpleLighting renders the
//		 scene with standard N.L lighting.
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
float4x4 mWorldViewProjection;
texture  MeshTexture;

float4 MaterialDiffuseColor     = { 1.0f, 1.0f, 1.0f, 1.0f };    
float4 MaterialAmbientColor     = { 0.0f, 0.0f, 0.0f, 0.0f };    
float4 Light1DiffuseColor;       
float3 Light1Dir;
float4 Light2DiffuseColor;       
float3 Light2Dir;
float4 Light3DiffuseColor;       
float3 Light3Dir;


//-----------------------------------------------------------------------------
// Texture samplers
//-----------------------------------------------------------------------------
sampler MeshTextureSampler = 
sampler_state
{
    Texture = <MeshTexture>;
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
};


//-----------------------------------------------------------------------------
// Vertex shader output structure
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position  : POSITION;    // position of the vertex
    float4 Diffuse   : COLOR0;      // diffuse color of the vertex
    float2 TextureUV : TEXCOORD0;   // typical texture coords stored here
};


//-----------------------------------------------------------------------------
// Name: SimpleLightingVS
// Type: Vertex shader                                      
// Desc: Renders with standard N.L lighting
//-----------------------------------------------------------------------------
VS_OUTPUT SimpleLightingVS( float4 vPos : POSITION, 
                            float3 vNormal : NORMAL,
                            float2 vTexCoord0 : TEXCOORD0,
							uniform bool bTexture )
{
    VS_OUTPUT Output;
    
    // Output the vetrex position in projection space
    Output.Position = mul(vPos, mWorldViewProjection);    
    
    float4 light1Diffuse = Light1DiffuseColor * max(0, dot(vNormal, Light1Dir));
    float4 light2Diffuse = Light2DiffuseColor * max(0, dot(vNormal, Light2Dir));
    float4 light3Diffuse = Light3DiffuseColor * max(0, dot(vNormal, Light3Dir));
    Output.Diffuse = MaterialAmbientColor + MaterialDiffuseColor * (light1Diffuse + light2Diffuse + light3Diffuse);

	if( bTexture )
		Output.TextureUV = vTexCoord0;
	else
		Output.TextureUV = 0;
    
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
// Name: SHIPixelShader
// Type: Pixel shader
// Desc: Trival pixel shader
//-----------------------------------------------------------------------------
PS_OUTPUT StandardPS( VS_OUTPUT In,
				      uniform bool bTexture ) 
{ 
    PS_OUTPUT Output;

	if( bTexture )
		Output.RGBColor = tex2D(MeshTextureSampler, In.TextureUV) * In.Diffuse;
	else
		Output.RGBColor = In.Diffuse;

    return Output;
}


//-----------------------------------------------------------------------------
// Name: SimpleLighting
// Type: Technique                                     
// Desc: Renders with standard N.L lighting
//-----------------------------------------------------------------------------
technique SimpleLightingTex1
{
    pass P0
    {          
        VertexShader = compile vs_1_1 SimpleLightingVS( true ); // trival vertex shader (could use FF if desired)
        PixelShader  = compile ps_1_1 StandardPS( true );       // trival pixel shader (could use FF if desired)
    }
}


//-----------------------------------------------------------------------------
// Name: SimpleLighting
// Type: Technique                                     
// Desc: Renders with standard N.L lighting
//-----------------------------------------------------------------------------
technique SimpleLightingTex0
{
    pass P0
    {          
        VertexShader = compile vs_1_1 SimpleLightingVS( false ); // trival vertex shader (could use FF if desired)
        PixelShader  = compile ps_1_1 StandardPS( false );       // trival pixel shader (could use FF if desired)
    }
}


