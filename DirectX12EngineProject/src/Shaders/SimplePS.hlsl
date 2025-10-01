// SimplePs.hlsl, Pixel Shader

struct PSInput
{
    float4 position : SV_Position; // 클립 공간 좌표
    float3 normal : NORMAL; // 보간용 normal
    float2 texcoord : TEXCOORD; // 보간용 uv
};

float4 PSMain(PSInput input) : SV_Target
{
    // Normal을 0~1 범위 컬러로 변환해서 출력 (디버깅용)
    float3 n = normalize(input.normal) * 0.5f + 0.5f;
    return float4(n, 1.0f);
}
