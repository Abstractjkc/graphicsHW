#include "Basic.hlsli"

float4 PS(VertexPosHWNormalColor pIn) : SV_Target
{
    // 标准化法向量
    pIn.NormalW = normalize(pIn.NormalW);

    // 顶点指向眼睛的向量
    float3 toEyeW = normalize(g_EyePosW - pIn.PosW);

    // 初始化为0 
    float4 ambient, diffuse, spec;
    float4 A, D, S;
    ambient = diffuse = spec = A = D = S = float4(0.0f, 0.0f, 0.0f, 0.0f);

    DirectionalLight dirLight = g_DirLight[0];
    [flatten]
    if (g_IsReflection)
    {
        dirLight.Direction = mul(dirLight.Direction, (float3x3) (g_Reflection));
    }
    ComputeDirectionalLight(g_Material, dirLight, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;


    PointLight pointLight = g_PointLight[0];
    [flatten]
    if (g_IsReflection)
    {
        pointLight.Position = (float3) mul(float4(pointLight.Position, 1.0f), g_Reflection);
    }
    ComputePointLight(g_Material, pointLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    SpotLight spotLight = g_SpotLight[0];
    [flatten]
    if (g_IsReflection)
    {
        spotLight.Position = (float3) mul(float4(spotLight.Position, 1.0f), g_Reflection);
        spotLight.Direction = mul(spotLight.Direction, (float3x3) g_Reflection);
    }
    ComputeSpotLight(g_Material, spotLight, pIn.PosW, pIn.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    float4 litColor = pIn.Color * (ambient + diffuse) + spec;

    litColor.a = g_Material.Diffuse.a * pIn.Color.a;

    return litColor;
}
