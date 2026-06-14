#pragma once

#include "Object.h"

static const int MAX_PROJECTILES = 100;
static const float PROJECTILE_FIRE_COOLDOWN = 0.10f;

class CProjectileMesh : public CMesh
{
public:
	CProjectileMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, float fWidth = 0.7f, float fHeight = 0.7f, float fLength = 6.0f);
	virtual ~CProjectileMesh();

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

class CProjectileObject : public CGameObject
{
public:
	CProjectileObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	CProjectileObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, float fWidth, float fHeight, float fLength,
		const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive, const XMFLOAT4& xmf4Specular,
		float fSpeed, float fLifeTime, float fCollisionRadius, int nDamage);
	virtual ~CProjectileObject() { }

	void Fire(const XMFLOAT3& position, const XMFLOAT3& direction);
	void Reset();
	bool IsActive() const { return(m_bActive); }
	float GetCollisionRadius() const { return(m_fCollisionRadius); }
	int GetDamage() const { return(m_nDamage); }

	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);

	bool							m_bActive = false;
	XMFLOAT3						m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	float							m_fSpeed = 700.0f;
	float							m_fLifeTime = 2.0f;
	float							m_fElapsedLifeTime = 0.0f;
	float							m_fCollisionRadius = 2.0f;
	int								m_nDamage = 1;
};
