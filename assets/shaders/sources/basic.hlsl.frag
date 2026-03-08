struct FSInput {
    float4 color: COLOR;
};

float4 main(FSInput input) : SV_Target
{
    return input.color * 0.5 + 0.5; 
}
