#include "stdafx.h"
#include "Bullet.h"

#include <vector>

static XMFLOAT3 NormalizeVector(const XMFLOAT3& v, const XMFLOAT3& fallback = XMFLOAT3(0.0f, 0.0f, 1.0f))
{
	XMVECTOR xmv = XMLoadFloat3(&v);
	if (XMVectorGetX(XMVector3LengthSq(xmv)) <= 0.000001f) return(fallback);

	XMFLOAT3 result;
	XMStoreFloat3(&result, XMVector3Normalize(xmv));
	return(result);
}

CProjectileMesh::CProjectileMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, float fWidth, float fHeight, float fLength)
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nType = VERTEXT_POSITION | VERTEXT_NORMAL;
	m_nVertices = 24;

	float hx = fWidth * 0.5f;
	float hy = fHeight * 0.5f;
	float hz = fLength * 0.5f;

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

CProjectileMesh::~CProjectileMesh()
{
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CProjectileMesh::ReleaseUploadBuffers()
{
	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	m_pd3dPositionUploadBuffer = NULL;

	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;

	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dIndexUploadBuffer = NULL;
}

void CProjectileMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList)
{
	Render(pd3dCommandList, 0);
}

void CProjectileMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW d3dVertexBufferViews[2] = { m_d3dPositionBufferView, m_d3dNormalBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 2, d3dVertexBufferViews);
	pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
	pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

CProjectileObject::CProjectileObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
	: CProjectileObject(pd3dDevice, pd3dCommandList, 0.7f, 0.7f, 6.0f,
		XMFLOAT4(0.9f, 0.75f, 0.25f, 1.0f), XMFLOAT4(1.0f, 0.85f, 0.15f, 1.0f),
		XMFLOAT4(0.35f, 0.25f, 0.03f, 1.0f), XMFLOAT4(0.6f, 0.45f, 0.1f, 16.0f),
		700.0f, 2.0f, 2.0f, 1)
{
}

CProjectileObject::CProjectileObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, float fWidth, float fHeight, float fLength,
	const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive, const XMFLOAT4& xmf4Specular,
	float fSpeed, float fLifeTime, float fCollisionRadius, int nDamage)
{
	SetMesh(new CProjectileMesh(pd3dDevice, pd3dCommandList, fWidth, fHeight, fLength));

	m_nMaterials = 1;
	m_ppMaterials = new CMaterial*[m_nMaterials];
	m_ppMaterials[0] = new CMaterial();
	m_ppMaterials[0]->SetIlluminatedShader();

	CMaterialColors *pMaterialColors = new CMaterialColors();
	pMaterialColors->m_xmf4Ambient = xmf4Ambient;
	pMaterialColors->m_xmf4Diffuse = xmf4Diffuse;
	pMaterialColors->m_xmf4Emissive = xmf4Emissive;
	pMaterialColors->m_xmf4Specular = xmf4Specular;
	m_ppMaterials[0]->SetMaterialColors(pMaterialColors);

	m_fSpeed = fSpeed;
	m_fLifeTime = fLifeTime;
	m_fCollisionRadius = fCollisionRadius;
	m_nDamage = nDamage;

	Reset();
}

void CProjectileObject::Fire(const XMFLOAT3& position, const XMFLOAT3& direction)
{
	m_bActive = true;
	m_fElapsedLifeTime = 0.0f;
	m_xmf3Direction = NormalizeVector(direction);

	XMFLOAT3 up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMVECTOR xmvLook = XMLoadFloat3(&m_xmf3Direction);
	XMVECTOR xmvUp = XMLoadFloat3(&up);
	XMVECTOR xmvRight = XMVector3Cross(xmvUp, xmvLook);
	if (XMVectorGetX(XMVector3LengthSq(xmvRight)) <= 0.000001f)
	{
		up = XMFLOAT3(1.0f, 0.0f, 0.0f);
		xmvUp = XMLoadFloat3(&up);
		xmvRight = XMVector3Cross(xmvUp, xmvLook);
	}
	xmvRight = XMVector3Normalize(xmvRight);
	xmvUp = XMVector3Normalize(XMVector3Cross(xmvLook, xmvRight));

	XMFLOAT3 right;
	XMFLOAT3 correctedUp;
	XMStoreFloat3(&right, xmvRight);
	XMStoreFloat3(&correctedUp, xmvUp);

	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4Transform._11 = right.x; m_xmf4x4Transform._12 = right.y; m_xmf4x4Transform._13 = right.z;
	m_xmf4x4Transform._21 = correctedUp.x; m_xmf4x4Transform._22 = correctedUp.y; m_xmf4x4Transform._23 = correctedUp.z;
	m_xmf4x4Transform._31 = m_xmf3Direction.x; m_xmf4x4Transform._32 = m_xmf3Direction.y; m_xmf4x4Transform._33 = m_xmf3Direction.z;
	m_xmf4x4Transform._41 = position.x; m_xmf4x4Transform._42 = position.y; m_xmf4x4Transform._43 = position.z;
	UpdateTransform(NULL);
}

void CProjectileObject::Reset()
{
	m_bActive = false;
	m_fElapsedLifeTime = 0.0f;
	m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
}

void CProjectileObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (!m_bActive) return;

	m_fElapsedLifeTime += fTimeElapsed;
	if (m_fElapsedLifeTime >= m_fLifeTime)
	{
		Reset();
		return;
	}

	m_xmf4x4Transform._41 += (m_xmf3Direction.x * m_fSpeed * fTimeElapsed);
	m_xmf4x4Transform._42 += (m_xmf3Direction.y * m_fSpeed * fTimeElapsed);
	m_xmf4x4Transform._43 += (m_xmf3Direction.z * m_fSpeed * fTimeElapsed);
	UpdateTransform(pxmf4x4Parent);
}
