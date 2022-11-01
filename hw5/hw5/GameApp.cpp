#include "GameApp.h"
#include "d3dUtil.h"
#include "DXTrace.h"
using namespace DirectX;

GameApp::GameApp(HINSTANCE hInstance)
	: D3DApp(hInstance),
	m_VertexCount()
{
}

GameApp::~GameApp()
{
}

bool GameApp::Init()
{
	if (!D3DApp::Init())
		return false;

	// 务必先初始化所有渲染状态，以供下面的特效使用
	RenderStates::InitAll(m_pd3dDevice.Get());

	if (!m_BasicEffect.InitAll(m_pd3dDevice.Get()))
		return false;

	if (!InitResource())
		return false;

	// 初始化鼠标，键盘不需要
	m_pMouse->SetWindow(m_hMainWnd);
	m_pMouse->SetMode(DirectX::Mouse::MODE_ABSOLUTE);

	return true;
}

void GameApp::OnResize()
{
	assert(m_pd2dFactory);
	assert(m_pdwriteFactory);

	D3DApp::OnResize();

	
	// 更新投影矩阵
	m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));
	
}

void GameApp::UpdateScene(float dt)
{

	// 更新鼠标事件，获取相对偏移量
	Mouse::State mouseState = m_pMouse->GetState();
	Mouse::State lastMouseState = m_MouseTracker.GetLastState();
	m_MouseTracker.Update(mouseState);

	Keyboard::State keyState = m_pKeyboard->GetState();
	m_KeyboardTracker.Update(keyState);

	// 根据键盘更新观察位置
	
	XMFLOAT3 pos = { 0, 0, 0 };
	float moveSpeed = 40.0f;
	if (keyState.IsKeyDown(Keyboard::LeftShift)) moveSpeed *= 2.0f;
	if (keyState.IsKeyDown(Keyboard::W)) pos.z += moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::S)) pos.z -= moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::A)) pos.x -= moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::D)) pos.x += moveSpeed * dt;
	if (keyState.IsKeyDown(Keyboard::Q)) viewRot.z += 0.5f * dt;
	if (keyState.IsKeyDown(Keyboard::E)) viewRot.z -= 0.5f * dt;

	if (mouseState.leftButton == true && m_MouseTracker.leftButton == m_MouseTracker.HELD)
	{
		viewRot.y += (mouseState.x - lastMouseState.x) * 0.3f * dt;
		viewRot.x += (mouseState.y - lastMouseState.y) * 0.3f * dt;
	}

	if (viewRot.x > 1.5f) viewRot.x = 1.5f;
	else if (viewRot.x < -1.5f) viewRot.x = -1.5f;
	if (viewRot.z > 0.7f) viewRot.z = 0.7f;
	else if (viewRot.z < -0.7f) viewRot.z = -0.7f;

	XMStoreFloat3(&pos, XMVector3Transform(XMLoadFloat3(&pos), XMMatrixRotationX(viewRot.x) * XMMatrixRotationZ(viewRot.z) * XMMatrixRotationY(viewRot.y)));
	viewPos.x += pos.x;
	viewPos.y += pos.y;
	viewPos.z += pos.z;
	m_BasicEffect.SetViewMatrix(XMMatrixTranslation(-viewPos.x, -viewPos.y, -viewPos.z) * XMMatrixRotationY(-viewRot.y) * XMMatrixRotationZ(-viewRot.z) * XMMatrixRotationX(-viewRot.x));
	m_BasicEffect.SetEyePos(XMVectorSet(viewPos.x, viewPos.y, viewPos.z, 0.0f));
	
	// 切换灯光模式
	
	if (m_KeyboardTracker.IsKeyPressed(Keyboard::D1))
	{
		m_LightMode = LightMode::DirectionalLight;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D2))
	{
		m_LightMode = LightMode::PointLight;
	}
	else if (m_KeyboardTracker.IsKeyPressed(Keyboard::D3))
	{
		m_LightMode = LightMode::SpotLight;
	}

	// 根据灯光模式设置光源
	
	if (m_LightMode == LightMode::DirectionalLight) {
		m_BasicEffect.SetDirLight(0, dirLight);
		m_BasicEffect.SetPointLight(0, PointLight());
		m_BasicEffect.SetSpotLight(0, SpotLight());
	}
	else if (m_LightMode == LightMode::PointLight)
	{
		float upperBound = 10.0f;
		float pointLightPositionx = upperBound * sinf(angle * 10.0f);
		float pointLightPositionz = upperBound * sinf(angle * 6.0f);		//点光源在平面内运动
		pointLight.position = XMFLOAT3(pointLightPositionx, 2.0f, pointLightPositionz);
		m_BasicEffect.SetDirLight(0, DirectionalLight());
		m_BasicEffect.SetPointLight(0, pointLight);
		m_BasicEffect.SetSpotLight(0, SpotLight());
	}
	else
	{
		spotLight.position = XMFLOAT3(viewPos.x, viewPos.y, viewPos.z);	//光源跟随相机位置
		XMFLOAT3 spotLightDirection(0, 0, 1.0f);
		XMStoreFloat3(&spotLightDirection, XMVector3Transform(XMLoadFloat3(&spotLightDirection), XMMatrixRotationX(viewRot.x) * XMMatrixRotationZ(viewRot.z) * XMMatrixRotationY(viewRot.y)));
		spotLight.direction = spotLightDirection;
		m_BasicEffect.SetDirLight(0, DirectionalLight());
		m_BasicEffect.SetPointLight(0, PointLight());
		m_BasicEffect.SetSpotLight(0, spotLight);
	}

	// 更新 angle 形成动画
	angle += 0.5f * dt;
}

