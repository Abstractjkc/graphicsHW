#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

const D3D11_INPUT_ELEMENT_DESC GameApp::VertexPosColor::inputLayout[2] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance), m_CBuffer()
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	if (!InitEffect())
		return false;

	if (!InitResource())
		return false;

	return true;
}

void GameApp::OnResize()
{
	D3DApp::OnResize();
}

void GameApp::UpdateScene(float dt)
{
	
	angle += 0.5f * dt;
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	static float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };	// RGBA = (0,0,0,255)
	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	//==========================添加代码-1-开始=======================

	const int N = 42;	// 正方形边长
	float center = 1.0f * N / 2 + 0.5f;		//计算中心位置
	auto f = [center](int x, int y) ->float {		//计算与中心的曼哈顿距离
		return abs(x - center)+abs(y-center);
	};
	for (int i = 1; i <= N; i++)
	{
		for (int j = 1; j <= N; j++)
		{
			float A = 5 / (0.1f * (f(i, j)) + 1);	//根据位置计算振幅
			float scale = (sinf(angle * 3.0f - f(i, j)) + 1.3f)*0.35f * 11.0f / N; //计算缩放比例
			DirectX::XMMATRIX mScale = XMMatrixScaling(scale, scale, scale);
			DirectX::XMMATRIX mRotate = XMMatrixRotationX(angle) * XMMatrixRotationY(angle);
			//mTranslate 确定各个字符的位置 y方向加 sin 函数形成振荡效果
			DirectX::XMMATRIX mTranslate = XMMatrixTranslation((i - center) * 4.5f * 11.0f/N, A * (sinf(angle * 3.0f - f(i, j)) + 1.3f), (j - center) * 4.5f * 11.0f / N);
			m_CBuffer.world = XMMatrixTranspose(mScale * mRotate * mTranslate); 
			// 更新常量缓冲区，让立方体转起来
			D3D11_MAPPED_SUBRESOURCE mappedData;
			HR(m_pd3dImmediateContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
			memcpy_s(mappedData.pData, sizeof(m_CBuffer), &m_CBuffer, sizeof(m_CBuffer));
			m_pd3dImmediateContext->Unmap(m_pConstantBuffer.Get(), 0);
			m_pd3dImmediateContext->DrawIndexed(12 * 9 * 3, 0, 0);	//绘制字


			// 绘制叶子
			srand(i * j + 1);
			DirectX::XMMATRIX mScaleChild = XMMatrixScaling(0.3f, 0.3f, 0.3f);
			DirectX::XMMATRIX mTranslateChild = XMMatrixTranslation(2.5f, 0, 0);
			for (int k = 1; k <= 1 + rand() % 3; k++)
			{
				DirectX::XMMATRIX mRotateChild = XMMatrixRotationX(rand() + angle) * XMMatrixRotationY(rand() + angle) * XMMatrixRotationZ(rand() + angle);
				m_CBuffer.world = XMMatrixTranspose(mScaleChild * mTranslateChild * mScale * mRotateChild * mTranslate);

				D3D11_MAPPED_SUBRESOURCE mappedData;
				HR(m_pd3dImmediateContext->Map(m_pConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));
				memcpy_s(mappedData.pData, sizeof(m_CBuffer), &m_CBuffer, sizeof(m_CBuffer));
				m_pd3dImmediateContext->Unmap(m_pConstantBuffer.Get(), 0);
				m_pd3dImmediateContext->DrawIndexed(12 * 9 * 3, 0, 0);	//绘制字
			}
		}
	}

	//==========================添加代码-1-结束=======================

	HR(m_pSwapChain->Present(0, 0));
}


