#include "Cube.hlsli"

VertexOut VS(VertexIn vIn)
{
    VertexOut vOut;
    vOut.posH = mul(float4(vIn.posL, 1.0f), g_World);  // mul ���Ǿ���˷�, �����*Ҫ���������Ϊ
    vOut.posH = mul(vOut.posH, g_View);               // ��������ȵ��������󣬽��Ϊ
    vOut.posH = mul(vOut.posH, g_Proj);               // Cij = Aij * Bij
    vOut.color = vIn.color;                         // ����alphaͨ����ֵĬ��Ϊ1.0
    return vOut;
}