struct VSOut
{
    float4 pos : SV_Position;
};

VSOut main(uint vid : SV_VertexID)
{
    // Fullscreen triangle in NDC
    float2 p;
    if (vid == 0)      p = float2(-0.5, -0.5);
    else if (vid == 1) p = float2( 0.5, -0.5);
    else               p = float2(0.0,  0.5);

    VSOut o;
    o.pos = float4(p, 0.0, 1.0);
    return o;
}
