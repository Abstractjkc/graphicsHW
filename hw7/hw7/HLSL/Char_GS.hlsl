#include "Basic.hlsli"

[maxvertexcount(6)]
void GS(triangle VertexPosHWNormalColor input[3], inout TriangleStream<VertexPosHWNormalColor> output)
{
	int i;
	[unroll]
	for (i = 0; i < 3; ++i)
	{
		output.Append(input[i]);
	}
	output.RestartStrip();
	VertexPosHWNormalColor vertexes[3];
	float symmetryPlaneX = 30.0f;
	matrix viewProj = mul(g_View, g_Proj);
    [unroll]
	for (i = 0; i < 3; ++i)
	{
		vertexes[i].PosW = float3(2 * symmetryPlaneX - input[i].PosW.x, input[i].PosW.y, input[i].PosW.z);
		vertexes[i].PosH = mul(float4(vertexes[i].PosW, 1.0f), viewProj);
		vertexes[i].NormalW = -input[i].NormalW;
		vertexes[i].Color = float4(0.0f,1.0f,1.0f,1.0f);
		output.Append(vertexes[i]);
	}
	output.RestartStrip();
}