struct VSIn
{
    float3 position : POS;
    float4 color : COLOR;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

cbuffer SCENE_DATA : register(b0)
{
    float4x4 view;
    float4x4 projection;
    float4 camPos;
    float4 dirLightDir;
    float4 dirLightColor;
    float4 ambientTerm;
    float4 fogColor;
    float fogDensity;
    float fogStartDistance;
    float contrast;
    float saturation;
};

VSOut main(VSIn input)
{
    VSOut output;
    output.position = float4(input.position, 1.0f);
    //output.position = mul(output.position, view);
    //output.position = mul(output.position, projection);
    output.position.w = 1.0f;
    output.color = input.color;
    return output;
}
