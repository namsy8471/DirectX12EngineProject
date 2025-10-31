// SimpleVS.hlsl

struct VSInput
{
    float3 position : POSITION; // Vertex.Position
    float3 normal : NORMAL; // Vertex.Normal
    float2 texcoord : TEXCOORD; // Vertex.TexCoord
    float3 tangent : TANGENT; // Vertex.Tangent
    float3 bitangent : BITANGENT; // Vertex.Bitangent
};


cbuffer SceneConstants : register(b0)
{
    float4x4 gWorldViewProj;
};

struct PSInput
{
    float4 position : SV_Position; // 클립 공간 좌표
    float3 normal : NORMAL; // 보간용 normal
    float2 texcoord : TEXCOORD; // 보간용 uv
};

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.position = mul(float4(input.position, 1.0f), gWorldViewProj);

    // 여기서는 그냥 normal을 컬러처럼 넘김 (디버깅용)
    result.normal = normalize(input.normal);
    result.texcoord = input.texcoord;

    return result;
}
