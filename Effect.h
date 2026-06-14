#pragma once

#include "Decoration.h"

class CEffectObject : public CDecorationBoxObject
{
public:
	CEffectObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);

	void Spawn(const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Velocity, float fLifeTime, float fStartScale, float fEndScale,
		const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive);
	void Reset();
	bool IsActive() const { return(m_bActive); }

	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);

private:
	void SetEffectColors(const XMFLOAT4& xmf4Ambient, const XMFLOAT4& xmf4Diffuse, const XMFLOAT4& xmf4Emissive);

	bool						m_bActive = false;
	float						m_fElapsedTime = 0.0f;
	float						m_fLifeTime = 0.5f;
	XMFLOAT3					m_xmf3StartPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float						m_fStartScale = 1.0f;
	float						m_fEndScale = 1.0f;
};
