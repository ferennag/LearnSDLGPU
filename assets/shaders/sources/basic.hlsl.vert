cbuffer FrameUbo: register(b0, space1) {
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float3 cameraPosition;
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
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
    float3 worldNormal: NORMAL;
    float3 cameraPosition: TEXCOORD1;
    float4 worldPosition: TEXCOORD2;
};

VSOut main(VSIn input){
    VSOut output;
    float4 worldPosition = mul(model, mul(meshTransform, float4(input.position, 1.0)));
    output.position = mul(projection, mul(view, worldPosition));
    output.texCoord = input.texCoord;
    output.worldNormal = mul((float3x3)model, mul((float3x3)meshTransform, input.normal));
    output.cameraPosition = cameraPosition;
    output.worldPosition = worldPosition;
    return output;
}
