//-----------------------------------------------------------------------------
// File: SHPRTVertex.fx
//
// Desc: The technique PrecomputedSHLighting renders the scene with per vertex PRT
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
float4x4 mWorldViewProjection;

#define MAX_NUM_CHANNELS	3
// The values for NUM_CLUSTERS and NUM_PCA_VECTORS are
// defined by the app upon the D3DXCreateEffectFromFile() call.

float4 vClusteredPCA[NUM_CLUSTERS*(1+MAX_NUM_CHANNELS*(NUM_PCA_VECTORS/4))];


//-----------------------------------------------------------------------------
// Vertex shader output structure
//-----------------------------------------------------------------------------
struct VS_OUTPUT
{
    float4 Position  : POSITION;    // position of the vertex
    float4 Diffuse   : COLOR0;      // diffuse color of the vertex
};


//-----------------------------------------------------------------------------
// Name: SHPRTDiffuseVS
// Type: Vertex shader                                      
// Desc: Renders using per vertex SH PRT with CPCA compression
//-----------------------------------------------------------------------------
VS_OUTPUT SHPRTDiffuseVS( float4 vPos : POSITION,
                          int iClusterOffset : BLENDWEIGHT,
                          float4 vPCAWeights[NUM_PCA_VECTORS/4] : BLENDWEIGHT1 )
{
    VS_OUTPUT Output;
    
    // Output the vetrex position in projection space
    Output.Position = mul(vPos, mWorldViewProjection);
    
    // With SH PRT using CPCA, a single diffuse channel is caluated by:
    //       R[p] = (M[k] dot L') + sum( w[p][j] * (B[k][j] dot L');
    // where the sum runs j between 0 and # of PCA vectors
    //       R[p] = exit radiance at point p
    //       M[k] = mean of cluster k 
    //       L' = source radiance coefficients
    //       w[p][j] = the j'th PCA weight for point p
    //       B[k][j] = the j'th PCA basis vector for cluster k
    //
    // Note: since both (M[k] dot L') and (B[k][j] dot L') can be computed on the CPU, 
    // these values are passed in as the array vClusteredPCA.   
           
    float4 vExitR = float4(0,0,0,0);
    float4 vExitG = float4(0,0,0,0);
    float4 vExitB = float4(0,0,0,0);
    
    // For each channel, multiply and sum all the vPCAWeights[j] by vClusteredPCA[x] 
    // where: vPCAWeights[j] is w[p][j]
    //		  vClusteredPCA[x] is the value of (B[k][j] dot L') that was
    //		  calculated on the CPU and passed in as a shader constant
    // Note this code is multipled and added 4 floats at a time since each 
    // register is a 4-D vector, and is the reason for using (NUM_PCA_VECTORS/4)
    for (int j=0; j < (NUM_PCA_VECTORS/4); j++) 
    {
        vExitR += vPCAWeights[j] * vClusteredPCA[iClusterOffset+j+1+(NUM_PCA_VECTORS/4)*0];
        vExitG += vPCAWeights[j] * vClusteredPCA[iClusterOffset+j+1+(NUM_PCA_VECTORS/4)*1];
        vExitB += vPCAWeights[j] * vClusteredPCA[iClusterOffset+j+1+(NUM_PCA_VECTORS/4)*2];
    }    

	// Now for each channel, sum the 4D vector and add vClusteredPCA[x] 
	// where: vClusteredPCA[x] which is the value of (M[k] dot L') and
	//		  was calculated on the CPU and passed in as a shader constant.
    float4 vExitRadiance = vClusteredPCA[iClusterOffset];
    vExitRadiance.r += dot(vExitR,1);
    vExitRadiance.g += dot(vExitG,1);
    vExitRadiance.b += dot(vExitB,1);

    // For spectral simulations the material properity is baked into the transfer coefficients.
    // If using nonspectral, then you can modulate by the diffuse material properity here.
    Output.Diffuse = vExitRadiance;
    
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
PS_OUTPUT StandardPS( VS_OUTPUT In ) 
{ 
    PS_OUTPUT Output;

    Output.RGBColor = In.Diffuse;

    return Output;
}


//-----------------------------------------------------------------------------
// Name: PrecomputedSHLighting
// Type: Technique                                     
// Desc: Renders with per vertex PRT 
//-----------------------------------------------------------------------------
technique PrecomputedSHLighting
{
    pass P0
    {          
        VertexShader = compile vs_1_1 SHPRTDiffuseVS();
        PixelShader  = compile ps_1_1 StandardPS(); // trival pixel shader (could use FF instead if desired)
    }
}

