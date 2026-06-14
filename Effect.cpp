#include "stdafx.h"
#include "Effect.h"

CEffectObject::CEffectObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
	: CDecorationBoxObject(pd3dDevice, pd3dCommandList, XMFLOAT4(1.0f, 0.55f, 0.08f, 1.0f), XMFLOAT4(1.0f, 0.45f, 0.05f, 1.0f), XMFLOAT4(0.85f, 0.25f, 0.02f, 1.0f))
{
	Reset();
}

void CEffectObject::SetEffectColors(const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive)
{
	if (!m_ppMaterials || !m_ppMaterials[0] || !m_ppMaterials[0]->m_pMaterialColors) return;

	m_ppMaterials[0]->m_pMaterialColors->m_xmf4Ambient = xmf4Ambient;
	m_ppMaterials[0]->m_pMaterialColors->m_xmf4Diffuse = xmf4Diffuse;
	m_ppMaterials[0]->m_pMaterialColors->m_xmf4Emissive = xmf4Emissive;
	m_ppMaterials[0]->m_pMaterialColors->m_xmf4Specular = XMFLOAT4(0.45f, 0.35f, 0.12f, 12.0f);
}

void CEffectObject::Spawn(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Velocity, float fLifeTime, float fStartScale, float fEndScale,
	const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive)
{
	m_bActive = true;
	m_fElapsedTime = 0.0f;
	m_fLifeTime = fLifeTime;
	m_xmf3StartPosition = xmf3Position;
	m_xmf3Velocity = xmf3Velocity;
	m_fStartScale = fStartScale;
	m_fEndScale = fEndScale;
	SetEffectColors(xmf4Ambient, xmf4Diffuse, xmf4Emissive);

	m_xmf4x4Transform = Matrix4x4::Identity();
	SetScale(fStartScale, fStartScale, fStartScale);
	SetPosition(xmf3Position);
}

void CEffectObject::Reset()
{
	m_bActive = false;
	m_fElapsedTime = 0.0f;
	m_xmf4x4Transform = Matrix4x4::Identity();
	SetScale(0.01f, 0.01f, 0.01f);
	SetPosition(XMFLOAT3(0.0f, -10000.0f, 0.0f));
}

void CEffectObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (!m_bActive) return;

	m_fElapsedTime += fTimeElapsed;
	if (m_fElapsedTime >= m_fLifeTime)
	{
		Reset();
		return;
	}

	float fRatio = (m_fLifeTime > 0.0f) ? (m_fElapsedTime / m_fLifeTime) : 1.0f;
	float fScale = m_fStartScale + (m_fEndScale - m_fStartScale) * fRatio;
	XMFLOAT3 xmf3Position = Vector3::Add(m_xmf3StartPosition, Vector3::ScalarProduct(m_xmf3Velocity, m_fElapsedTime, false));

	m_xmf4x4Transform = Matrix4x4::Identity();
	SetScale(fScale, fScale, fScale);
	Rotate(180.0f * fRatio, 420.0f * fRatio, 95.0f * fRatio);
	SetPosition(xmf3Position);
	UpdateTransform(pxmf4x4Parent);
}
