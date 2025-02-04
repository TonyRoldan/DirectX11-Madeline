struct OUTPUT
{
    float4 ignored : SV_POSITION;
    uint instance : COLLIDER_INDEX;
};

OUTPUT main(uint instance : SV_InstanceID)
{
    OUTPUT output = { { 0, 0, 0, 0 }, instance };
    return output;
}