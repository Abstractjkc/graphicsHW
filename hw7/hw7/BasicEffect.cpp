#include "Effects.h"
#include "d3dUtil.h"
#include "EffectHelper.h"	// 必须晚于Effects.h和d3dUtil.h包含
#include "DXTrace.h"
#include "Vertex.h"
using namespace DirectX;




//
// BasicEffect::Impl 需要先于BasicEffect的定义
//

class BasicEffect::Impl : public AlignedType<BasicEffect::Impl>
{
public:

	//
	// 这些结构体对应HLSL的结构体。需要按16字节对齐
	//

	struct CBChangesEveryDrawing
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX worldInvTranspose;
		Material material;
	};

	struct CBDrawingStates
	{
		int isReflection;
		DirectX::XMFLOAT3 pad;
	};

	struct CBChangesEveryFrame
	{
		DirectX::XMMATRIX view;
		DirectX::XMFLOAT3 eyePos;

	};

	struct CBChangesOnResize
	{
		DirectX::XMMATRIX proj;
	};


	struct CBChangesRarely
	{
		DirectX::XMMATRIX reflection;
		DirectionalLight dirLight[10];
		PointLight pointLight[10];
		SpotLight spotLight[10];
		int numDirLight;
		int numPointLight;
		int numSpotLight;
		float pad;		// 打包保证16字节对齐
	};

public:
	// 必须显式指定
	Impl() : m_IsDirty() {}
	~Impl() = default;

public:
	// 需要16字节对齐的优先放在前面
	CBufferObject<0, CBChangesEveryDrawing> m_CBDrawing;
	CBufferObject<1, CBDrawingStates>		m_CBStates;
	CBufferObject<2, CBChangesEveryFrame>   m_CBFrame;		    // 每次对象绘制的常量缓冲区
	CBufferObject<3, CBChangesOnResize>     m_CBOnResize;		// 每次窗口大小变更的常量缓冲区
	CBufferObject<4, CBChangesRarely>		m_CBRarely;		    // 几乎不会变更的常量缓冲区
	BOOL m_IsDirty;											    // 是否有值变更
	std::vector<CBufferBase*> m_pCBuffers;					    // 统一管理上面所有的常量缓冲区


	//ComPtr<ID3D11VertexShader> m_pTriangleVS;
	//ComPtr<ID3D11PixelShader> m_pTrianglePS;
	//ComPtr<ID3D11GeometryShader> m_pTriangleGS;

	//ComPtr<ID3D11VertexShader> m_pCylinderVS;
	//ComPtr<ID3D11PixelShader> m_pCylinderPS;
	//ComPtr<ID3D11GeometryShader> m_pCylinderGS;

	//ComPtr<ID3D11VertexShader> m_pNormalVS;
	//ComPtr<ID3D11PixelShader> m_pNormalPS;
	//ComPtr<ID3D11GeometryShader> m_pNormalGS;

	ComPtr<ID3D11VertexShader> m_pCharVS;
	ComPtr<ID3D11PixelShader> m_pCharPS;
	ComPtr<ID3D11GeometryShader> m_pCharGS;

	ComPtr<ID3D11VertexShader> m_pPlaneVS;
	ComPtr<ID3D11PixelShader> m_pPlanePS;


	ComPtr<ID3D11InputLayout> m_pVertexPosColorLayout;			// VertexPosColor输入布局
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalColorLayout;	// VertexPosNormalColor输入布局
	ComPtr<ID3D11InputLayout> m_pVertexPosNormalTexLayout;		// VertexPosNormalTex 输入布局
	ComPtr<ID3D11ShaderResourceView> m_pTexture;				// 用于绘制的纹理
	ComPtr<ID3D11ShaderResourceView> m_pMirrorTexture;			// 镜面的纹理
};

//
// BasicEffect
//

namespace
{
	// BasicEffect单例
	static BasicEffect * g_pInstance = nullptr;
}

BasicEffect::BasicEffect()
{
	if (g_pInstance)
		throw std::exception("BasicEffect is a singleton!");
	g_pInstance = this;
	pImpl = std::make_unique<BasicEffect::Impl>();
}

