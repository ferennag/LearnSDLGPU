cbuffer FrameUbo: register(b0, space1) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
}

struct VSIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOut
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VSOut main(VSIn input){
    VSOut output;
    output.pos = mul(projection, mul(view, mul(model, float4(input.position, 1.0))));
    output.color = float4(input.normal, 1.0);
    return output;
}
