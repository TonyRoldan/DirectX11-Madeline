#pragma pack_matrix(row_major) 

// ---------- Structs ----------
struct RASTER_OUT
{
    float4 posHomog : SV_Position;
    float3 posWorld : WORLD;
    float2 uv       : UV;
    float3 normWorld : NORM; 
    float4 _weight : WEIGHTS;
    float4 _joints : JOINTS;
};

struct ATTRIBUTE
{
    float3 diffReflect;
    float dissolve;
    float3 specReflect;
    float spec;
    float3 ambReflect;
    float sharpness;
    float3 transFilter;
    float optDens;
    float3 emmReflect;
    uint illum;  
};

struct VERT_IN
{
    float3 _pos : POSITION;
    float2 _uv : UV;
    float3 _norm : NORM;
    float4 _weight : WEIGHTS;
    float4 _joints : JOINTS;
};

// ---------- Buffers ----------

StructuredBuffer<float4x4> bones : register(t1);
StructuredBuffer<float4x4> instanceTransforms : register(t0);


cbuffer INSTANCE_DATA : register(b2)
{
    uint transformStart;
    uint materialStart;
    uint colliderStart;
    uint colliderEnd;
};

cbuffer SCENE_DATA : register(b1)
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

cbuffer MESH_DATA : register(b0)
{
    float4x4 world;
    ATTRIBUTE material; 
    uint hasTexture;
};


RASTER_OUT main(VERT_IN inputVertex, uint id : SV_InstanceID)
{   
    float4x4 boneTransform = (float4x4)0;
    boneTransform += (mul(bones[(int) inputVertex._joints.x], inputVertex._weight.x));
    boneTransform += (mul(bones[(int) inputVertex._joints.y], inputVertex._weight.y));
    boneTransform += (mul(bones[(int) inputVertex._joints.z], inputVertex._weight.z));
    boneTransform += (mul(bones[(int) inputVertex._joints.w], inputVertex._weight.w));
   
    //boneTransform += bones[(int) inputVertex._joints.x] * inputVertex._weight.x;
    //boneTransform += bones[(int) inputVertex._joints.y] * inputVertex._weight.y;
    //boneTransform += bones[(int) inputVertex._joints.z] * inputVertex._weight.z;
    //boneTransform += bones[(int) inputVertex._joints.w] * inputVertex._weight.w;
    
    
    RASTER_OUT output = (RASTER_OUT) 0;
    float4x4 currTransform = transpose(instanceTransforms[transformStart + id]);
    output.posHomog = float4(inputVertex._pos, 1); 
    output.posHomog = mul(output.posHomog, boneTransform);
    output.posHomog = mul(output.posHomog, currTransform);
    output.posWorld = output.posHomog;
    output.posHomog = mul(output.posHomog, view);
    output.posHomog = mul(output.posHomog, projection);
    
    output.uv = inputVertex._uv;
    
    output.normWorld = inputVertex._norm;   
    
    output.normWorld = mul(float4(output.normWorld, 0), boneTransform).xyz;
    output.normWorld = mul(float4(output.normWorld, 0), currTransform).xyz;
    output.normWorld = normalize(output.normWorld);
    
    output._weight = inputVertex._weight;
    output._joints = inputVertex._joints;
       
    return output;
}