BasicEffect::~BasicEffect()
{
}

BasicEffect::BasicEffect(BasicEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
}

BasicEffect & BasicEffect::operator=(BasicEffect && moveFrom) noexcept
{
	pImpl.swap(moveFrom.pImpl);
	return *this;
}

BasicEffect & BasicEffect::Get()
{
	if (!g_pInstance)
		throw std::exception("BasicEffect needs an instance!");
	return *g_pInstance;
}

bool BasicEffect::InitAll(ID3D11Device * device)
{
	if (!device)
		return false;

	if (!pImpl->m_pCBuffers.empty())
		return true;

	if (!RenderStates::IsInit())
		throw std::exception("RenderStates need to be initialized first!");

	ComPtr<ID3DBlob> blob;
	
	// 创建顶点着色器和顶点布局

	HR(CreateShaderFromFile(L"HLSL\\Char_VS.cso", L"HLSL\\Char_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCharVS.GetAddressOf()));
	HR(device->CreateInputLayout(VertexPosNormalColor::inputLayout, ARRAYSIZE(VertexPosNormalColor::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalColorLayout.GetAddressOf()));

	HR(CreateShaderFromFile(L"HLSL\\Plane_VS.cso", L"HLSL\\Plane_VS.hlsl", "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pPlaneVS.GetAddressOf()));
	HR(device->CreateInputLayout(VertexPosNormalTex::inputLayout, ARRAYSIZE(VertexPosNormalTex::inputLayout),
		blob->GetBufferPointer(), blob->GetBufferSize(), pImpl->m_pVertexPosNormalTexLayout.GetAddressOf()));

	
	HR(CreateShaderFromFile(L"HLSL\\Char_PS.cso", L"HLSL\\Char_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCharPS.GetAddressOf()));
	
	HR(CreateShaderFromFile(L"HLSL\\Plane_PS.cso", L"HLSL\\Plane_PS.hlsl", "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pPlanePS.GetAddressOf()));
	
	
	HR(CreateShaderFromFile(L"HLSL\\Char_GS.cso", L"HLSL\\Char_GS.hlsl", "GS", "gs_5_0", blob.ReleaseAndGetAddressOf()));
	HR(device->CreateGeometryShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, pImpl->m_pCharGS.GetAddressOf()));

	HR(CreateDDSTextureFromFile(device, L"Texture\\Plane.dds", nullptr, pImpl->m_pTexture.GetAddressOf()));
	HR(CreateDDSTextureFromFile(device, L"Texture\\ice.dds", nullptr, pImpl->m_pMirrorTexture.GetAddressOf()));
	pImpl->m_pCBuffers.assign({
		&pImpl->m_CBDrawing,
		&pImpl->m_CBStates,
		&pImpl->m_CBFrame, 
		&pImpl->m_CBOnResize, 
		&pImpl->m_CBRarely,
		});

	// 创建常量缓冲区
	for (auto& pBuffer : pImpl->m_pCBuffers)
	{
		HR(pBuffer->CreateBuffer(device));
	}

	// 设置调试对象名
	/*D3D11SetDebugObjectName(pImpl->m_pVertexPosColorLayout.Get(), "VertexPosColorLayout");*/
	D3D11SetDebugObjectName(pImpl->m_pVertexPosNormalColorLayout.Get(), "VertexPosNormalColorLayout");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[0]->cBuffer.Get(), "CBFrame");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[1]->cBuffer.Get(), "CBOnResize");
	D3D11SetDebugObjectName(pImpl->m_pCBuffers[2]->cBuffer.Get(), "CBRarely");
	
	D3D11SetDebugObjectName(pImpl->m_pCharVS.Get(), "Char_VS");
	D3D11SetDebugObjectName(pImpl->m_pCharGS.Get(), "Char_GS");
	D3D11SetDebugObjectName(pImpl->m_pCharPS.Get(), "Char_PS");

	return true;
}

void BasicEffect::SetRenderChar(ID3D11DeviceContext* deviceContext) 
{
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalColorLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pCharVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(pImpl->m_pCharGS.Get(), nullptr, 0);
	deviceContext->RSSetState(nullptr);
	deviceContext->PSSetShader(pImpl->m_pCharPS.Get(), nullptr, 0);
}

void BasicEffect::SetRenderPlane(ID3D11DeviceContext* deviceContext) {
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pPlaneVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->PSSetShader(pImpl->m_pPlanePS.Get(), nullptr, 0);
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pTexture.GetAddressOf());
}
void BasicEffect::SetRenderMirror(ID3D11DeviceContext* deviceContext) {
	deviceContext->IASetInputLayout(pImpl->m_pVertexPosNormalTexLayout.Get());
	deviceContext->VSSetShader(pImpl->m_pPlaneVS.Get(), nullptr, 0);
	deviceContext->GSSetShader(nullptr, nullptr, 0);
	deviceContext->PSSetShader(pImpl->m_pPlanePS.Get(), nullptr, 0);
	deviceContext->PSSetShaderResources(0, 1, pImpl->m_pMirrorTexture.GetAddressOf());
}
void XM_CALLCONV BasicEffect::SetWorldMatrix(DirectX::FXMMATRIX W)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.world = XMMatrixTranspose(W);
	cBuffer.data.worldInvTranspose = XMMatrixInverse(nullptr, W);	// 两次转置抵消
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetViewMatrix(FXMMATRIX V)
{
	auto& cBuffer = pImpl->m_CBFrame;
	cBuffer.data.view = XMMatrixTranspose(V);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetProjMatrix(FXMMATRIX P)
{
	auto& cBuffer = pImpl->m_CBOnResize;
	cBuffer.data.proj = XMMatrixTranspose(P);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetDirLight(size_t pos, const DirectionalLight & dirLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.dirLight[pos] = dirLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetPointLight(size_t pos, const PointLight & pointLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.pointLight[pos] = pointLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetSpotLight(size_t pos, const SpotLight & spotLight)
{
	auto& cBuffer = pImpl->m_CBRarely;
	cBuffer.data.spotLight[pos] = spotLight;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void BasicEffect::SetMaterial(const Material & material)
{
	auto& cBuffer = pImpl->m_CBDrawing;
	cBuffer.data.material = material;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetEyePos(FXMVECTOR eyePos)
{
	auto& cBuffer = pImpl->m_CBFrame;
	XMStoreFloat3(&cBuffer.data.eyePos, eyePos);
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}


void BasicEffect::SetDrawingStates(int drawingStates)
{
	auto& cBuffer = pImpl->m_CBStates;
	cBuffer.data.isReflection = drawingStates;
	pImpl->m_IsDirty = cBuffer.isDirty = true;
}

void XM_CALLCONV BasicEffect::SetReflectMatrix(FXMMATRIX reflectMatrix)
{
	auto& CBuffer = pImpl->m_CBRarely;
	CBuffer.data.reflection = reflectMatrix;
	pImpl->m_IsDirty = CBuffer.isDirty = true;
}


void BasicEffect::Apply(ID3D11DeviceContext * deviceContext)
{
	auto& pCBuffers = pImpl->m_pCBuffers;
	// 将缓冲区绑定到渲染管线上
	pCBuffers[0]->BindVS(deviceContext);
	pCBuffers[1]->BindVS(deviceContext);
	pCBuffers[2]->BindVS(deviceContext);
	pCBuffers[3]->BindVS(deviceContext);
	pCBuffers[4]->BindVS(deviceContext);
	pCBuffers[2]->BindGS(deviceContext);
	pCBuffers[3]->BindGS(deviceContext);
	pCBuffers[0]->BindPS(deviceContext);
	pCBuffers[1]->BindPS(deviceContext);
	pCBuffers[2]->BindPS(deviceContext);
	pCBuffers[4]->BindPS(deviceContext);
	if (pImpl->m_IsDirty)
	{
		pImpl->m_IsDirty = false;
		for (auto& pCBuffer : pCBuffers)
		{
			pCBuffer->UpdateBuffer(deviceContext);
		}
	}
}






