//-----------------------------------------------------------------------------
// File: SHPRTPixel.fx
//
// Desc: The technique PrecomputedSHLighting renders the scene with per pixel PRT
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
float4x4 mWorldViewProjection;

#define MAX_NUM_CHANNELS	3
// The values for NUM_PCA_VECTORS are
// defined by the app upon the D3DXCreateEffectFromFile() call.

float4 vClusteredPCA[1*(1+MAX_NUM_CHANNELS*(NUM_PCA_VECTORS/4))];

sampler texSampler[NUM_PCA_VECTORS/4] : register(s0);

//-----------------------------------------------------------------------------
// Vertex shader output structure
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position  : POSITION;    // position of the vertex
    float2 vTexCoord : TEXCOORD0;   // texture coordinate of the vertex
};


//-----------------------------------------------------------------------------
// Name: StandardVS
// Type: Vertex shader                                      
// Desc: Trival vertex shader
//-----------------------------------------------------------------------------
VS_OUTPUT StandardVS( float4 vPos : POSITION,
					  float2 vTexCoord : TEXCOORD0 )
{
    VS_OUTPUT Output;
    
    // Output the vetrex position in projection space
    Output.Position = mul(vPos, mWorldViewProjection);    
    
    Output.vTexCoord = vTexCoord;
    
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
// Name: SHPRTDiffusePS
// Type: Pixel shader                                      
// Desc: Renders using SH PRT with PCA compression
//-----------------------------------------------------------------------------
PS_OUTPUT SHPRTDiffusePS( VS_OUTPUT Input )
{
    PS_OUTPUT Output;    
    float4 vPCAWeights[NUM_PCA_VECTORS/4];
    
    for(int j=0; j<NUM_PCA_VECTORS/4; j++ )
    {
		vPCAWeights[j] = tex2D( texSampler[j], Input.vTexCoord );
    }
    
    // With SH PRT using PCA, a single diffuse channel is caluated by:
    //       R[p] = (M dot L') + sum( w[p][j] * (B[j] dot L');
    // where the sum runs j between 0 and # of PCA vectors
    //       R[p] = exit radiance at point p
    //       M = mean of the cluster 
    //       L' = source radiance coefficients
    //       w[p][j] = the j'th PCA weight for point p
    //       B[j] = the j'th PCA basis vector for the cluster 
    //
    // Note: since both (M dot L') and (B[j] dot L') can be computed on the CPU, 
    // these values are passed in as the array vClusteredPCA.   
           
    float4 vExitR = float4(0,0,0,0);
    float4 vExitG = float4(0,0,0,0);
    float4 vExitB = float4(0,0,0,0);
    
    // For each channel, multiply and sum all the vPCAWeights[j] by vClusteredPCA[x] 
    // where: vPCAWeights[j] is w[p][j]
    //		  vClusteredPCA[x] is the value of (B[j] dot L') that was
    //		  calculated on the CPU and passed in as a shader constant
    // Note this code is multipled and added 4 floats at a time since each 
    // register is a 4-D vector, and is the reason for using (NUM_PCA_VECTORS/4)
    for (int j=0; j < (NUM_PCA_VECTORS/4); j++) 
    {
        vExitR += vPCAWeights[j] * vClusteredPCA[j+1+(NUM_PCA_VECTORS/4)*0];
        vExitG += vPCAWeights[j] * vClusteredPCA[j+1+(NUM_PCA_VECTORS/4)*1];
        vExitB += vPCAWeights[j] * vClusteredPCA[j+1+(NUM_PCA_VECTORS/4)*2];
    } 

	// Now for each channel, sum the 4D vector and add vClusteredPCA[0] 
	// where: vClusteredPCA[0] which is the value of (M dot L') and
	//		  was calculated on the CPU and passed in as a shader constant.
    float4 vExitRadiance = vClusteredPCA[0];
    vExitRadiance.r += dot(vExitR,1);
    vExitRadiance.g += dot(vExitG,1);
    vExitRadiance.b += dot(vExitB,1);

    // For spectral simulations the material properity is baked into the transfer coefficients.
    // If using nonspectral, then you can modulate by the diffuse material properity here.
    Output.RGBColor = vExitRadiance;
    
    return Output;
}


//-----------------------------------------------------------------------------
// Name: PrecomputedSHLighting
// Type: Technique                                     
// Desc: Renders with per pixel PRT 
//-----------------------------------------------------------------------------
technique PrecomputedSHLighting
{
    pass P0
    {          
        VertexShader = compile vs_1_1 StandardVS(); // trival vertex shader (could use FF instead if desired)
        PixelShader  = compile ps_2_0 SHPRTDiffusePS(); 
    }
}

