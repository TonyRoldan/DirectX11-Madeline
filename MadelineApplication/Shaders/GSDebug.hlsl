static const matrix identity =
{
    float4(1, 0, 0, 0),
    float4(0, 1, 0, 0),
    float4(0, 0, 1, 0),
    float4(0, 0, 0, 1)
};

struct BOUNDING_BOX
{
    float4 min;
    float4 max;
};

struct VERT_IN
{
    float4 ignored : SV_POSITION;
    uint instance : COLLIDER_INDEX;
};

struct VERT_OUT
{
    float4 hpos : SV_POSITION;
    float4 color : COLOR;
};

cbuffer INSTANCE_DATA : register(b1)
{
    uint transformStart;
    uint materialStart;
    uint colliderStart;
    uint colliderEnd;
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

StructuredBuffer<BOUNDING_BOX> instanceColliders : register(t0);

[maxvertexcount(30)]
void main(point VERT_IN input[1], inout LineStream<VERT_OUT> output)
{
    BOUNDING_BOX box = instanceColliders[input[0].instance];
	
    float4 minc = box.min;
    float4 maxc = box.max;
    minc.w = 1.0f;
    maxc.w = 1.0f;
    
    float4 center = {0.0f,0.0f,0.0f,1.0f};
    center.xyz = lerp(box.min.xyz, box.max.xyz, 0.5f);
    
    matrix world = identity;
    world[3] = center;
	
    VERT_OUT vertex[8] =
    {
        { float4(minc.x, maxc.y, minc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(maxc.x, maxc.y, minc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(maxc.x, minc.y, minc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(minc.x, minc.y, minc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(minc.x, maxc.y, maxc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(maxc.x, maxc.y, maxc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(maxc.x, minc.y, maxc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) },
        { float4(minc.x, minc.y, maxc.z, 1), float4(1.0f, 0.0f, 0.0f, 1.0f) }
    };
    
    for (uint i = 0; i < 8; ++i)
    {
        vertex[i].hpos *= 1.1f;
        vertex[i].hpos = mul(view, vertex[i].hpos);
        vertex[i].hpos = mul(projection, vertex[i].hpos);
    }
    
    unsigned int indicies[24] =
    {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 7, 3, 2, 6, 5, 1
    };
    
    for (uint j = 0; j < 24; j += 2)
    {
        output.Append(vertex[indicies[j]]);
        output.Append(vertex[indicies[j + 1]]);
        output.RestartStrip();
    }
    
    //***To render XYZ Axis uncomment this***

    //float4 red = { 1.0f, 0.0f, 0.0f, 1.0f };
    //VERT_OUT axis[6] =
    //{
    //    { world[3], red },
    //    { world[3] + float4(normalize(world[0].xyz), 0), red },
    //    { world[3], red },
    //    { world[3] + float4(normalize(world[1].xyz), 0), red },
    //    { world[3], red },
    //    { world[3] + float4(normalize(world[2].xyz), 0), red }
    //};
    
    //for (uint k = 0; k < 6; ++k)
    //{
    //    //axis[k].hpos = mul(world, axis[k].hpos);
    //    axis[k].hpos = mul(view, axis[k].hpos);
    //    axis[k].hpos = mul(projection, axis[k].hpos);
    //}
    
    //for (uint l = 0; l < 6; l += 2)
    //{
    //    output.Append(axis[l]);
    //    output.Append(axis[l + 1]);
    //    output.RestartStrip();
    //}
}