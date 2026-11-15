// this is me trying to rewrite the gradient shader (that really just outputs blue) to slang since slang
// looks easier to compile to spirv... and this is my own project so i can do as i please.
// oh woah the "module" thing in slang looks sick...

// slangc gradient.comp.slang -profile glsl_450 -target spirv -o gradient.comp.spv -entry computeMain
RWTexture2D<float4> resultImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void computeMain(uint3 GlobalDispatchID : SV_DispatchThreadID)
{
    int imageWidth, imageHeight;
    resultImage.GetDimensions(imageWidth, imageHeight);

    if (GlobalDispatchID.x < imageWidth &&
        GlobalDispatchID.y < imageHeight)
    {
        if (GlobalDispatchID.x % 16 == 0
        || GlobalDispatchID.y % 16 == 0)
        {
            resultImage[GlobalDispatchID.xy] = float4(0.0, 0.0, 0.0, 1.0);
        }
        else
        {
            float r = float(GlobalDispatchID.x) / imageWidth;
            float g = float(GlobalDispatchID.y) / imageHeight;
            resultImage[GlobalDispatchID.xy] = float4(r, g, 0.0, 1.0);
        }
    }
}