void GameApp::DrawScene()
{
	assert(m_pd3dImmediateContext);
	assert(m_pSwapChain);

	m_pd3dImmediateContext->ClearRenderTargetView(m_pRenderTargetView.Get(), reinterpret_cast<const float*>(&Colors::Black));
	m_pd3dImmediateContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// 文字森林效果--------开始
	const int N = 30;	// 正方形边长
	float center = 1.0f * N / 2 + 0.5f;		//计算中心位置
	auto f = [center](int x, int y) ->float {		//计算与中心的曼哈顿距离
		return abs(x - center) + abs(y - center);
	};
	for (int i = 1; i <= N; i++)
	{
		for (int j = 1; j <= N; j++)
		{
			float A = 5 / (0.1f * (f(i, j)) + 1);	//根据位置计算振幅
			float scale = (sinf(angle * 3.0f - f(i, j)) + 1.3f) * 0.35f * 11.0f / N; //计算缩放比例
			DirectX::XMMATRIX mScale = XMMatrixScaling(scale, scale, scale);
			DirectX::XMMATRIX mRotate = XMMatrixRotationX(angle) * XMMatrixRotationY(angle);
			//mTranslate 确定各个字符的位置 y方向加 sin 函数形成振荡效果
			DirectX::XMMATRIX mTranslate = XMMatrixTranslation((i - center) * 4.5f * 11.0f / N, A * (sinf(angle * 3.0f - f(i, j)) + 1.3f), (j - center) * 4.5f * 11.0f / N);
			m_BasicEffect.SetWorldMatrix(mScale * mRotate * mTranslate);
			// 绘制字符
			m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
			m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);

			// 绘制叶子
			srand(i * j + 1);
			DirectX::XMMATRIX mScaleChild = XMMatrixScaling(0.3f, 0.3f, 0.3f);
			DirectX::XMMATRIX mTranslateChild = XMMatrixTranslation(2.5f, 0, 0);
			for (int k = 1; k <= 1 + rand() % 3; k++)
			{
				DirectX::XMMATRIX mRotateChild = XMMatrixRotationX(rand() + angle) * XMMatrixRotationY(rand() + angle) * XMMatrixRotationZ(rand() + angle);
				m_BasicEffect.SetWorldMatrix(mScaleChild * mTranslateChild * mScale * mRotateChild * mTranslate);
				m_BasicEffect.Apply(m_pd3dImmediateContext.Get());
				m_pd3dImmediateContext->DrawIndexed(m_IndexCount, 0, 0);
			}
		}
	}
	//文字森林效果----结束
	
	HR(m_pSwapChain->Present(0, 0));
}



