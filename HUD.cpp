#include "stdafx.h"
#include "HUD.h"

CHudBarMesh::CHudBarMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nType = VERTEXT_POSITION | VERTEXT_NORMAL;
	m_nVertices = 24;

	const float hx = 0.5f;
	const float hy = 0.5f;
	const float hz = 0.5f;

	XMFLOAT3 positions[24] =
	{
		XMFLOAT3(-hx, -hy, -hz), XMFLOAT3(-hx, +hy, -hz), XMFLOAT3(+hx, +hy, -hz), XMFLOAT3(+hx, -hy, -hz),
		XMFLOAT3(-hx, -hy, +hz), XMFLOAT3(+hx, -hy, +hz), XMFLOAT3(+hx, +hy, +hz), XMFLOAT3(-hx, +hy, +hz),
		XMFLOAT3(-hx, -hy, -hz), XMFLOAT3(-hx, -hy, +hz), XMFLOAT3(-hx, +hy, +hz), XMFLOAT3(-hx, +hy, -hz),
		XMFLOAT3(+hx, -hy, -hz), XMFLOAT3(+hx, +hy, -hz), XMFLOAT3(+hx, +hy, +hz), XMFLOAT3(+hx, -hy, +hz),
		XMFLOAT3(-hx, +hy, -hz), XMFLOAT3(-hx, +hy, +hz), XMFLOAT3(+hx, +hy, +hz), XMFLOAT3(+hx, +hy, -hz),
		XMFLOAT3(-hx, -hy, -hz), XMFLOAT3(+hx, -hy, -hz), XMFLOAT3(+hx, -hy, +hz), XMFLOAT3(-hx, -hy, +hz)
	};

	XMFLOAT3 normals[24] =
	{
		XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f),
		XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, +1.0f),
		XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f),
		XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f), XMFLOAT3(+1.0f, 0.0f, 0.0f),
		XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f),
		XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f)
	};

	UINT indices[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 5, 6, 4, 6, 7,
		8, 9, 10, 8, 10, 11,
		12, 13, 14, 12, 14, 15,
		16, 17, 18, 16, 18, 19,
		20, 21, 22, 20, 22, 23
	};
	m_nIndices = _countof(indices);

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, positions, sizeof(positions), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);
	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(positions);

	m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, normals, sizeof(normals), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);
	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(normals);

	m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, indices, sizeof(indices), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(indices);
}

CHudBarMesh::~CHudBarMesh()
{
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CHudBarMesh::ReleaseUploadBuffers()
{
	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	m_pd3dPositionUploadBuffer = NULL;

	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;

	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dIndexUploadBuffer = NULL;
}

void CHudBarMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList)
{
	Render(pd3dCommandList, 0);
}

void CHudBarMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW d3dVertexBufferViews[2] = { m_d3dPositionBufferView, m_d3dNormalBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 2, d3dVertexBufferViews);
	pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
	pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

CHudBarObject::CHudBarObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive)
{
	SetMesh(new CHudBarMesh(pd3dDevice, pd3dCommandList));

	m_nMaterials = 1;
	m_ppMaterials = new CMaterial*[m_nMaterials];
	m_ppMaterials[0] = new CMaterial();
	m_ppMaterials[0]->SetIlluminatedShader();

	CMaterialColors *pMaterialColors = new CMaterialColors();
	pMaterialColors->m_xmf4Ambient = xmf4Ambient;
	pMaterialColors->m_xmf4Diffuse = xmf4Diffuse;
	pMaterialColors->m_xmf4Emissive = xmf4Emissive;
	pMaterialColors->m_xmf4Specular = XMFLOAT4(0.12f, 0.12f, 0.12f, 8.0f);
	m_ppMaterials[0]->SetMaterialColors(pMaterialColors);
}

void CHudBarObject::SetHudTransform(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Right, const XMFLOAT3& xmf3Up, const XMFLOAT3& xmf3Look, float fWidth, float fHeight, float fDepth)
{
	XMFLOAT3 xmf3NormalizedRight = Vector3::Normalize(xmf3Right);
	XMFLOAT3 xmf3NormalizedUp = Vector3::Normalize(xmf3Up);
	XMFLOAT3 xmf3NormalizedLook = Vector3::Normalize(xmf3Look);
	XMFLOAT3 xmf3ScaledRight = Vector3::ScalarProduct(xmf3NormalizedRight, fWidth, false);
	XMFLOAT3 xmf3ScaledUp = Vector3::ScalarProduct(xmf3NormalizedUp, fHeight, false);
	XMFLOAT3 xmf3ScaledLook = Vector3::ScalarProduct(xmf3NormalizedLook, fDepth, false);

	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4Transform._11 = xmf3ScaledRight.x; m_xmf4x4Transform._12 = xmf3ScaledRight.y; m_xmf4x4Transform._13 = xmf3ScaledRight.z;
	m_xmf4x4Transform._21 = xmf3ScaledUp.x;    m_xmf4x4Transform._22 = xmf3ScaledUp.y;    m_xmf4x4Transform._23 = xmf3ScaledUp.z;
	m_xmf4x4Transform._31 = xmf3ScaledLook.x;  m_xmf4x4Transform._32 = xmf3ScaledLook.y;  m_xmf4x4Transform._33 = xmf3ScaledLook.z;
	m_xmf4x4Transform._41 = xmf3Center.x;      m_xmf4x4Transform._42 = xmf3Center.y;      m_xmf4x4Transform._43 = xmf3Center.z;
	UpdateTransform(NULL);
}
