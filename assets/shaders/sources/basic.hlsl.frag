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

static const float PI = 3.14159265f;

// GGX/Trowbridge-Reitz Normal Distribution function
float D(float alpha, float3 N, float3 H) {
    float numerator = pow(alpha, 2.0);
    float NdotH = max(dot(N, H), 0.0);
    float denominator = PI * pow(pow(NdotH, 2.0) * (pow(alpha, 2.0) - 1.0) + 1.0, 2.0);
    denominator = max(denominator, 0.000001);
    return numerator / denominator;
}

// Schlick-Beckmann geometry shadowing function
float G1(float alpha, float3 N, float3 X) {
    float numerator = max(dot(N, X), 0.0);
    float k = alpha / 2.0;
    float denominator = max(dot(N,X), 0.0) * (1.0 - k) + k;
    denominator = max(denominator, 0.000001);
    return numerator/denominator;
}

// Smith model
float G(float alpha, float3 N, float3 V, float3 L) {
    return G1(alpha, N, V) * G1(alpha, N, L);
}

// Frescnel-Schlick
float3 F(float3 F0, float3 V, float3 H) {
    return F0 + (float3(1.0,1.0,1.0) - F0) * pow(1 - max(dot(V, H), 0.0), 5.0);
}

float4 main(FSInput input) : SV_Target {
    float lightIntensity = 8;
    float3 lightColor = float3(1.0,1.0,1.0) * lightIntensity;
    float3 N = normalize(input.worldNormal);
    float3 V = normalize(input.cameraPosition - input.worldPosition.xyz);
    float3 L = normalize(float3(0.0, 5.0, 1.0));
    float3 H = normalize(V + L);
    float4 baseColor = baseColorFactor * BaseColor.Sample(BaseColorSampler, input.texCoord);
    float alpha = baseColor.a;
    float roughness = roughnessFactor * MetallicRoughness.Sample(MetallicRoughnessSampler, input.texCoord).g;
    float metallic = metallicFactor * MetallicRoughness.Sample(MetallicRoughnessSampler, input.texCoord).b;

    float3 F0 = float3(0.04, 0.04, 0.04);
    F0 = lerp(F0, baseColor.rgb, metallic);
    float3 Ks = F(F0, V, H);
    float3 Kd = float3(1.0,1.0,1.0) - Ks;

    float3 lambert = baseColor.rgb / PI;

    float3 cookTorranceNumerator = D(roughness, N, H) * G(roughness, N, V, L) * F(F0, V, H);
    float3 cookTorranceDenominator = 4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0);
    cookTorranceDenominator = max(cookTorranceDenominator, 0.0000001);
    float3 cookTorrance = cookTorranceNumerator / cookTorranceDenominator;

    float3 BRDF = Kd * lambert + cookTorrance;

    float3 emissive = Emissive.Sample(EmissiveSampler, input.texCoord).rgb * emissiveFactor;
    float3 output = emissive + BRDF * lightColor * max(dot(L, N), 0.0); 
    return float4(output, alpha);
}
