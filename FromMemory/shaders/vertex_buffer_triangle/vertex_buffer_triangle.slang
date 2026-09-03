
struct VSInput
{
    [[vk::location(0)]] float3 Pos;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
};

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;
    output.Pos = (input.Pos, 1.0f);
    return output;
}

[shader("fragment")]
float4 fragmentMain ()
{
    // return the color red yolo
    return float4(1.0, 0.0, 0.0, 1.0);
}