#include "stdafx.h"
#include "Decoration.h"

CDecorationBoxMesh::CDecorationBoxMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
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

CDecorationBoxMesh::~CDecorationBoxMesh()
{
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CDecorationBoxMesh::ReleaseUploadBuffers()
{
	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	m_pd3dPositionUploadBuffer = NULL;

	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;

	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dIndexUploadBuffer = NULL;
}

void CDecorationBoxMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList)
{
	Render(pd3dCommandList, 0);
}

void CDecorationBoxMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW d3dVertexBufferViews[2] = { m_d3dPositionBufferView, m_d3dNormalBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 2, d3dVertexBufferViews);
	pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
	pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

CDecorationBoxObject::CDecorationBoxObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive)
{
	SetMesh(new CDecorationBoxMesh(pd3dDevice, pd3dCommandList));

	m_nMaterials = 1;
	m_ppMaterials = new CMaterial*[m_nMaterials];
	m_ppMaterials[0] = new CMaterial();
	m_ppMaterials[0]->SetIlluminatedShader();

	CMaterialColors *pMaterialColors = new CMaterialColors();
	pMaterialColors->m_xmf4Ambient = xmf4Ambient;
	pMaterialColors->m_xmf4Diffuse = xmf4Diffuse;
	pMaterialColors->m_xmf4Emissive = xmf4Emissive;
	pMaterialColors->m_xmf4Specular = XMFLOAT4(0.18f, 0.18f, 0.18f, 8.0f);
	m_ppMaterials[0]->SetMaterialColors(pMaterialColors);
}