bool GameApp::InitResource()
{
	

	// 载入字符模型

	auto meshData = Geometry::CreateChar<VertexPosNormalColor>();
	ResetMesh(meshData);

	// 初始化灯光类型
	
	m_LightMode = LightMode::DirectionalLight;
	
	// 设置初始位置
	
	viewPos.z -= 50.0f;
	viewPos.y += 20.0f;
	
	// 方向光
	
	dirLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	dirLight.diffuse = XMFLOAT4(0.8f, 0.8f, 0.1f, 1.0f);
	dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	dirLight.direction = XMFLOAT3(-0.577f, -0.577f, 0.577f);
	m_BasicEffect.SetDirLight(0, dirLight);
	
	// 点光
	
	pointLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	pointLight.diffuse = XMFLOAT4(0.12f, 0.94f, 0.09f, 1.0f);
	pointLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	pointLight.position = XMFLOAT3(0.0f, 1.0f, 0.0f);
	pointLight.att = XMFLOAT3(0.0f, 0.05f, 0.0f);
	pointLight.range = 25.0f;
	
	//聚光灯
	
	spotLight.ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	spotLight.diffuse = XMFLOAT4(0.1f, 0.9f, 1.9f, 1.0f);
	spotLight.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	spotLight.position = XMFLOAT3(viewPos.x, viewPos.y, viewPos.z);
	spotLight.direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	spotLight.spot = 12.0f;
	spotLight.att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	spotLight.range = 10000.0f;
	
	// 材质
	
	Material material{};
	material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	m_BasicEffect.SetMaterial(material);
	// 摄像机位置
	m_BasicEffect.SetEyePos(XMVectorSet(viewPos.x, viewPos.y, viewPos.z, 1.0f));
	// 矩阵
	m_BasicEffect.SetWorldMatrix(XMMatrixIdentity());
	m_BasicEffect.SetViewMatrix(XMMatrixTranslation(-viewPos.x, -viewPos.y, -viewPos.z) * XMMatrixRotationY(-viewRot.y) * XMMatrixRotationZ(-viewRot.z) * XMMatrixRotationX(-viewRot.x));
	m_BasicEffect.SetProjMatrix(XMMatrixPerspectiveFovLH(XM_PI / 3, AspectRatio(), 1.0f, 1000.0f));




	// 输入装配阶段的顶点缓冲区设置
	UINT stride = sizeof(VertexPosNormalColor);		// 跨越字节数
	UINT offset = 0;							// 起始偏移量
	m_pd3dImmediateContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	// 设置默认渲染状态
	m_BasicEffect.SetRenderChar(m_pd3dImmediateContext.Get());


	return true;
}



void GameApp::ResetMesh(const Geometry::MeshData<VertexPosNormalColor>& meshData)
{
	m_pVertexBuffer.Reset();
	m_pIndexBuffer.Reset();
	// 设置顶点缓冲区描述
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = (UINT)meshData.vertexVec.size() * sizeof(VertexPosNormalColor);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	// 新建顶点缓冲区
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = meshData.vertexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&vbd, &InitData, m_pVertexBuffer.ReleaseAndGetAddressOf()));
	m_VertexCount = (UINT)meshData.vertexVec.size();

	m_IndexCount = (UINT)meshData.indexVec.size();
	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = m_IndexCount * sizeof(WORD);
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	// 新建索引缓冲区
	InitData.pSysMem = meshData.indexVec.data();
	HR(m_pd3dDevice->CreateBuffer(&ibd, &InitData, m_pIndexBuffer.GetAddressOf()));
	// 输入装配阶段的索引缓冲区设置
	m_pd3dImmediateContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	// 设置调试对象名
	D3D11SetDebugObjectName(m_pVertexBuffer.Get(), "CharVertexBuffer");
	D3D11SetDebugObjectName(m_pIndexBuffer.Get(), "CharIndexBuffer");
}
