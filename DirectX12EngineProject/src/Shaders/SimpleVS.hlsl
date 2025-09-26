// 정점 셰이더의 출력이자 픽셀 셰이더의 입력이 될 구조체입니다.
// SEMANTIC(시맨틱)은 이 데이터가 파이프라인의 어떤 단계에서 사용될지 알려주는 이름표입니다.
struct PSInput
{
    float4 position : SV_Position; // 클립 공간 좌표. SV_Position은 필수 시스템 시맨틱입니다.
    float4 color : COLOR; // 정점에서 보간된 색상 값.
};

// =================================================================================
// 정점 셰이더 (Vertex Shader)
// =================================================================================
PSInput VSMain(uint vertexID : SV_VertexID)
{
    PSInput result;

    // 정점 버퍼 없이, 정점의 ID(0, 1, 2)를 기반으로 직접 삼각형의 좌표를 만듭니다.
    // 화면 중앙에 간단한 삼각형을 정의합니다. (좌표는 -1.0 ~ +1.0 사이)
    if (vertexID == 0)
        result.position = float4(0.0f, 0.5f, 0.5f, 1.0f);
    else if (vertexID == 1)
        result.position = float4(0.5f, -0.5f, 0.5f, 1.0f);
    else
        result.position = float4(-0.5f, -0.5f, 0.5f, 1.0f);

    // 각 정점에 다른 색상을 지정합니다 (빨강, 초록, 파랑).
    // 이 색상들은 삼각형 내부를 칠할 때 자동으로 보간(interpolate)됩니다.
    if (vertexID == 0)
        result.color = float4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    else if (vertexID == 1)
        result.color = float4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    else
        result.color = float4(0.0f, 0.0f, 1.0f, 1.0f); // Blue
    
    return result;
}

