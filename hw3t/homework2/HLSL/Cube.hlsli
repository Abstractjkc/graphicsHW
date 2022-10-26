
cbuffer ConstantBuffer : register(b0)
{
    matrix g_World; // matrix������float4x4���������row_major������£�����Ĭ��Ϊ��������
    matrix g_View;  // ������ǰ�����row_major��ʾ��������
    matrix g_Proj;  // �ý̳�����ʹ��Ĭ�ϵ��������󣬵���Ҫ��C++�����Ԥ�Ƚ��������ת�á�
}


struct VertexIn
{
	float3 posL : POSITION;
	float4 color : COLOR;
};

struct VertexOut
{
	float4 posH : SV_POSITION;
	float4 color : COLOR;
};
