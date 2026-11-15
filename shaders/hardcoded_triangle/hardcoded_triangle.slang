
// #JOHN - apparently 

[shader("vertex")]
float4 vertexMain (
    uint vertexID : SV_VertexID) : SV_Position
{
    // index into a set of positions on the screen initially
    // going to yolo specify these in NDC and hopefully it works
    float4 threeLittlePositions[] =
    {
        float4(-.5, -.5, 0., 1.),
        float4(.5, -.5, 0., 1.),
        float4(0., .5, 0., 1.)
    };

    return threeLittlePositions[vertexID];
}

[shader("fragment")]
float4 fragmentMain ()
{
    // return the color red yolo
    return float4(1.0, 0.0, 0.0, 1.0);
}