

// slangc push_constant_triangle.slang -target spirv -o push_constant_triangle.vert.spv -entry vertexMain -matrix-layout-column-major
// slangc push_constant_triangle.slang -target spirv -o push_constant_triangle.frag.spv -entry fragmentMain -matrix-layout-column-major
struct Vertex
{
    float3 pos;
    float u;
    float3 norm;
    float v;
};

struct VertexStructuredBuffer
{
    StructuredBuffer<Vertex> vertices;
};

ParameterBlock<VertexStructuredBuffer> vertexBuffer;

// push constants
struct PushConstants
{
    matrix<float, 4, 4> MVP;
};

struct VSOutput
{
    float4 Pos : SV_POSITION;
    float3 color;
};

[[vk::push_constant]] uniform PushConstants matAndVertexBuff;

[shader("vertex")]
VSOutput vertexMain(uint instanceID : SV_VertexID)
{
    VSOutput output;
    Vertex v = vertexBuffer.vertices[instanceID];
    output.Pos = mul(matAndVertexBuff.MVP, float4(v.pos, 1.0f));
    output.color = v.norm;
    return output;
}

[shader("fragment")]
float4 fragmentMain (VSOutput input)
{
    // return normal as color for now
    return float4(input.color, 1.0);
}