bool GameApp::InitEffect()
{
	ComPtr<ID3DBlob> blob;

	// 创建顶点着色器
	HR(CreateShaderFromFile(L"HLSL\\Cube_VS.cso", L"HLSL\\Cube_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));
	// 创建顶点布局
	HR(m_pd3dDevice->CreateInputLayout(VertexPosColor::inputLayout, ARRAYSIZE(VertexPosColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), m_pVertexLayout.GetAddressOf()));

	// 创建像素着色器
	HR(CreateShaderFromFile(L"HLSL\\Cube_PS.cso", L"HLSL\\Cube_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(m_pd3dDevice->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));

	return true;
}

bool GameApp::InitResource()
{
	// ******************
	// 由py脚本读取obj文件中的顶点数据
	VertexPosColor vertices[] =
	{
		{ XMFLOAT3(-0.99875265f,-0.041459948f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-2.0400277f,-0.74491195f,-0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(-2.1421185f,-0.51064325f,-0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(-0.89666195f,-0.275729f,-0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(-2.1421185f,-0.51064325f,0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(-0.89666195f,-0.275729f,0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(-0.99875265f,-0.041459948f,0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-2.0400277f,-0.74491195f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(-0.83641885f,0.38688671f,-0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(-2.0494972000000002f,0.7476368499999999f,-0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(-1.9225763000000002f,0.9701841999999999f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(-0.96334035f,0.16433947000000002f,-0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(-1.9225763000000002f,0.9701841999999999f,0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-0.96334035f,0.16433947000000002f,0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(-0.83641885f,0.38688671f,0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(-2.0494972000000002f,0.7476368499999999f,0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(0.92785505f,1.3540408000000002f,-0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(-0.6107911f,1.1296312f,-0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(-0.6107911f,1.3540408000000002f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(0.92785505f,1.1296312f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-0.6107911f,1.3540408000000002f,0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(0.92785505f,1.1296312f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(0.92785505f,1.3540408000000002f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(-0.6107911f,1.1296312f,0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(0.061108155f,1.1296309f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-0.12657324f,-0.35889577f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(-0.12657324f,1.1296309f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(0.061108155f,-0.35889577f,-0.5f), XMFLOAT4(0, 0, 1, 1.0f) },
{ XMFLOAT3(-0.12657324f,1.1296309f,0.5f), XMFLOAT4(0, 0, 1, 1.0f) },
{ XMFLOAT3(0.061108155f,-0.35889577f,0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(0.061108155f,1.1296309f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(-0.12657324f,-0.35889577f,0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-0.336633475f,-0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(1.2530559499999998f,-1.20423345f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(1.2530559499999998f,-0.336633475f,-0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-1.20423345f,-0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(1.2530559499999998f,-0.336633475f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-1.20423345f,0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-0.336633475f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(1.2530559499999998f,-1.20423345f,0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(1.31358905f,-1.0657139f,-0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(0.8018203f,-1.05829335f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(0.84446355f,-0.9154765500000001f,-0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(1.2709456499999998f,-1.2085309499999999f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(0.84446355f,-0.9154765500000001f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(1.2709456499999998f,-1.2085309499999999f,0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(1.31358905f,-1.0657139f,0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(0.8018203f,-1.05829335f,0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(0.9206915f,1.1296309f,-0.5f), XMFLOAT4(1, 0, 0, 1.0f) },
{ XMFLOAT3(0.65350085f,-0.11964272499999999f,-0.5f), XMFLOAT4(0, 1, 1, 1.0f) },
{ XMFLOAT3(0.65350085f,1.1296309f,-0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(0.9206915f,-0.11964272499999999f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(0.65350085f,1.1296309f,0.5f), XMFLOAT4(0, 0, 1, 1.0f) },
{ XMFLOAT3(0.9206915f,-0.11964272499999999f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(0.9206915f,1.1296309f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(0.65350085f,-0.11964272499999999f,0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(1.2530558f,-0.6656544f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-0.8848058999999999f,-0.7854574999999999f,-0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(-0.8848058999999999f,-0.6656544f,-0.5f), XMFLOAT4(1, 0, 1, 1.0f) },
{ XMFLOAT3(1.2530558f,-0.7854574999999999f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(-0.8848058999999999f,-0.6656544f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(1.2530558f,-0.7854574999999999f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(1.2530558f,-0.6656544f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(-0.8848058999999999f,-0.7854574999999999f,0.5f), XMFLOAT4(0, 0, 0, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-0.11964068f,-0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(0.061108155f,-0.34405086f,-0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(0.061108155f,-0.11964068f,-0.5f), XMFLOAT4(1, 1, 0, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-0.34405086f,-0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(0.061108155f,-0.11964068f,0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-0.34405086f,0.5f), XMFLOAT4(0, 1, 0, 1.0f) },
{ XMFLOAT3(1.4697421999999998f,-0.11964068f,0.5f), XMFLOAT4(1, 1, 1, 1.0f) },
{ XMFLOAT3(0.061108155f,-0.34405086f,0.5f), XMFLOAT4(1, 1, 0, 1.0f) },

	};
	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof vertices;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.GetAddressOf()));

	// ******************
	// 索引数组
	// 由py脚本读取obj文件中的数据
	WORD indices[] = {
		0,1,2,
0,3,1,
4,5,6,
4,7,5,
6,3,0,
6,5,3,
2,7,4,
2,1,7,
2,6,0,
2,4,6,
7,3,5,
7,1,3,
8,9,10,
8,11,9,
12,13,14,
12,15,13,
14,11,8,
14,13,11,
10,15,12,
10,9,15,
10,14,8,
10,12,14,
15,11,13,
15,9,11,
16,17,18,
16,19,17,
20,21,22,
20,23,21,
22,19,16,
22,21,19,
18,23,20,
18,17,23,
18,22,16,
18,20,22,
23,19,21,
23,17,19,
24,25,26,
24,27,25,
28,29,30,
28,31,29,
30,27,24,
30,29,27,
26,31,28,
26,25,31,
26,30,24,
26,28,30,
31,27,29,
31,25,27,
32,33,34,
32,35,33,
36,37,38,
36,39,37,
38,35,32,
38,37,35,
34,39,36,
34,33,39,
34,38,32,
34,36,38,
39,35,37,
39,33,35,
40,41,42,
40,43,41,
44,45,46,
44,47,45,
46,43,40,
46,45,43,
42,47,44,
42,41,47,
42,46,40,
42,44,46,
47,43,45,
47,41,43,
48,49,50,
48,51,49,
52,53,54,
52,55,53,
54,51,48,
54,53,51,
50,55,52,
50,49,55,
50,54,48,
50,52,54,
55,51,53,
55,49,51,
56,57,58,
56,59,57,
60,61,62,
60,63,61,
62,59,56,
62,61,59,
58,63,60,
58,57,63,
58,62,56,
58,60,62,
63,59,61,
63,57,59,
64,65,66,
64,67,65,
68,69,70,
68,71,69,
70,67,64,
70,69,67,
66,71,68,
66,65,71,
66,70,64,
66,68,70,
71,67,69,
71,65,67,
	};
	// 设置索引缓冲区描述
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof indices;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = indices;
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);


	// ******************
	// 设置常量缓冲区描述
	//
	D3D11_BUFFER_DESC cbd;
	ZeroMemory(&cbd, sizeof(cbd));
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(ConstantBuffer);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// 新建常量缓冲区，不使用初始数据
	HR(m_pd3dDevice->CreateBuffer(&cbd, nullptr, m_pConstantBuffer.GetAddressOf()));

	// 初始化常量缓冲区的值
	m_CBuffer.world = XMMatrixIdentity();	// 单位矩阵的转置是它本身
	m_CBuffer.view = XMMatrixTranspose(XMMatrixLookAtLH(
		XMVectorSet(0.0f, 22.0f, -35.0f, 0.0f),
		XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)
	));
	m_CBuffer.proj = XMMatrixTranspose(XMMatrixPerspectiveFovLH(XM_PIDIV2, AspectRatio(), 1.0f, 1000.0f));


	// ******************
	// 给渲染管线各个阶段绑定好所需资源
	//

	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosColor);	// 跨越字节数
	UINT offset = 0;						// 起始偏移量

	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	// 设置图元类型，设定输入布局
	m_pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pd3dImmediateContext->IASetInputLayout(m_pVertexLayout.Get());
	// 将着色器绑定到渲染管线
	m_pd3dImmediateContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	// 将更新好的常量缓冲区绑定到顶点着色器
	m_pd3dImmediateContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	m_pd3dImmediateContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	// ******************
	// 设置调试对象名
	//
	D3D11SetDebugObjectName(m_pVertexLayout.Get(), "VertexPosColorLayout");
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "VertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "IndexBuffer");
	D3D11SetDebugObjectName(m_pConstantBuffer.Get(), "ConstantBuffer");
	D3D11SetDebugObjectName(m_pVertexShader.Get(), "Cube_VS");
	D3D11SetDebugObjectName(m_pPixelShader.Get(), "Cube_PS");

	return true;
}
