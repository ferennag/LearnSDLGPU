struct VSIn {
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOut
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VSOut main(VSIn input){
    VSOut output;
    output.pos = float4(input.position, 1.0);
    output.color = float4(input.color, 1.0);
    return output;
}
