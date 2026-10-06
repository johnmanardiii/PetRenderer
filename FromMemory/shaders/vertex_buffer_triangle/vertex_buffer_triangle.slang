
struct VSInput
{
// #JOHN - is there a way to do it without this vk location thing?
    [[vk::location(0)]] float3 Pos;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
};


// .\hardcoded_triangle.slang -target spirv -entry vertexMain -o hardcoded_triangle.vert.spv
[shader("vertex")]
VSOutput vertexMain(VSInput input, uniform matrix<float, 3, 3> rotation)
{
    VSOutput output;
    output.Pos = float4(mul(rotation, input.Pos), 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain ()
{
    // return the color red yolo
    return float4(1.0, 0.0, 0.0, 1.0);
}