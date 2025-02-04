struct PIN
{
    float4 hpos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PIN input) : SV_TARGET
{
    return input.color;
}