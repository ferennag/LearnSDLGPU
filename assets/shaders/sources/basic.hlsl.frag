struct FSInput {
    float4 color: COLOR;
    float2 texCoord : TEXCOORD;
};

Texture2D Texture: register(t0, space2);
SamplerState Sampler: register(s0, space2);

float4 main(FSInput input) : SV_Target
{
    return Texture.Sample(Sampler, input.texCoord);
    // return input.color * 0.5 + 0.5; 
}
