#ifndef GAMEAPP_H
#define GAMEAPP_H

#include "d3dApp.h"
#include "Effects.h"
#include "Vertex.h"
#include "Geometry.h"
class GameApp : public D3DApp
{
public:

	enum class LightMode {DirectionalLight, PointLight, SpotLight};
public:
	GameApp(HINSTANCE hInstance);
	~GameApp();

	bool Init();
	void OnResize();
	void UpdateScene(float dt);
	void DrawScene();

private:
	bool InitResource();

	template<class VertexType>
	void ResetMesh(const Geometry::MeshData<VertexType>& meshData);


private:
	

	ComPtr<ID3D11Buffer> m_pVertexBuffer;						// 顶点集合
	ComPtr<ID3D11Buffer> m_pIndexBuffer;						// 索引缓冲区
	int m_VertexCount;										    // 顶点数目
	int m_IndexCount;											// 索引数目
	LightMode m_LightMode;										// 灯光模式
	BasicEffect m_BasicEffect;							        // 对象渲染特效管理

	DirectionalLight dirLight;									// 方向光源
	PointLight pointLight;										// 点光源
	SpotLight spotLight;										// 聚光灯
	DirectX::XMFLOAT3 viewPos;									// 观察位置
	DirectX::XMFLOAT3 viewRot;									// 观察方向

	Material mirrorMaterial;									// 镜子材质
	Material charMaterial;										// 字符材质
	float angle;												// 文字旋转角度

	/*template<class VertexType = VertexPosNormalTex, class IndexType = WORD>*/
	Geometry::MeshData<VertexPosNormalColor, WORD> charMeshData;
	Geometry::MeshData<VertexPosNormalTex, WORD> planeMeshData;
	Geometry::MeshData<VertexPosNormalTex, WORD> mirrorMeshData;

	DirectX::XMMATRIX planeWorldMatrix;
	DirectX::XMMATRIX mirrorWorldMatrix;
};


#endif