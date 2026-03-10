struct FSInput {
    float2 texCoord : TEXCOORD;
    float3 worldNormal: NORMAL;
    float3 cameraPosition: TEXCOORD1;
    float4 worldPosition: TEXCOORD2;
};

cbuffer Material: register(b0, space1) {
    float4 baseColorFactor;
    float4 diffuseFactor;
    float4 specularFactor;
    float3 emissiveFactor;
    float emissiveStrength;
    float metallicFactor;
    float roughnessFactor;
    float glossinessFactor;
}

SamplerState BaseColorSampler: register(s0, space2);
Texture2D BaseColor: register(t0, space2);

SamplerState MetallicRoughnessSampler: register(s1, space2);
Texture2D MetallicRoughness: register(t1, space2);

SamplerState SpecularGlossinessSampler: register(s2, space2);
Texture2D SpecularGlossiness: register(t2, space2);

SamplerState NormalSampler: register(s3, space2);
Texture2D Normal: register(t3, space2);

SamplerState DiffuseSampler: register(s4, space2);
Texture2D Diffuse: register(t4, space2);

SamplerState OcclusionSampler: register(s5, space2);
Texture2D Occlusion: register(t5, space2);

SamplerState EmissiveSampler: register(s6, space2);
Texture2D Emissive: register(t6, space2);

float4 main(FSInput input) : SV_Target {
    float3 N = normalize(input.worldNormal);
    float3 L = normalize(float3(0.5, 1.0, 0.3));
    float3 V = normalize(input.cameraPosition - input.worldPosition); // simple fake camera dir
    float3 H = normalize(L + V);

    float4 baseColor = BaseColor.Sample(BaseColorSampler, input.texCoord) * baseColorFactor;

    float ambient = 0.15;
    float diffuse = max(dot(N, L), 0.0);

    float4 mrSample = MetallicRoughness.Sample(MetallicRoughnessSampler, input.texCoord);
    float roughness = mrSample.g * roughnessFactor;
    float metallic  = mrSample.b * metallicFactor;

    float shininess = lerp(128.0, 4.0, roughness);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    // Non-metals have white-ish specular, metals tint specular by base color
    float3 specColor = lerp(float3(0.04, 0.04, 0.04), baseColor.rgb, metallic);
    float3 diffuseColor = baseColor.rgb * (1.0 - metallic);

    float4 output = float4(diffuseColor * (ambient + diffuse) + specColor * spec, baseColor.a);
    return output;
}
