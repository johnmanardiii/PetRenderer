//  slangc .\vertex_buffer_triangle.slang -target spirv -o .\vertex_buffer_triangle.vert.spv -entry vertexMain -matrix-layout-column-major

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
VSOutput vertexMain(VSInput input, uniform matrix<float, 4, 4> rotation)
{
    VSOutput output;
    output.Pos = mul(rotation, float4(input.Pos, 1.0f));
    return output;
}

[shader("fragment")]
float4 fragmentMain ()
{
    // return the color red yolo
    return float4(1.0, 0.0, 0.0, 1.0);
}