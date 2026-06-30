
// The texture we are going to write to in this shader (uhhh not sure if width matters a lot here, like the float type)
[[vk::image_format("rgba16f")]]
[[vk::binding(0, 0)]]   // descriptor set 0, binding 0. If I just said binding(0), that defaults the dset to 0.
RWTexture2D<float4> OutputImage;

[numthreads(16, 16, 1)] 
void main(uint3 dispatchThreadID : SV_DispatchThreadID)     // this : syntax means 'get meaningful name on GPU shader and assign here'
{
    int imageWidth, imageHeight;
    OutputImage.GetDimensions(imageWidth, imageHeight);

    // #JOHN - is the dispatch thread ID global or per block? seems kinda per block but idk
    // #JOHN how does this ID get passed into the compute shader?
    // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/sv-dispatchthreadid
    // seems like SV_GroupThreadID might be closer to what I am looking for but how do i get it in there?
    // can you only pass tihngs in via parameters or is it more global like in GLSL?
    if (dispatchThreadID.x < imageWidth 
        && dispatchThreadID.y < imageHeight)
    {
        // set to blue as a test
        OutputImage[dispatchThreadID.xy] = float4(0.0, 0.0, 1.0, 1.0);
    }
}

// #JOHN - once i have a few more shaders, write some automation for this:
// currently used this cmd:
// dxc -spirv -T cs_6_4 .\gradient\gradient.comp.hlsl -Fo .\gradient.comp.spv
// and I have the vulkan sdk in my path... though I could query that. it is nice that dxc comes with the vulkan sdk