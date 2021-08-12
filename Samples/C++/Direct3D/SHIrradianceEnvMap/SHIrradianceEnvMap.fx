//-----------------------------------------------------------------------------
// File: SHIrradianceEnvMap.fx
//
// Desc: 
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
float4x4 mWorldViewProjection;
float4   MaterialDiffuseColor     = { 1.0f, 1.0f, 1.0f, 1.0f };    // diffuse
texture  MeshTexture;

float4 cAr;
float4 cAg;
float4 cAb;
float4 cBr;
float4 cBg;
float4 cBb;
float4 cC;


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
// Name: IrradianceEnvironmentMapVS
// Type: Vertex shader                                      
// Desc: 
//-----------------------------------------------------------------------------
VS_OUTPUT IrradianceEnvironmentMapVS( float4 vPos : POSITION, 
                                      float3 vNormal : NORMAL,
                                      float2 vTexCoord0 : TEXCOORD0 )
{
    VS_OUTPUT Output;
    
    // Output the vetrex position in projection space
    Output.Position = mul(vPos, mWorldViewProjection);
    
    // Linear + constant polynomial terms
    float3 linearConstantColor;
    float4 vA = float4(vNormal,1.0f);   
    linearConstantColor.r = dot(cAr,vA);
    linearConstantColor.g = dot(cAg,vA);
    linearConstantColor.b = dot(cAb,vA);
    
    // 4 of the quadratic polynomials
    float3 firstQuadraticColor;
    float4 vB = vNormal.xyzz * vNormal.yzzx;   
    firstQuadraticColor.r = dot(cBr,vB);
    firstQuadraticColor.g = dot(cBg,vB);
    firstQuadraticColor.b = dot(cBb,vB);
   
    // Final quadratic polynomial
    float vC = vNormal.x*vNormal.x - vNormal.y*vNormal.y;
    float3 finalQuadraticColor = cC.rgb * vC;    

    float4 LightColor;
    LightColor.rgb = linearConstantColor + firstQuadraticColor + finalQuadraticColor;
    LightColor.a   = 1.0f;
	
    Output.Diffuse = MaterialDiffuseColor * LightColor; 
    
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
// Name: StandardPS
// Type: Pixel shader
// Desc: Trival pixel shader (could use FF if desired)
//-----------------------------------------------------------------------------
PS_OUTPUT StandardPS( VS_OUTPUT In ) 
{ 
    PS_OUTPUT Output;

    Output.RGBColor = tex2D(MeshTextureSampler, In.TextureUV) * In.Diffuse;

    return Output;
}


//-----------------------------------------------------------------------------
// Name: IrradianceEnvironmentMap
// Type: Technique                                     
// Desc: 
//-----------------------------------------------------------------------------
technique IrradianceEnvironmentMap
{
    pass P0
    {          
        VertexShader = compile vs_1_1 IrradianceEnvironmentMapVS();
        PixelShader  = compile ps_1_1 StandardPS(); // trival pixel shader (could use FF if desired)
    }
}




