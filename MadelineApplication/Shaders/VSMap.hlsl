
struct RASTER_OUT
{
    float4 posHomog : SV_Position;
    float3 posWorld : WORLD;
    float2 uv : UV;
    float3 normWorld : NORM;
};

struct VERT_IN
{
    float3 _pos : POSITION;
    float3 _uv : UV;
    float3 _norm : NORM;
};

RASTER_OUT main(VERT_IN inputVertex)
{
    RASTER_OUT vertOut;
    
    vertOut.posHomog = float4(inputVertex._pos, 1);
    vertOut.uv = inputVertex._uv.xy;
    
    return vertOut;
}