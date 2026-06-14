//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"
#include "Player.h"

#define MAX_LIGHTS			16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

class CTerrainObject;
class CProjectileObject;
class CHudBarObject;
class CEffectObject;
class CTankObject;

struct SLevel1TargetState
{
	int							m_nObjectIndex = -1;
	int							m_nWave = 0;
	int							m_nMaxHP = 0;
	int							m_nHP = 0;
	bool						m_bActive = false;
	bool						m_bDestroying = false;
	float						m_fDestroyElapsedTime = 0.0f;
	float						m_fCollisionRadius = 0.0f;
	XMFLOAT3					m_xmf3StartPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3BasePosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT4X4					m_xmf4x4StartTransform;
	float						m_fMoveAngle = 0.0f;
	float						m_fMoveSpeed = 0.0f;
	float						m_fMoveRadius = 0.0f;
	float						m_fMovePhase = 0.0f;
	float						m_fMoveRadiusX = 0.0f;
	float						m_fMoveRadiusZ = 0.0f;
	float						m_fBobAmplitude = 0.0f;
	float						m_fBobSpeed = 1.0f;
	float						m_fFireCooldown = 0.0f;
	float						m_fFireInterval = 2.0f;
	int							m_nProjectileDamage = 5;
};

enum GAME_SCENE_MODE
{
	GAME_SCENE_START = 0,
	GAME_SCENE_MENU,
	GAME_SCENE_TUTORIAL,
	GAME_SCENE_LEVEL1,
	GAME_SCENE_LEVEL2,
	GAME_SCENE_LEVEL3
};

class CScene
{
public:
    CScene();
    ~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	void BuildDefaultLightsAndMaterials();
	void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	bool ProcessInput(UCHAR *pKeysBuffer);
	bool IsLevelPlaying() const { return(m_nSceneMode >= GAME_SCENE_TUTORIAL); }
	bool IsLevel1Cleared() const { return(m_bLevel1Cleared); }
	bool IsLevel1Failed() const { return(m_bLevel1Failed); }
	bool ShouldRenderMainPlayer() const { return(m_nSceneMode != GAME_SCENE_LEVEL2); }
	bool ShouldUpdateMainPlayer() const { return(m_nSceneMode != GAME_SCENE_LEVEL2); }
	XMFLOAT3 GetDisplayPlayerPosition() const;
	void GetClearColor(float pfClearColor[4]) const;
    void AnimateObjects(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL);
	void RenderLevel1HudOverlay(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);

	void ReleaseUploadBuffers();

	CPlayer						*m_pPlayer = NULL;
	CTerrainObject				*m_pTerrain = NULL;

public:
	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;

	CGameObject					**m_ppGameObjects = NULL;
	int							m_nGameObjects = 0;

	LIGHT						*m_pLights = NULL;
	int							m_nLights = 0;

	XMFLOAT4					m_xmf4GlobalAmbient;

	ID3D12Resource				*m_pd3dcbLights = NULL;
	LIGHTS						*m_pcbMappedLights = NULL;

