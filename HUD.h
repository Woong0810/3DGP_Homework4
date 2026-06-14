#pragma once

#include "Object.h"

class CHudBarMesh : public CMesh
{
public:
	CHudBarMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual ~CHudBarMesh();

	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet);

protected:
	ID3D12Resource					*m_pd3dPositionBuffer = NULL;
	ID3D12Resource					*m_pd3dPositionUploadBuffer = NULL;
	D3D12_VERTEX_BUFFER_VIEW		m_d3dPositionBufferView;

	ID3D12Resource					*m_pd3dNormalBuffer = NULL;
	ID3D12Resource					*m_pd3dNormalUploadBuffer = NULL;
	D3D12_VERTEX_BUFFER_VIEW		m_d3dNormalBufferView;

	ID3D12Resource					*m_pd3dIndexBuffer = NULL;
	ID3D12Resource					*m_pd3dIndexUploadBuffer = NULL;
	D3D12_INDEX_BUFFER_VIEW			m_d3dIndexBufferView;

	UINT							m_nIndices = 0;
};

class CHudBarObject : public CGameObject
{
public:
	CHudBarObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive);

	void SetHudTransform(const XMFLOAT3& xmf3Center, const XMFLOAT3& xmf3Right, const XMFLOAT3& xmf3Up, const XMFLOAT3& xmf3Look, float fWidth, float fHeight, float fDepth);
};
