cbuffer FrameUbo: register(b0, space1) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
}

cbuffer MeshUbo: register(b1, space1) {
    float4x4 meshTransform;
}

struct VSIn {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD;
};

struct VSOut
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

VSOut main(VSIn input){
    VSOut output;
    output.pos = mul(projection, mul(view, mul(model, mul(meshTransform, float4(input.position, 1.0)))));
    output.color = float4(input.normal, 1.0);
    output.texCoord = input.texCoord;
    return output;
}