	float						m_fElapsedTime = 0.0f;

private:
	CGameObject *CreateTextObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const char *pstrFileName, const XMFLOAT3& xmf3Position, float fScale);
	void SetSceneMode(GAME_SCENE_MODE nSceneMode);
	void ResetLevel1();
	void ResetLevel2();
	void ClampPlayerToTerrain();
	bool ProcessLevel2Input(UCHAR *pKeysBuffer);
	void RotateLevel2PlayerTank(float fYawDelta);
	void UpdateLevel2PlayerTankTransform(bool bSyncMainPlayerPosition);
	void UpdateLevel2Camera();
	void UpdateLevel2CameraLookOnly();
	void FirePlayerProjectile();
	void FireLevel1EnemyProjectile(int nTargetIndex);
	void InitializeLevel1Targets();
	void ResetLevel1Targets();
	void ActivateLevel1Wave(int nWave);
	void UpdateLevel1Targets(float fTimeElapsed);
	void OrientLevel1TargetToPlayer(int nTargetIndex);
	void UpdateLevel1EnemyFire(float fTimeElapsed);
	void UpdateLevel1ClearText();
	void UpdateLevel1GameOverText();
	void UpdateLevel1HudBars();
	void RenderLevel1HudBars(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void BuildLevel1Decorations(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void RenderLevel1Decorations(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void ReleaseLevel1Decorations();
	void BuildLevel2Objects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void CenterLevel2TankVisualPivot(CTankObject *pTankObject);
	void UpdateLevel2Objects(float fTimeElapsed);
	void RenderLevel2Objects(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void ReleaseLevel2Objects();
	void BuildLevel2Projectiles(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ResetLevel2Projectiles();
	void UpdateLevel2Projectiles(float fTimeElapsed);
	void RenderLevel2Projectiles(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void ReleaseLevel2Projectiles();
	void FireLevel2PlayerProjectile();
	void CheckLevel2ProjectileEnemyCollisions();
	void DestroyLevel2EnemyTank(int nEnemyIndex);
	void DestroyAllLevel2EnemyTanks();
	bool IsLevel2Cleared() const;
	void UpdateLevel2ClearState();
	void UpdateLevel2YouWinText();
	void RenderLevel2YouWinText(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void BuildLevel1Effects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ResetLevel1Effects();
	void UpdateLevel1Effects(float fTimeElapsed);
	void RenderLevel1Effects(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void ReleaseLevel1Effects();
	CEffectObject *FindInactiveLevel1Effect();
	void SpawnLevel1HitEffect(const XMFLOAT3& xmf3Position);
	void SpawnLevel1ExplosionEffect(const XMFLOAT3& xmf3Position);
	void SpawnPlayerHitEffect(const XMFLOAT3& xmf3Position);
	void BuildStartNameExplosionEffects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ResetStartNameExplosionEffects();
	void UpdateStartNameExplosionEffects(float fTimeElapsed);
	void RenderStartNameExplosionEffects(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera);
	void ReleaseStartNameExplosionEffects();
	CEffectObject *FindInactiveStartNameExplosionEffect();
	void SpawnStartNameExplosionEffects();
	void CheckProjectileTargetCollisions();
	void CheckEnemyProjectilePlayerCollisions();
	void ApplyDamageToLevel1Target(int nTargetIndex, int nDamage);
	void ApplyDamageToPlayer(int nDamage);
	bool IsCurrentLevel1WaveCleared() const;
	void AdvanceLevel1WaveIfNeeded();
	void ResetEnemyProjectiles();
	int GetHudEnemyTargetIndex() const;
	bool IsActiveLevel1TargetObject(int nObjectIndex) const;
	bool IsVisibleObject(int nObject) const;
	bool IsStartTitleHover(int x, int y) const;
	bool IsStartNameHover(int x, int y) const;
	bool IsMenuStartHover(int x, int y, int *pnMenuItem = NULL) const;
	bool IsMenuEndHover(int x, int y) const;

	GAME_SCENE_MODE			m_nSceneMode = GAME_SCENE_START;
	bool						m_bTitleHovered = false;
	bool						m_bNameHovered = false;
	bool						m_bNameExploding = false;
	float						m_fNameExplosionElapsedTime = 0.0f;
	float						m_fModeElapsedTime = 0.0f;
	float						m_fTitleHoverRotation = 0.0f;
	float						m_fNameHoverRotation = 0.0f;
	XMFLOAT4X4				m_xmf4x4StartTitleBaseTransform;
	XMFLOAT4X4				m_xmf4x4StartNameBaseTransform;
	bool						m_bMenuStartHovered[4] = { false, false, false, false };
	bool						m_bMenuEndHovered = false;
	float						m_fMenuStartHoverRotation[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float						m_fMenuEndHoverRotation = 0.0f;
	XMFLOAT4X4				m_xmf4x4MenuStartBaseTransforms[4];
	XMFLOAT4X4				m_xmf4x4MenuEndBaseTransform;
	XMFLOAT4X4				m_xmf4x4Level1ClearBaseTransform;
	XMFLOAT4X4				m_xmf4x4Level1GameOverBaseTransform;
	CProjectileObject			**m_ppProjectiles = NULL;
	int							m_nProjectiles = 0;
	float						m_fProjectileFireCooldown = 0.0f;
	CProjectileObject			**m_ppEnemyProjectiles = NULL;
	int							m_nEnemyProjectiles = 0;
	CHudBarObject				**m_ppHudBars = NULL;
	int							m_nHudBars = 0;
	CCamera						*m_pHudCamera = NULL;
	CGameObject					**m_ppLevel1Decorations = NULL;
	int							m_nLevel1Decorations = 0;
	CEffectObject				**m_ppLevel1Effects = NULL;
	int							m_nLevel1Effects = 0;
	CTerrainObject				*m_pLevel2Terrain = NULL;
	CTankObject					*m_pLevel2PlayerTank = NULL;
	CTankObject					**m_ppLevel2EnemyTanks = NULL;
	int							m_nLevel2EnemyTanks = 0;
	bool						*m_pbLevel2EnemyTankAlive = NULL;
	CGameObject					**m_ppLevel2Obstacles = NULL;
	int							m_nLevel2Obstacles = 0;
	CProjectileObject			**m_ppLevel2Projectiles = NULL;
	int							m_nLevel2Projectiles = 0;
	float						m_fLevel2ProjectileFireCooldown = 0.0f;
	bool						m_bLevel2Cleared = false;
	float						m_fLevel2ClearElapsedTime = 0.0f;
	CGameObject					*m_pLevel2YouWinObject = NULL;
	XMFLOAT4X4					m_xmf4x4Level2YouWinBaseTransform;
	bool						m_bPendingLevel2Reset = false;
	bool						m_bLevel2MouseDragging = false;
	POINT						m_ptLevel2OldCursorPos;
	XMFLOAT3					m_xmf3Level2PlayerPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float						m_fLevel2PlayerYaw = 0.0f;
	CEffectObject				**m_ppStartNameExplosionEffects = NULL;
	int							m_nStartNameExplosionEffects = 0;
	int							m_nPlayerMaxHP = 100;
	int							m_nPlayerHP = 100;
	int							m_nLastHitLevel1TargetIndex = -1;
	float						m_fLastHitTargetDisplayElapsedTime = 0.0f;
	SLevel1TargetState			*m_pLevel1Targets = NULL;
	int							m_nLevel1Targets = 0;
	int							m_nCurrentLevel1Wave = 1;
	bool						m_bLevel1Cleared = false;
	float						m_fLevel1ClearElapsedTime = 0.0f;
	bool						m_bLevel1Failed = false;
	float						m_fLevel1FailedElapsedTime = 0.0f;
};
