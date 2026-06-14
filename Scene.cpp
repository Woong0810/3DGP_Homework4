//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "Terrain.h"
#include "Bullet.h"
#include "HUD.h"
#include "Decoration.h"
#include "Effect.h"

static const int UI_TITLE_OBJECT = 0;
static const int UI_NAME_OBJECT = 1;
static const int UI_TUTORIAL_OBJECT = 2;
static const int UI_LEVEL1_OBJECT = 3;
static const int UI_LEVEL2_OBJECT = 4;
static const int UI_LEVEL3_OBJECT = 5;
static const int UI_TUTORIAL_START_OBJECT = 6;
static const int UI_LEVEL1_START_OBJECT = 7;
static const int UI_LEVEL2_START_OBJECT = 8;
static const int UI_LEVEL3_START_OBJECT = 9;
static const int UI_END_OBJECT = 10;
static const int UI_MENU_START_FIRST_OBJECT = UI_TUTORIAL_START_OBJECT;
static const int UI_MENU_START_COUNT = 4;
static const int WORLD_OBJECT_START = 11;
static const int WORLD_OBJECT_COUNT = 5;
static const int LEVEL1_CLEAR_OBJECT = WORLD_OBJECT_START + WORLD_OBJECT_COUNT;
static const int LEVEL1_GAMEOVER_OBJECT = LEVEL1_CLEAR_OBJECT + 1;
static const int TOTAL_SCENE_OBJECTS = LEVEL1_GAMEOVER_OBJECT + 1;
static const int HUD_BAR_COUNT = 4;
static const int HUD_PLAYER_BACKGROUND = 0;
static const int HUD_PLAYER_GAUGE = 1;
static const int HUD_ENEMY_BACKGROUND = 2;
static const int HUD_ENEMY_GAUGE = 3;
static const int MAX_ENEMY_PROJECTILES = 48;
static const int LEVEL1_EFFECT_COUNT = 96;
static const int START_NAME_EXPLOSION_EFFECT_COUNT = 32;
static const int LEVEL2_ENEMY_TANK_COUNT = 10;
static const int LEVEL2_OBSTACLE_COUNT = 60;
CScene::CScene()
{
}

CScene::~CScene()
{
}

void CScene::GetClearColor(float pfClearColor[4]) const
{
	const float pfDefaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	const float pfLevel1BattleClearColor[4] = { 0.42f, 0.52f, 0.58f, 1.0f };
	const bool bLevel1BattleScreen = ((m_nSceneMode == GAME_SCENE_LEVEL1) && !m_bLevel1Cleared && !m_bLevel1Failed);
	const float *pfSourceClearColor = bLevel1BattleScreen ? pfLevel1BattleClearColor : pfDefaultClearColor;
	for (int i = 0; i < 4; i++) pfClearColor[i] = pfSourceClearColor[i];
}

void CScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 1;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.22f, 0.22f, 0.22f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.35f, 0.35f, 0.35f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.85f, 0.85f, 0.80f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 0.0f);
	m_pLights[0].m_xmf3Direction = Vector3::Normalize(XMFLOAT3(0.35f, -1.0f, 0.25f));
}
CGameObject *CScene::CreateTextObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const char *pstrFileName, const XMFLOAT3& xmf3Position, float fScale)
{
	int nMeshesInHierarchy = 0;
	int pnMaterialsInHierarchy[64]{};
	CGameObject *pTextModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pstrFileName, &nMeshesInHierarchy, pnMaterialsInHierarchy);
	if (pTextModel) pTextModel->m_xmf4x4Transform = Matrix4x4::Identity();

	CGameObject *pTextObject = new CGameObject();
	pTextObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
	pTextObject->SetChild(pTextModel, true);
	pTextObject->SetScale(fScale, fScale, fScale);
	pTextObject->Rotate(0.0f, 180.0f, 0.0f);
	pTextObject->SetPosition(xmf3Position);
	return(pTextObject);
}

void CScene::SetSceneMode(GAME_SCENE_MODE nSceneMode)
{
	m_nSceneMode = nSceneMode;
	m_fModeElapsedTime = 0.0f;
	if (m_nSceneMode < GAME_SCENE_TUTORIAL)
	{
		::ReleaseCapture();
		::SetCursor(::LoadCursor(NULL, IDC_ARROW));
	}
	if (m_nSceneMode == GAME_SCENE_LEVEL1) ResetLevel1();
	if (m_nSceneMode == GAME_SCENE_LEVEL2) ResetLevel2();
	if (m_nSceneMode != GAME_SCENE_START)
	{
		m_bTitleHovered = false;
		m_bNameHovered = false;
		m_bNameExploding = false;
		m_fNameExplosionElapsedTime = 0.0f;
		ResetStartNameExplosionEffects();
		if (m_ppGameObjects && m_ppGameObjects[UI_NAME_OBJECT]) m_ppGameObjects[UI_NAME_OBJECT]->m_xmf4x4Transform = m_xmf4x4StartNameBaseTransform;
		m_fTitleHoverRotation = 0.0f;
		m_fNameHoverRotation = 0.0f;
	}
	if (m_nSceneMode != GAME_SCENE_MENU)
	{
		for (int i = 0; i < UI_MENU_START_COUNT; i++)
		{
			m_bMenuStartHovered[i] = false;
			m_fMenuStartHoverRotation[i] = 0.0f;
		}
		m_bMenuEndHovered = false;
		m_fMenuEndHoverRotation = 0.0f;
	}
}

void CScene::ResetLevel1()
{
	if (!m_pPlayer) return;

	const float fStartX = 0.0f;
	const float fStartZ = -120.0f;
	const float fStartAltitude = 60.0f;
	float fTerrainY = (m_pTerrain) ? m_pTerrain->GetHeight(fStartX, fStartZ) : 20.0f;
	XMFLOAT3 xmf3StartPosition = XMFLOAT3(fStartX, fTerrainY + fStartAltitude, fStartZ);

	m_pPlayer->ResetOrientation();
	m_pPlayer->SetPosition(xmf3StartPosition);
	m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	ClampPlayerToTerrain();

	CCamera *pLevel1Camera = m_pPlayer->ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
	if (pLevel1Camera)
	{
		XMFLOAT3 xmf3PlayerPosition = m_pPlayer->GetPosition();
		pLevel1Camera->SetTimeLag(0.25f);
		pLevel1Camera->SetOffset(XMFLOAT3(0.0f, 14.0f, -22.0f));
		pLevel1Camera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		pLevel1Camera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		pLevel1Camera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		pLevel1Camera->SetPosition(Vector3::Add(xmf3PlayerPosition, pLevel1Camera->GetOffset()));
		pLevel1Camera->SetLookAt(xmf3PlayerPosition);
		pLevel1Camera->RegenerateViewMatrix();
	}
	for (int i = 0; i < m_nProjectiles; i++) if (m_ppProjectiles[i]) m_ppProjectiles[i]->Reset();
	ResetEnemyProjectiles();
	ResetLevel1Effects();
	m_fProjectileFireCooldown = 0.0f;
	m_nPlayerHP = m_nPlayerMaxHP;
	m_nLastHitLevel1TargetIndex = -1;
	m_fLastHitTargetDisplayElapsedTime = 0.0f;
	m_bLevel1Cleared = false;
	m_fLevel1ClearElapsedTime = 0.0f;
	m_bLevel1Failed = false;
	m_fLevel1FailedElapsedTime = 0.0f;
	if (m_ppGameObjects && m_ppGameObjects[LEVEL1_CLEAR_OBJECT]) m_ppGameObjects[LEVEL1_CLEAR_OBJECT]->m_xmf4x4Transform = m_xmf4x4Level1ClearBaseTransform;
	if (m_ppGameObjects && m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]) m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->m_xmf4x4Transform = m_xmf4x4Level1GameOverBaseTransform;
	ResetLevel1Targets();

	char pstrDebug[128];
	sprintf_s(pstrDebug, "Level-1 Reset Player Position: %.2f, %.2f, %.2f\n", xmf3StartPosition.x, xmf3StartPosition.y, xmf3StartPosition.z);
	::OutputDebugStringA(pstrDebug);
}

void CScene::ResetLevel2()
{
	if (!m_pPlayer || !m_pLevel2PlayerTank || !m_pLevel2Terrain)
	{
		m_bPendingLevel2Reset = true;
		return;
	}

	const float fStartX = 0.0f;
	const float fStartZ = -280.0f;
	const float fStartAltitude = 3.0f;
	float fTerrainY = m_pLevel2Terrain->GetHeight(fStartX, fStartZ);
	m_xmf3Level2PlayerPosition = XMFLOAT3(fStartX, fTerrainY + fStartAltitude, fStartZ);
	m_fLevel2PlayerYaw = 0.0f;
	m_bLevel2MouseDragging = false;
	m_bPendingLevel2Reset = false;
	m_bLevel1Cleared = false;
	m_fLevel1ClearElapsedTime = 0.0f;
	m_bLevel1Failed = false;
	m_fLevel1FailedElapsedTime = 0.0f;

	m_pPlayer->ResetOrientation();
	m_pPlayer->SetPosition(m_xmf3Level2PlayerPosition);
	m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));

	UpdateLevel2PlayerTankTransform();
	UpdateLevel2Camera();
}
void CScene::ClampPlayerToTerrain()
{
	if (!m_pPlayer) return;

	if (m_nSceneMode != GAME_SCENE_LEVEL1) return;

	CTerrainObject *pCurrentTerrain = m_pTerrain;
	float fMinAltitude = 40.0f;
	if (!pCurrentTerrain) return;

	XMFLOAT3 xmf3PlayerPosition = m_pPlayer->GetPosition();
	float fTerrainY = pCurrentTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
	float fMinimumY = fTerrainY + fMinAltitude;
	if (xmf3PlayerPosition.y < fMinimumY)
	{
		xmf3PlayerPosition.y = fMinimumY;
		m_pPlayer->SetPosition(xmf3PlayerPosition);
	}
}


bool CScene::ProcessLevel2Input(UCHAR *pKeysBuffer)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL2) || !m_pLevel2PlayerTank || !m_pLevel2Terrain || !m_pPlayer) return(false);

	XMFLOAT3 xmf3MoveDirection = XMFLOAT3(0.0f, 0.0f, 0.0f);
	const float fLevel2MoveYawOffset = 180.0f;
	float fYawRadians = XMConvertToRadians(m_fLevel2PlayerYaw + fLevel2MoveYawOffset);
	XMFLOAT3 xmf3Forward = XMFLOAT3(sinf(fYawRadians), 0.0f, cosf(fYawRadians));
	XMFLOAT3 xmf3Right = XMFLOAT3(cosf(fYawRadians), 0.0f, -sinf(fYawRadians));

	if ((pKeysBuffer['W'] & 0xF0) || (pKeysBuffer[VK_UP] & 0xF0)) xmf3MoveDirection = Vector3::Add(xmf3MoveDirection, xmf3Forward);
	if ((pKeysBuffer['S'] & 0xF0) || (pKeysBuffer[VK_DOWN] & 0xF0)) xmf3MoveDirection = Vector3::Add(xmf3MoveDirection, xmf3Forward, -1.0f);
	if ((pKeysBuffer['D'] & 0xF0) || (pKeysBuffer[VK_RIGHT] & 0xF0)) xmf3MoveDirection = Vector3::Add(xmf3MoveDirection, xmf3Right);
	if ((pKeysBuffer['A'] & 0xF0) || (pKeysBuffer[VK_LEFT] & 0xF0)) xmf3MoveDirection = Vector3::Add(xmf3MoveDirection, xmf3Right, -1.0f);

	if (Vector3::Length(xmf3MoveDirection) > 0.001f)
	{
		const float fMoveSpeed = 42.0f;
		xmf3MoveDirection = Vector3::Normalize(xmf3MoveDirection);
		m_xmf3Level2PlayerPosition = Vector3::Add(m_xmf3Level2PlayerPosition, Vector3::ScalarProduct(xmf3MoveDirection, fMoveSpeed * m_fElapsedTime, false));
		m_xmf3Level2PlayerPosition.y = m_pLevel2Terrain->GetHeight(m_xmf3Level2PlayerPosition.x, m_xmf3Level2PlayerPosition.z) + 3.0f;
		UpdateLevel2PlayerTankTransform();
		UpdateLevel2Camera();
	}
	else
	{
		m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
	}

	return(true);
}

void CScene::RotateLevel2PlayerTank(float fYawDelta)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL2) || !m_pLevel2PlayerTank || !m_pPlayer) return;

	m_fLevel2PlayerYaw += fYawDelta;
	if (m_fLevel2PlayerYaw > 360.0f) m_fLevel2PlayerYaw -= 360.0f;
	if (m_fLevel2PlayerYaw < -360.0f) m_fLevel2PlayerYaw += 360.0f;
	UpdateLevel2PlayerTankTransform();
	UpdateLevel2Camera();
}

void CScene::UpdateLevel2PlayerTankTransform()
{
	if (!m_pLevel2PlayerTank || !m_pPlayer) return;

	m_pLevel2PlayerTank->m_xmf4x4Transform = Matrix4x4::Identity();
	m_pLevel2PlayerTank->SetScale(2.8f, 2.8f, 2.8f);
	m_pLevel2PlayerTank->Rotate(0.0f, m_fLevel2PlayerYaw, 0.0f);
	m_pLevel2PlayerTank->SetPosition(m_xmf3Level2PlayerPosition);

	m_pPlayer->ResetOrientation();
	m_pPlayer->Rotate(0.0f, m_fLevel2PlayerYaw, 0.0f);
	m_pPlayer->SetPosition(m_xmf3Level2PlayerPosition);
	m_pPlayer->SetVelocity(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void CScene::UpdateLevel2Camera()
{
	if (!m_pPlayer) return;

	CCamera *pLevel2Camera = m_pPlayer->ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
	if (pLevel2Camera)
	{
		float fYawRadians = XMConvertToRadians(m_fLevel2PlayerYaw);
		XMFLOAT3 xmf3Forward = XMFLOAT3(sinf(fYawRadians), 0.0f, cosf(fYawRadians));
		XMFLOAT3 xmf3CameraLocalOffset = XMFLOAT3(0.0f, 30.0f, 80.0f);
		XMFLOAT3 xmf3CameraWorldOffset = XMFLOAT3(xmf3Forward.x * 34.0f, 18.0f, xmf3Forward.z * 34.0f);
		XMFLOAT3 xmf3CameraPosition = Vector3::Add(m_xmf3Level2PlayerPosition, xmf3CameraWorldOffset);
		pLevel2Camera->SetTimeLag(0.0f);
		pLevel2Camera->SetOffset(xmf3CameraLocalOffset);
		pLevel2Camera->SetPosition(xmf3CameraPosition);
		pLevel2Camera->SetLookAt(m_xmf3Level2PlayerPosition);
		pLevel2Camera->RegenerateViewMatrix();
	}
}
void CScene::FirePlayerProjectile()
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_pPlayer || (m_fProjectileFireCooldown > 0.0f)) return;

	CProjectileObject *ppProjectilesToFire[2] = { NULL, NULL };
	for (int i = 0; i < m_nProjectiles; i++)
	{
		if (m_ppProjectiles[i] && !m_ppProjectiles[i]->IsActive())
		{
			if (!ppProjectilesToFire[0]) ppProjectilesToFire[0] = m_ppProjectiles[i];
			else
			{
				ppProjectilesToFire[1] = m_ppProjectiles[i];
				break;
			}
		}
	}
	if (!ppProjectilesToFire[0]) return;

	const float fMuzzleRightOffset = 1.6f;
	const float fMuzzleLookOffset = 3.0f;
	const float fMuzzleUpOffset = -0.75f;
	XMFLOAT3 xmf3Look = Vector3::Normalize(m_pPlayer->GetLookVector());
	XMFLOAT3 xmf3Right = Vector3::Normalize(m_pPlayer->GetRightVector());
	XMFLOAT3 xmf3Up = Vector3::Normalize(m_pPlayer->GetUpVector());
	const float fProjectileDownAimOffset = -0.3f;
	XMFLOAT3 xmf3FireDirection = Vector3::Normalize(Vector3::Add(xmf3Look, xmf3Up, fProjectileDownAimOffset));
	XMFLOAT3 xmf3BasePosition = m_pPlayer->GetPosition();

	m_pPlayer->OnPrepareRender();
	CGameObject *pMissileFrame = m_pPlayer->FindFrame("Hellfire_Missile");
	if (pMissileFrame) xmf3BasePosition = pMissileFrame->GetPosition();

	XMFLOAT3 xmf3LeftMuzzlePosition = Vector3::Add(Vector3::Add(Vector3::Add(xmf3BasePosition, xmf3Right, -fMuzzleRightOffset), xmf3Look, fMuzzleLookOffset), xmf3Up, fMuzzleUpOffset);
	XMFLOAT3 xmf3RightMuzzlePosition = Vector3::Add(Vector3::Add(Vector3::Add(xmf3BasePosition, xmf3Right, +fMuzzleRightOffset), xmf3Look, fMuzzleLookOffset), xmf3Up, fMuzzleUpOffset);

	ppProjectilesToFire[0]->Fire(xmf3LeftMuzzlePosition, xmf3FireDirection);
	if (ppProjectilesToFire[1]) ppProjectilesToFire[1]->Fire(xmf3RightMuzzlePosition, xmf3FireDirection);
	m_fProjectileFireCooldown = PROJECTILE_FIRE_COOLDOWN;
}
void CScene::FireLevel1EnemyProjectile(int nTargetIndex)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_pPlayer || !m_ppEnemyProjectiles || !m_pLevel1Targets) return;
	if ((nTargetIndex < 0) || (nTargetIndex >= m_nLevel1Targets)) return;

	SLevel1TargetState& target = m_pLevel1Targets[nTargetIndex];
	if (!target.m_bActive || target.m_bDestroying) return;
	if ((target.m_nObjectIndex < 0) || (target.m_nObjectIndex >= m_nGameObjects) || !m_ppGameObjects[target.m_nObjectIndex]) return;

	CProjectileObject *pEnemyProjectile = NULL;
	for (int i = 0; i < m_nEnemyProjectiles; i++)
	{
		if (m_ppEnemyProjectiles[i] && !m_ppEnemyProjectiles[i]->IsActive())
		{
			pEnemyProjectile = m_ppEnemyProjectiles[i];
			break;
		}
	}
	if (!pEnemyProjectile) return;

	XMFLOAT3 xmf3EnemyPosition = m_ppGameObjects[target.m_nObjectIndex]->GetPosition();
	XMFLOAT3 xmf3PlayerPosition = m_pPlayer->GetPosition();
	XMFLOAT3 xmf3Direction = Vector3::Subtract(xmf3PlayerPosition, xmf3EnemyPosition);
	if (Vector3::Length(xmf3Direction) <= 0.001f) return;
	xmf3Direction = Vector3::Normalize(xmf3Direction);

	const float fEnemyMuzzleForwardOffset = 5.0f;
	XMFLOAT3 xmf3StartPosition = Vector3::Add(xmf3EnemyPosition, xmf3Direction, fEnemyMuzzleForwardOffset);
	pEnemyProjectile->m_nDamage = target.m_nProjectileDamage;
	pEnemyProjectile->Fire(xmf3StartPosition, xmf3Direction);
}
void CScene::InitializeLevel1Targets()
{
	if (m_pLevel1Targets) delete[] m_pLevel1Targets;

	m_nLevel1Targets = 4;
	m_pLevel1Targets = new SLevel1TargetState[m_nLevel1Targets];
	m_nCurrentLevel1Wave = 1;

	const int nApache1Index = WORLD_OBJECT_START + 0;
	const int nApache2Index = WORLD_OBJECT_START + 1;
	const int nSuperCobraIndex = WORLD_OBJECT_START + 2;
	const int nMi24Index = WORLD_OBJECT_START + 3;

	const XMFLOAT3 xmf3StartPositions[4] =
	{
		XMFLOAT3(-120.0f, 80.0f, 200.0f),
		XMFLOAT3(+120.0f, 80.0f, 220.0f),
		XMFLOAT3(0.0f, 300.0f, 300.0f),
		XMFLOAT3(0.0f, 110.0f, -200.0f)
	};
	const int pnObjectIndices[4] = { nApache1Index, nApache2Index, nSuperCobraIndex, nMi24Index };
	const int pnWaves[4] = { 1, 1, 2, 3 };
	const int pnMaxHPs[4] = { 18, 18, 30, 45 };
	const float pfCollisionRadii[4] = { 9.0f, 9.0f, 8.0f, 9.0f };
	const float pfMoveSpeeds[4] = { 1.1f, 1.0f, 1.35f, 0.65f };
	const float pfMoveRadii[4] = { 22.0f, 24.0f, 28.0f, 18.0f };
	const float pfMovePhases[4] = { 0.0f, 1.7f, 2.8f, 4.1f };
	const float pfMoveRadiusX[4] = { 28.0f, 30.0f, 36.0f, 30.0f };
	const float pfMoveRadiusZ[4] = { 10.0f, 12.0f, 18.0f, 24.0f };
	const float pfBobAmplitudes[4] = { 4.0f, 5.0f, 6.0f, 5.0f };
	const float pfBobSpeeds[4] = { 1.2f, 1.35f, 1.45f, 1.1f };
	const float pfFireIntervals[4] = { 2.10f, 2.20f, 1.50f, 1.15f };
	const int pnProjectileDamages[4] = { 5, 5, 8, 10 };

	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		XMFLOAT3 xmf3StartPosition = xmf3StartPositions[i];
		float fTerrainY = (m_pTerrain) ? m_pTerrain->GetHeight(xmf3StartPosition.x, xmf3StartPosition.z) : 0.0f;
		xmf3StartPosition.y += fTerrainY;

		m_pLevel1Targets[i].m_nObjectIndex = pnObjectIndices[i];
		m_pLevel1Targets[i].m_nWave = pnWaves[i];
		m_pLevel1Targets[i].m_nMaxHP = pnMaxHPs[i];
		m_pLevel1Targets[i].m_nHP = pnMaxHPs[i];
		m_pLevel1Targets[i].m_bActive = false;
		m_pLevel1Targets[i].m_bDestroying = false;
		m_pLevel1Targets[i].m_fDestroyElapsedTime = 0.0f;
		m_pLevel1Targets[i].m_fCollisionRadius = pfCollisionRadii[i];
		m_pLevel1Targets[i].m_xmf3StartPosition = xmf3StartPosition;
		m_pLevel1Targets[i].m_xmf3BasePosition = xmf3StartPosition;
		m_pLevel1Targets[i].m_fMoveAngle = 0.0f;
		m_pLevel1Targets[i].m_fMoveSpeed = pfMoveSpeeds[i];
		m_pLevel1Targets[i].m_fMoveRadius = pfMoveRadii[i];
		m_pLevel1Targets[i].m_fMovePhase = pfMovePhases[i];
		m_pLevel1Targets[i].m_fMoveRadiusX = pfMoveRadiusX[i];
		m_pLevel1Targets[i].m_fMoveRadiusZ = pfMoveRadiusZ[i];
		m_pLevel1Targets[i].m_fBobAmplitude = pfBobAmplitudes[i];
		m_pLevel1Targets[i].m_fBobSpeed = pfBobSpeeds[i];
		m_pLevel1Targets[i].m_fFireInterval = pfFireIntervals[i];
		m_pLevel1Targets[i].m_fFireCooldown = pfFireIntervals[i] * (0.55f + 0.12f * (float)i);
		m_pLevel1Targets[i].m_nProjectileDamage = pnProjectileDamages[i];

		if (m_ppGameObjects[pnObjectIndices[i]])
		{
			m_ppGameObjects[pnObjectIndices[i]]->SetPosition(xmf3StartPosition);
			m_pLevel1Targets[i].m_xmf4x4StartTransform = m_ppGameObjects[pnObjectIndices[i]]->m_xmf4x4Transform;
		}
	}
}

void CScene::ResetLevel1Targets()
{
	m_nCurrentLevel1Wave = 1;
	m_bLevel1Cleared = false;
	m_fLevel1ClearElapsedTime = 0.0f;
	m_bLevel1Failed = false;
	m_fLevel1FailedElapsedTime = 0.0f;
	if (!m_pLevel1Targets) return;

	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		SLevel1TargetState& target = m_pLevel1Targets[i];
		target.m_nHP = target.m_nMaxHP;
		target.m_bActive = false;
		target.m_bDestroying = false;
		target.m_fDestroyElapsedTime = 0.0f;
		target.m_fMoveAngle = 0.0f;
		target.m_fFireCooldown = target.m_fFireInterval * (0.55f + 0.12f * (float)i);
		target.m_xmf3BasePosition = target.m_xmf3StartPosition;
		if ((target.m_nObjectIndex >= 0) && (target.m_nObjectIndex < m_nGameObjects) && m_ppGameObjects[target.m_nObjectIndex])
		{
			m_ppGameObjects[target.m_nObjectIndex]->m_xmf4x4Transform = target.m_xmf4x4StartTransform;
			m_ppGameObjects[target.m_nObjectIndex]->SetPosition(target.m_xmf3StartPosition);
		}
	}
	ActivateLevel1Wave(1);
}

void CScene::ActivateLevel1Wave(int nWave)
{
	if (!m_pLevel1Targets) return;

	m_nCurrentLevel1Wave = nWave;
	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		SLevel1TargetState& target = m_pLevel1Targets[i];
		bool bWaveTarget = (target.m_nWave == nWave);
		target.m_bActive = bWaveTarget;
		target.m_bDestroying = false;
		target.m_fDestroyElapsedTime = 0.0f;
		if (bWaveTarget)
		{
			target.m_nHP = target.m_nMaxHP;
			target.m_fMoveAngle = 0.0f;
			target.m_fFireCooldown = target.m_fFireInterval * (0.55f + 0.12f * (float)i);
			target.m_xmf3BasePosition = target.m_xmf3StartPosition;
			if ((target.m_nObjectIndex >= 0) && (target.m_nObjectIndex < m_nGameObjects) && m_ppGameObjects[target.m_nObjectIndex])
			{
				m_ppGameObjects[target.m_nObjectIndex]->m_xmf4x4Transform = target.m_xmf4x4StartTransform;
				m_ppGameObjects[target.m_nObjectIndex]->SetPosition(target.m_xmf3StartPosition);
				OrientLevel1TargetToPlayer(i);
			}
		}
	}
}

void CScene::OrientLevel1TargetToPlayer(int nTargetIndex)
{
	if (!m_pPlayer || !m_pLevel1Targets) return;
	if ((nTargetIndex < 0) || (nTargetIndex >= m_nLevel1Targets)) return;

	SLevel1TargetState& target = m_pLevel1Targets[nTargetIndex];
	if (!target.m_bActive || target.m_bDestroying) return;
	if ((target.m_nObjectIndex < 0) || (target.m_nObjectIndex >= m_nGameObjects) || !m_ppGameObjects[target.m_nObjectIndex]) return;

	CGameObject *pTargetObject = m_ppGameObjects[target.m_nObjectIndex];
	XMFLOAT3 xmf3EnemyPosition = pTargetObject->GetPosition();
	XMFLOAT3 xmf3PlayerPosition = m_pPlayer->GetPosition();
	XMFLOAT3 xmf3ToPlayer = Vector3::Subtract(xmf3PlayerPosition, xmf3EnemyPosition);
	xmf3ToPlayer.y = 0.0f;
	if (Vector3::Length(xmf3ToPlayer) <= 0.001f) return;

	XMFLOAT3 xmf3Look = Vector3::Normalize(xmf3ToPlayer);
	// If enemy helicopters face sideways, adjust this value by +/-90 degrees.
	const float fEnemyModelYawOffset = 0.0f;
	float fYaw = XMConvertToDegrees(atan2f(xmf3Look.x, xmf3Look.z));
	float fFinalYaw = fYaw + fEnemyModelYawOffset;

	pTargetObject->m_xmf4x4Transform = Matrix4x4::Identity();
	pTargetObject->SetScale(1.0f, 1.0f, 1.0f);
	pTargetObject->Rotate(0.0f, fFinalYaw, 0.0f);
	pTargetObject->SetPosition(xmf3EnemyPosition);
}
void CScene::UpdateLevel1Targets(float fTimeElapsed)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_pLevel1Targets) return;

	const float fDestroyDuration = 0.45f;
	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		SLevel1TargetState& target = m_pLevel1Targets[i];
		if (!target.m_bActive) continue;
		if ((target.m_nObjectIndex < 0) || (target.m_nObjectIndex >= m_nGameObjects)) continue;

		CGameObject *pTargetObject = m_ppGameObjects[target.m_nObjectIndex];
		if (!pTargetObject) continue;

		if (target.m_bDestroying)
		{
			target.m_fDestroyElapsedTime += fTimeElapsed;
			float fDestroyRatio = target.m_fDestroyElapsedTime / fDestroyDuration;
			if (fDestroyRatio > 1.0f) fDestroyRatio = 1.0f;

			XMFLOAT3 xmf3DestroyPosition = target.m_xmf3BasePosition;
			xmf3DestroyPosition.y += fDestroyRatio * 8.0f;
			float fDestroyScale = 1.0f + (fDestroyRatio * 0.8f);

			pTargetObject->m_xmf4x4Transform = target.m_xmf4x4StartTransform;
			pTargetObject->SetPosition(xmf3DestroyPosition);
			pTargetObject->SetScale(fDestroyScale, fDestroyScale, fDestroyScale);
			pTargetObject->Rotate(0.0f, 720.0f * target.m_fDestroyElapsedTime, 0.0f);

			if (target.m_fDestroyElapsedTime >= fDestroyDuration)
			{
				target.m_bActive = false;
				target.m_bDestroying = false;
				target.m_fDestroyElapsedTime = 0.0f;
				pTargetObject->m_xmf4x4Transform = target.m_xmf4x4StartTransform;
				pTargetObject->SetPosition(target.m_xmf3StartPosition);
			}
			continue;
		}

		target.m_fMoveAngle += target.m_fMoveSpeed * fTimeElapsed;
		XMFLOAT3 xmf3Position = target.m_xmf3BasePosition;
		xmf3Position.x = target.m_xmf3BasePosition.x + sinf(target.m_fMoveAngle + target.m_fMovePhase) * target.m_fMoveRadiusX;
		xmf3Position.z = target.m_xmf3BasePosition.z + cosf((target.m_fMoveAngle * 0.7f) + target.m_fMovePhase) * target.m_fMoveRadiusZ;
		xmf3Position.y = target.m_xmf3BasePosition.y + sinf((target.m_fMoveAngle * target.m_fBobSpeed) + target.m_fMovePhase) * target.m_fBobAmplitude;

		const float fMinEnemyAltitude = 55.0f;
		float fTerrainY = (m_pTerrain) ? m_pTerrain->GetHeight(xmf3Position.x, xmf3Position.z) : 0.0f;
		if (xmf3Position.y < fTerrainY + fMinEnemyAltitude) xmf3Position.y = fTerrainY + fMinEnemyAltitude;

		pTargetObject->SetPosition(xmf3Position);
		OrientLevel1TargetToPlayer(i);
	}
}

void CScene::UpdateLevel1EnemyFire(float fTimeElapsed)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_pLevel1Targets || !m_pPlayer) return;

	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		SLevel1TargetState& target = m_pLevel1Targets[i];
		if (!target.m_bActive || target.m_bDestroying) continue;

		target.m_fFireCooldown -= fTimeElapsed;
		if (target.m_fFireCooldown <= 0.0f)
		{
			FireLevel1EnemyProjectile(i);
			target.m_fFireCooldown = target.m_fFireInterval;
		}
	}
}

void CScene::ResetEnemyProjectiles()
{
	for (int i = 0; i < m_nEnemyProjectiles; i++) if (m_ppEnemyProjectiles[i]) m_ppEnemyProjectiles[i]->Reset();
}

void CScene::ApplyDamageToPlayer(int nDamage)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed) return;

	SpawnPlayerHitEffect(m_pPlayer ? m_pPlayer->GetPosition() : XMFLOAT3(0.0f, 0.0f, 0.0f));
	m_nPlayerHP -= nDamage;
	if (m_nPlayerHP < 0) m_nPlayerHP = 0;

	char pstrDebug[128];
	sprintf_s(pstrDebug, "[Level1] Player hit: HP=%d\n", m_nPlayerHP);
	::OutputDebugStringA(pstrDebug);

	if (m_nPlayerHP <= 0)
	{
		m_bLevel1Failed = true;
		m_fLevel1FailedElapsedTime = 0.0f;
		if (m_ppGameObjects && m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]) m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->m_xmf4x4Transform = m_xmf4x4Level1GameOverBaseTransform;
		ResetEnemyProjectiles();
		ResetLevel1Effects();
		::OutputDebugStringA("[Level1] Mission Failed\n");
	}
}

void CScene::CheckEnemyProjectilePlayerCollisions()
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_pPlayer || !m_ppEnemyProjectiles) return;

	const float fPlayerHitRadius = 7.0f;
	XMFLOAT3 xmf3PlayerPosition = m_pPlayer->GetPosition();
	for (int i = 0; i < m_nEnemyProjectiles; i++)
	{
		CProjectileObject *pEnemyProjectile = m_ppEnemyProjectiles[i];
		if (!pEnemyProjectile || !pEnemyProjectile->IsActive()) continue;

		XMFLOAT3 xmf3Difference = Vector3::Subtract(pEnemyProjectile->GetPosition(), xmf3PlayerPosition);
		float fDistance = Vector3::Length(xmf3Difference);
		float fHitDistance = pEnemyProjectile->GetCollisionRadius() + fPlayerHitRadius;
		if (fDistance <= fHitDistance)
		{
			int nDamage = pEnemyProjectile->GetDamage();
			pEnemyProjectile->Reset();
			ApplyDamageToPlayer(nDamage);
		}
	}
}
void CScene::UpdateLevel1ClearText()
{
	if (!m_bLevel1Cleared) return;
	if (!m_ppGameObjects || !m_ppGameObjects[LEVEL1_CLEAR_OBJECT]) return;

	const float fClearBaseY = 0.0f;
	const float fClearBobSpeed = 2.5f;
	const float fClearBobAmplitude = 5.0f;
	float fClearY = fClearBaseY + sinf(m_fLevel1ClearElapsedTime * fClearBobSpeed) * fClearBobAmplitude;

	CGameObject *pClearObject = m_ppGameObjects[LEVEL1_CLEAR_OBJECT];
	pClearObject->m_xmf4x4Transform = m_xmf4x4Level1ClearBaseTransform;
	pClearObject->SetPosition(XMFLOAT3(0.0f, fClearY, 0.0f));
}

void CScene::UpdateLevel1GameOverText()
{
	if (!m_bLevel1Failed) return;
	if (!m_ppGameObjects || !m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]) return;

	const float fGameOverBaseX = 80.0f;
	const float fGameOverBaseY = 0.0f;
	const float fGameOverBobSpeed = 2.5f;
	const float fGameOverBobAmplitude = 5.0f;
	float fGameOverY = fGameOverBaseY + sinf(m_fLevel1FailedElapsedTime * fGameOverBobSpeed) * fGameOverBobAmplitude;

	CGameObject *pGameOverObject = m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT];
	pGameOverObject->m_xmf4x4Transform = m_xmf4x4Level1GameOverBaseTransform;
	pGameOverObject->SetPosition(XMFLOAT3(fGameOverBaseX, fGameOverY, 0.0f));
}
void CScene::BuildLevel1Decorations(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	ReleaseLevel1Decorations();

	const int nBaseTreeCount = 20;
	const int nBaseRockCount = 10;
	const int nTreeCount = nBaseTreeCount * 3;
	const int nRockCount = nBaseRockCount * 3;
	const int nHelipadCount = 1;
	const int nHelipadMarkCount = 3;
	m_nLevel1Decorations = nTreeCount + nRockCount + nHelipadCount + nHelipadMarkCount;
	m_ppLevel1Decorations = new CGameObject*[m_nLevel1Decorations];
	for (int i = 0; i < m_nLevel1Decorations; i++) m_ppLevel1Decorations[i] = NULL;

	FILE *pTreeFile = NULL;
	bool bUseTreeModel = (fopen_s(&pTreeFile, "Model/Tree.bin", "rb") == 0);
	if (pTreeFile) fclose(pTreeFile);
	FILE *pRockFile = NULL;
	bool bUseRockModel = (fopen_s(&pRockFile, "Model/Rock.bin", "rb") == 0);
	if (pRockFile) fclose(pRockFile);
	if (!bUseTreeModel || !bUseRockModel)
	{
		::OutputDebugStringA("[Level1] Decoration model load skipped: Model/Tree.bin or Model/Rock.bin is missing.\n");
		ReleaseLevel1Decorations();
		return;
	}

	const float fTreeYOffset = 0.0f;
	const float fRockYOffset = 0.0f;
	const XMFLOAT3 xmf3BaseTreePositions[nBaseTreeCount] =
	{
		XMFLOAT3(-360.0f, 0.0f, -40.0f), XMFLOAT3(-330.0f, 0.0f, 120.0f), XMFLOAT3(-300.0f, 0.0f, 310.0f), XMFLOAT3(-260.0f, 0.0f, 470.0f),
		XMFLOAT3(-210.0f, 0.0f, 650.0f), XMFLOAT3(-170.0f, 0.0f, -230.0f), XMFLOAT3(-110.0f, 0.0f, 520.0f), XMFLOAT3(-70.0f, 0.0f, 760.0f),
		XMFLOAT3(95.0f, 0.0f, 700.0f), XMFLOAT3(145.0f, 0.0f, 510.0f), XMFLOAT3(190.0f, 0.0f, 320.0f), XMFLOAT3(230.0f, 0.0f, 90.0f),
		XMFLOAT3(270.0f, 0.0f, -160.0f), XMFLOAT3(315.0f, 0.0f, 210.0f), XMFLOAT3(350.0f, 0.0f, 430.0f), XMFLOAT3(390.0f, 0.0f, 620.0f),
		XMFLOAT3(-420.0f, 0.0f, 610.0f), XMFLOAT3(430.0f, 0.0f, 30.0f), XMFLOAT3(-250.0f, 0.0f, -360.0f), XMFLOAT3(255.0f, 0.0f, -330.0f)
	};
	const XMFLOAT2 xmf2TreeVariationOffsets[3] = { XMFLOAT2(0.0f, 0.0f), XMFLOAT2(42.0f, 58.0f), XMFLOAT2(-55.0f, -38.0f) };
	const float pfTreeVariationScale[3] = { 0.82f, 1.08f, 1.32f };

	int nDecorationIndex = 0;
	int pnDecorationMaterials[1] = { 1 };
	for (int i = 0; i < nBaseTreeCount; i++)
	{
		for (int j = 0; j < 3; j++, nDecorationIndex++)
		{
			float x = xmf3BaseTreePositions[i].x + xmf2TreeVariationOffsets[j].x + ((i % 3) - 1) * 11.0f;
			float z = xmf3BaseTreePositions[i].z + xmf2TreeVariationOffsets[j].y + ((i % 4) - 1.5f) * 13.0f;
			float y = (m_pTerrain) ? m_pTerrain->GetHeight(x, z) : 0.0f;
			float fTreeScale = pfTreeVariationScale[j] + (float)(i % 5) * 0.04f;
			float fTreeYaw = fmodf((float)(i * 37 + j * 109), 360.0f);

			int nMeshesInHierarchy = 0;
			int pnMaterialsInHierarchy[64]{};
			CGameObject *pTreeModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Tree.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);
			CGameObject *pTreeObject = new CGameObject();
			pTreeObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
			pTreeObject->SetChild(pTreeModel, true);
			pTreeObject->SetScale(fTreeScale, fTreeScale, fTreeScale);
			pTreeObject->Rotate(0.0f, fTreeYaw, 0.0f);
			pTreeObject->SetPosition(XMFLOAT3(x, y + fTreeYOffset, z));
			m_ppLevel1Decorations[nDecorationIndex] = pTreeObject;
		}
	}

	const XMFLOAT3 xmf3BaseRockPositions[nBaseRockCount] =
	{
		XMFLOAT3(-210.0f, 0.0f, -80.0f), XMFLOAT3(-150.0f, 0.0f, 160.0f), XMFLOAT3(-95.0f, 0.0f, 360.0f), XMFLOAT3(-30.0f, 0.0f, 610.0f), XMFLOAT3(70.0f, 0.0f, 240.0f),
		XMFLOAT3(135.0f, 0.0f, -30.0f), XMFLOAT3(210.0f, 0.0f, 460.0f), XMFLOAT3(300.0f, 0.0f, 150.0f), XMFLOAT3(-320.0f, 0.0f, 420.0f), XMFLOAT3(360.0f, 0.0f, -250.0f)
	};
	const XMFLOAT2 xmf2RockVariationOffsets[3] = { XMFLOAT2(0.0f, 0.0f), XMFLOAT2(32.0f, -46.0f), XMFLOAT2(-38.0f, 44.0f) };
	const float pfRockVariationScale[3] = { 0.68f, 1.04f, 1.42f };
	for (int i = 0; i < nBaseRockCount; i++)
	{
		for (int j = 0; j < 3; j++, nDecorationIndex++)
		{
			float x = xmf3BaseRockPositions[i].x + xmf2RockVariationOffsets[j].x + ((i % 2) ? 12.0f : -12.0f);
			float z = xmf3BaseRockPositions[i].z + xmf2RockVariationOffsets[j].y + ((i % 3) - 1) * 9.0f;
			float y = (m_pTerrain) ? m_pTerrain->GetHeight(x, z) : 0.0f;
			float fRockScale = pfRockVariationScale[j] + (float)(i % 4) * 0.05f;
			float fRockYaw = fmodf((float)(i * 53 + j * 97 + 17), 360.0f);

			int nMeshesInHierarchy = 0;
			int pnMaterialsInHierarchy[64]{};
			CGameObject *pRockModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Rock.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);
			CGameObject *pRockObject = new CGameObject();
			pRockObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
			pRockObject->SetChild(pRockModel, true);
			pRockObject->SetScale(fRockScale, fRockScale, fRockScale);
			pRockObject->Rotate(0.0f, fRockYaw, 0.0f);
			pRockObject->SetPosition(XMFLOAT3(x, y + fRockYOffset, z));
			m_ppLevel1Decorations[nDecorationIndex] = pRockObject;
		}
	}

	const float fHelipadX = 0.0f;
	const float fHelipadZ = -120.0f;
	const XMFLOAT3 xmf3HelipadScale = XMFLOAT3(52.0f, 0.5f, 52.0f);
	const float fHelipadHalfX = xmf3HelipadScale.x * 0.5f;
	const float fHelipadHalfZ = xmf3HelipadScale.z * 0.5f;
	const XMFLOAT2 xmf2HelipadSamples[9] =
	{
		XMFLOAT2(0.0f, 0.0f), XMFLOAT2(-fHelipadHalfX, -fHelipadHalfZ), XMFLOAT2(fHelipadHalfX, -fHelipadHalfZ),
		XMFLOAT2(-fHelipadHalfX, fHelipadHalfZ), XMFLOAT2(fHelipadHalfX, fHelipadHalfZ), XMFLOAT2(0.0f, -fHelipadHalfZ),
		XMFLOAT2(0.0f, fHelipadHalfZ), XMFLOAT2(-fHelipadHalfX, 0.0f), XMFLOAT2(fHelipadHalfX, 0.0f)
	};
	float fMaxHelipadTerrainY = (m_pTerrain) ? m_pTerrain->GetHeight(fHelipadX, fHelipadZ) : 0.0f;
	if (m_pTerrain)
	{
		for (int i = 1; i < _countof(xmf2HelipadSamples); i++)
		{
			float fSampleHeight = m_pTerrain->GetHeight(fHelipadX + xmf2HelipadSamples[i].x, fHelipadZ + xmf2HelipadSamples[i].y);
			if (fSampleHeight > fMaxHelipadTerrainY) fMaxHelipadTerrainY = fSampleHeight;
		}
	}
	const float fHelipadGroundClearance = 1.0f;
	float fHelipadCenterY = fMaxHelipadTerrainY + xmf3HelipadScale.y * 0.5f + fHelipadGroundClearance;
	float fHelipadTopY = fHelipadCenterY + xmf3HelipadScale.y * 0.5f + 0.15f;
	CDecorationBoxObject *pHelipadObject = new CDecorationBoxObject(pd3dDevice, pd3dCommandList, XMFLOAT4(0.85f, 0.85f, 0.82f, 1.0f), XMFLOAT4(0.95f, 0.95f, 0.90f, 1.0f), XMFLOAT4(0.08f, 0.08f, 0.06f, 1.0f));
	pHelipadObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnDecorationMaterials);
	pHelipadObject->SetScale(xmf3HelipadScale.x, xmf3HelipadScale.y, xmf3HelipadScale.z);
	pHelipadObject->SetPosition(XMFLOAT3(fHelipadX, fHelipadCenterY, fHelipadZ));
	m_ppLevel1Decorations[nDecorationIndex++] = pHelipadObject;

	const XMFLOAT3 xmf3HScales[nHelipadMarkCount] = { XMFLOAT3(4.0f, 0.25f, 26.0f), XMFLOAT3(4.0f, 0.25f, 26.0f), XMFLOAT3(28.0f, 0.25f, 4.0f) };
	const XMFLOAT3 xmf3HPositions[nHelipadMarkCount] = { XMFLOAT3(-12.0f, 0.0f, 0.0f), XMFLOAT3(12.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) };
	for (int i = 0; i < nHelipadMarkCount; i++, nDecorationIndex++)
	{
		CDecorationBoxObject *pHMarkObject = new CDecorationBoxObject(pd3dDevice, pd3dCommandList, XMFLOAT4(0.85f, 0.68f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.82f, 0.0f, 1.0f), XMFLOAT4(0.25f, 0.18f, 0.0f, 1.0f));
		pHMarkObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnDecorationMaterials);
		pHMarkObject->SetScale(xmf3HScales[i].x, xmf3HScales[i].y, xmf3HScales[i].z);
		pHMarkObject->SetPosition(XMFLOAT3(fHelipadX + xmf3HPositions[i].x, fHelipadTopY + xmf3HScales[i].y * 0.5f, fHelipadZ + xmf3HPositions[i].z));
		m_ppLevel1Decorations[nDecorationIndex] = pHMarkObject;
	}
}
void CScene::RenderLevel1Decorations(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_ppLevel1Decorations) return;
	for (int i = 0; i < m_nLevel1Decorations; i++)
	{
		if (m_ppLevel1Decorations[i])
		{
			m_ppLevel1Decorations[i]->UpdateTransform(NULL);
			m_ppLevel1Decorations[i]->Render(pd3dCommandList, pCamera, m_ppLevel1Decorations[i]->m_ppd3dcbInstancingGameObjects, m_ppLevel1Decorations[i]->m_ppcbMappedInstancingGameObjects);
		}
	}
}

void CScene::ReleaseLevel1Decorations()
{
	if (m_ppLevel1Decorations)
	{
		for (int i = 0; i < m_nLevel1Decorations; i++) if (m_ppLevel1Decorations[i]) m_ppLevel1Decorations[i]->Release();
		delete[] m_ppLevel1Decorations;
		m_ppLevel1Decorations = NULL;
	}
	m_nLevel1Decorations = 0;
}
void CScene::BuildLevel2Objects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	ReleaseLevel2Objects();

	int pnTerrainMaterials[1] = { 1 };
	m_pLevel2Terrain = new CTerrainObject(pd3dDevice, pd3dCommandList, "HeightMap/Level_2_terrain.raw");
	m_pLevel2Terrain->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnTerrainMaterials);

	FILE *pTankFile = NULL;
	bool bUseTankModel = (fopen_s(&pTankFile, "Model/M26.bin", "rb") == 0);
	if (pTankFile) fclose(pTankFile);
	FILE *pTreeFile = NULL;
	bool bUseTreeModel = (fopen_s(&pTreeFile, "Model/Tree.bin", "rb") == 0);
	if (pTreeFile) fclose(pTreeFile);
	FILE *pRockFile = NULL;
	bool bUseRockModel = (fopen_s(&pRockFile, "Model/Rock.bin", "rb") == 0);
	if (pRockFile) fclose(pRockFile);
	if (!bUseTankModel || !bUseTreeModel || !bUseRockModel)
	{
		ReleaseLevel2Objects();
		return;
	}

	int nPlayerMeshesInHierarchy = 0;
	int pnPlayerMaterialsInHierarchy[64]{};
	CGameObject *pPlayerTankModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/M26.bin", &nPlayerMeshesInHierarchy, pnPlayerMaterialsInHierarchy);
	CM26Object *pPlayerTankObject = new CM26Object();
	pPlayerTankObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nPlayerMeshesInHierarchy, pnPlayerMaterialsInHierarchy);
	pPlayerTankObject->SetChild(pPlayerTankModel, true);
	pPlayerTankObject->OnInitialize();
	m_pLevel2PlayerTank = pPlayerTankObject;
	m_nLevel2EnemyTanks = LEVEL2_ENEMY_TANK_COUNT;
	m_ppLevel2EnemyTanks = new CTankObject*[m_nLevel2EnemyTanks];
	for (int i = 0; i < m_nLevel2EnemyTanks; i++) m_ppLevel2EnemyTanks[i] = NULL;

	const XMFLOAT3 xmf3TankPositions[LEVEL2_ENEMY_TANK_COUNT] =
	{
		XMFLOAT3(-360.0f, 0.0f, 120.0f), XMFLOAT3(-250.0f, 0.0f, 360.0f), XMFLOAT3(-120.0f, 0.0f, 620.0f), XMFLOAT3(80.0f, 0.0f, 520.0f), XMFLOAT3(280.0f, 0.0f, 310.0f),
		XMFLOAT3(390.0f, 0.0f, 40.0f), XMFLOAT3(220.0f, 0.0f, -210.0f), XMFLOAT3(20.0f, 0.0f, -420.0f), XMFLOAT3(-190.0f, 0.0f, -260.0f), XMFLOAT3(-410.0f, 0.0f, -80.0f)
	};
	for (int i = 0; i < m_nLevel2EnemyTanks; i++)
	{
		int nMeshesInHierarchy = 0;
		int pnMaterialsInHierarchy[64]{};
		CGameObject *pTankModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/M26.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);
		CM26Object *pTankObject = new CM26Object();
		pTankObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
		pTankObject->SetChild(pTankModel, true);
		pTankObject->OnInitialize();
		pTankObject->SetScale(2.6f, 2.6f, 2.6f);
		pTankObject->Rotate(0.0f, (float)(i * 31), 0.0f);
		float fTankY = (m_pLevel2Terrain) ? m_pLevel2Terrain->GetHeight(xmf3TankPositions[i].x, xmf3TankPositions[i].z) : 0.0f;
		pTankObject->SetPosition(XMFLOAT3(xmf3TankPositions[i].x, fTankY + 2.0f, xmf3TankPositions[i].z));
		m_ppLevel2EnemyTanks[i] = pTankObject;
	}

	m_nLevel2Obstacles = LEVEL2_OBSTACLE_COUNT;
	m_ppLevel2Obstacles = new CGameObject*[m_nLevel2Obstacles];
	for (int i = 0; i < m_nLevel2Obstacles; i++) m_ppLevel2Obstacles[i] = NULL;

	for (int i = 0; i < m_nLevel2Obstacles; i++)
	{
		bool bTree = ((i % 3) != 0);
		float fColumn = (float)(i % 10);
		float fRow = (float)(i / 10);
		float x = -430.0f + fColumn * 95.0f + (float)((i * 17) % 31 - 15);
		float z = -470.0f + fRow * 190.0f + (float)((i * 23) % 47 - 23);
		float y = (m_pLevel2Terrain) ? m_pLevel2Terrain->GetHeight(x, z) : 0.0f;
		float fScale = bTree ? (0.78f + (float)(i % 5) * 0.08f) : (0.72f + (float)(i % 4) * 0.10f);
		float fYaw = fmodf((float)(i * 43), 360.0f);

		int nMeshesInHierarchy = 0;
		int pnMaterialsInHierarchy[64]{};
		CGameObject *pObstacleModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, bTree ? "Model/Tree.bin" : "Model/Rock.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);
		CGameObject *pObstacleObject = new CGameObject();
		pObstacleObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
		pObstacleObject->SetChild(pObstacleModel, true);
		pObstacleObject->SetScale(fScale, fScale, fScale);
		pObstacleObject->Rotate(0.0f, fYaw, 0.0f);
		pObstacleObject->SetPosition(XMFLOAT3(x, y, z));
		m_ppLevel2Obstacles[i] = pObstacleObject;
	}
}

void CScene::UpdateLevel2Objects(float fTimeElapsed)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL2) || !m_pPlayer || !m_ppLevel2EnemyTanks) return;

	XMFLOAT3 xmf3PlayerPosition = m_pPlayer->GetPosition();
	for (int i = 0; i < m_nLevel2EnemyTanks; i++)
	{
		if (m_ppLevel2EnemyTanks[i]) m_ppLevel2EnemyTanks[i]->AimTurretAt(xmf3PlayerPosition);
	}
}

void CScene::RenderLevel2Objects(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if (m_nSceneMode != GAME_SCENE_LEVEL2) return;

	if (m_pLevel2Terrain)
	{
		m_pLevel2Terrain->UpdateTransform(NULL);
		m_pLevel2Terrain->Render(pd3dCommandList, pCamera, m_pLevel2Terrain->m_ppd3dcbInstancingGameObjects, m_pLevel2Terrain->m_ppcbMappedInstancingGameObjects);
	}
	if (m_pLevel2PlayerTank)
	{
		m_pLevel2PlayerTank->UpdateTransform(NULL);
		m_pLevel2PlayerTank->Render(pd3dCommandList, pCamera, m_pLevel2PlayerTank->m_ppd3dcbInstancingGameObjects, m_pLevel2PlayerTank->m_ppcbMappedInstancingGameObjects);
	}
	for (int i = 0; i < m_nLevel2Obstacles; i++)
	{
		if (m_ppLevel2Obstacles && m_ppLevel2Obstacles[i])
		{
			m_ppLevel2Obstacles[i]->UpdateTransform(NULL);
			m_ppLevel2Obstacles[i]->Render(pd3dCommandList, pCamera, m_ppLevel2Obstacles[i]->m_ppd3dcbInstancingGameObjects, m_ppLevel2Obstacles[i]->m_ppcbMappedInstancingGameObjects);
		}
	}
	for (int i = 0; i < m_nLevel2EnemyTanks; i++)
	{
		if (m_ppLevel2EnemyTanks && m_ppLevel2EnemyTanks[i])
		{
			m_ppLevel2EnemyTanks[i]->UpdateTransform(NULL);
			m_ppLevel2EnemyTanks[i]->Render(pd3dCommandList, pCamera, m_ppLevel2EnemyTanks[i]->m_ppd3dcbInstancingGameObjects, m_ppLevel2EnemyTanks[i]->m_ppcbMappedInstancingGameObjects);
		}
	}
}

void CScene::ReleaseLevel2Objects()
{
	if (m_pLevel2Terrain)
	{
		m_pLevel2Terrain->Release();
		m_pLevel2Terrain = NULL;
	}
	if (m_pLevel2PlayerTank)
	{
		m_pLevel2PlayerTank->Release();
		m_pLevel2PlayerTank = NULL;
	}
	if (m_ppLevel2EnemyTanks)
	{
		for (int i = 0; i < m_nLevel2EnemyTanks; i++) if (m_ppLevel2EnemyTanks[i]) m_ppLevel2EnemyTanks[i]->Release();
		delete[] m_ppLevel2EnemyTanks;
		m_ppLevel2EnemyTanks = NULL;
	}
	m_nLevel2EnemyTanks = 0;
	if (m_ppLevel2Obstacles)
	{
		for (int i = 0; i < m_nLevel2Obstacles; i++) if (m_ppLevel2Obstacles[i]) m_ppLevel2Obstacles[i]->Release();
		delete[] m_ppLevel2Obstacles;
		m_ppLevel2Obstacles = NULL;
	}
	m_nLevel2Obstacles = 0;
}

void CScene::BuildLevel1Effects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	ReleaseLevel1Effects();

	m_nLevel1Effects = LEVEL1_EFFECT_COUNT;
	m_ppLevel1Effects = new CEffectObject*[m_nLevel1Effects];
	int pnEffectMaterials[1] = { 1 };
	for (int i = 0; i < m_nLevel1Effects; i++)
	{
		m_ppLevel1Effects[i] = new CEffectObject(pd3dDevice, pd3dCommandList);
		m_ppLevel1Effects[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnEffectMaterials);
	}
}

void CScene::ResetLevel1Effects()
{
	if (!m_ppLevel1Effects) return;
	for (int i = 0; i < m_nLevel1Effects; i++) if (m_ppLevel1Effects[i]) m_ppLevel1Effects[i]->Reset();
}

void CScene::UpdateLevel1Effects(float fTimeElapsed)
{
	if (!m_ppLevel1Effects) return;
	for (int i = 0; i < m_nLevel1Effects; i++) if (m_ppLevel1Effects[i] && m_ppLevel1Effects[i]->IsActive()) m_ppLevel1Effects[i]->Animate(fTimeElapsed, NULL);
}

void CScene::RenderLevel1Effects(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_ppLevel1Effects) return;
	for (int i = 0; i < m_nLevel1Effects; i++)
	{
		if (m_ppLevel1Effects[i] && m_ppLevel1Effects[i]->IsActive())
		{
			m_ppLevel1Effects[i]->Render(pd3dCommandList, pCamera, m_ppLevel1Effects[i]->m_ppd3dcbInstancingGameObjects, m_ppLevel1Effects[i]->m_ppcbMappedInstancingGameObjects);
		}
	}
}

void CScene::ReleaseLevel1Effects()
{
	if (m_ppLevel1Effects)
	{
		for (int i = 0; i < m_nLevel1Effects; i++) if (m_ppLevel1Effects[i]) m_ppLevel1Effects[i]->Release();
		delete[] m_ppLevel1Effects;
		m_ppLevel1Effects = NULL;
	}
	m_nLevel1Effects = 0;
}

CEffectObject *CScene::FindInactiveLevel1Effect()
{
	if (!m_ppLevel1Effects) return(NULL);
	for (int i = 0; i < m_nLevel1Effects; i++) if (m_ppLevel1Effects[i] && !m_ppLevel1Effects[i]->IsActive()) return(m_ppLevel1Effects[i]);
	return(NULL);
}

void CScene::SpawnLevel1HitEffect(const XMFLOAT3& xmf3Position)
{
	CEffectObject *pEffect = FindInactiveLevel1Effect();
	if (!pEffect) return;
	pEffect->Spawn(xmf3Position, XMFLOAT3(0.0f, 10.0f, 0.0f), 0.18f, 0.45f, 1.4f, XMFLOAT4(1.0f, 0.76f, 0.12f, 1.0f), XMFLOAT4(1.0f, 0.55f, 0.05f, 1.0f), XMFLOAT4(0.9f, 0.32f, 0.02f, 1.0f));
}

void CScene::SpawnLevel1ExplosionEffect(const XMFLOAT3& xmf3Position)
{
	const int nExplosionPieces = 24;
	const XMFLOAT4 xmf4Ambients[4] = { XMFLOAT4(1.0f, 0.82f, 0.12f, 1.0f), XMFLOAT4(1.0f, 0.42f, 0.05f, 1.0f), XMFLOAT4(0.85f, 0.08f, 0.03f, 1.0f), XMFLOAT4(0.20f, 0.18f, 0.16f, 1.0f) };
	const XMFLOAT4 xmf4Diffuses[4] = { XMFLOAT4(1.0f, 0.70f, 0.05f, 1.0f), XMFLOAT4(1.0f, 0.30f, 0.04f, 1.0f), XMFLOAT4(0.75f, 0.06f, 0.02f, 1.0f), XMFLOAT4(0.12f, 0.11f, 0.10f, 1.0f) };
	const XMFLOAT4 xmf4Emissives[4] = { XMFLOAT4(1.0f, 0.45f, 0.02f, 1.0f), XMFLOAT4(0.9f, 0.20f, 0.01f, 1.0f), XMFLOAT4(0.45f, 0.03f, 0.01f, 1.0f), XMFLOAT4(0.03f, 0.025f, 0.02f, 1.0f) };
	for (int i = 0; i < nExplosionPieces; i++)
	{
		CEffectObject *pEffect = FindInactiveLevel1Effect();
		if (!pEffect) return;
		float fAngle = XM_2PI * (float)i / (float)nExplosionPieces;
		float fRadiusSpeed = 25.0f + (float)(i % 6) * 6.0f;
		XMFLOAT3 xmf3Velocity = XMFLOAT3(cosf(fAngle) * fRadiusSpeed, 8.0f + (float)(i % 5) * 5.0f, sinf(fAngle) * fRadiusSpeed);
		int nColor = i % 4;
		pEffect->Spawn(xmf3Position, xmf3Velocity, 0.56f + (float)(i % 4) * 0.07f, 0.55f, 2.4f, xmf4Ambients[nColor], xmf4Diffuses[nColor], xmf4Emissives[nColor]);
	}
}

void CScene::SpawnPlayerHitEffect(const XMFLOAT3& xmf3Position)
{
	for (int i = 0; i < 4; i++)
	{
		CEffectObject *pEffect = FindInactiveLevel1Effect();
		if (!pEffect) return;
		float fAngle = XM_PIDIV2 * (float)i;
		XMFLOAT3 xmf3Velocity = XMFLOAT3(cosf(fAngle) * 8.0f, 8.5f, sinf(fAngle) * 8.0f);
		pEffect->Spawn(xmf3Position, xmf3Velocity, 0.24f, 0.55f, 1.8f, XMFLOAT4(1.0f, 0.26f, 0.08f, 1.0f), XMFLOAT4(1.0f, 0.15f, 0.03f, 1.0f), XMFLOAT4(0.8f, 0.04f, 0.01f, 1.0f));
	}
}

void CScene::BuildStartNameExplosionEffects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	ReleaseStartNameExplosionEffects();

	m_nStartNameExplosionEffects = START_NAME_EXPLOSION_EFFECT_COUNT;
	m_ppStartNameExplosionEffects = new CEffectObject*[m_nStartNameExplosionEffects];
	int pnEffectMaterials[1] = { 1 };
	for (int i = 0; i < m_nStartNameExplosionEffects; i++)
	{
		m_ppStartNameExplosionEffects[i] = new CEffectObject(pd3dDevice, pd3dCommandList);
		m_ppStartNameExplosionEffects[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnEffectMaterials);
	}
}

void CScene::ResetStartNameExplosionEffects()
{
	if (!m_ppStartNameExplosionEffects) return;
	for (int i = 0; i < m_nStartNameExplosionEffects; i++) if (m_ppStartNameExplosionEffects[i]) m_ppStartNameExplosionEffects[i]->Reset();
}

void CScene::UpdateStartNameExplosionEffects(float fTimeElapsed)
{
	if (!m_ppStartNameExplosionEffects) return;
	for (int i = 0; i < m_nStartNameExplosionEffects; i++) if (m_ppStartNameExplosionEffects[i] && m_ppStartNameExplosionEffects[i]->IsActive()) m_ppStartNameExplosionEffects[i]->Animate(fTimeElapsed, NULL);
}

void CScene::RenderStartNameExplosionEffects(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if ((m_nSceneMode != GAME_SCENE_START) || !m_bNameExploding || !m_ppStartNameExplosionEffects) return;
	for (int i = 0; i < m_nStartNameExplosionEffects; i++)
	{
		if (m_ppStartNameExplosionEffects[i] && m_ppStartNameExplosionEffects[i]->IsActive())
		{
			m_ppStartNameExplosionEffects[i]->Render(pd3dCommandList, pCamera, m_ppStartNameExplosionEffects[i]->m_ppd3dcbInstancingGameObjects, m_ppStartNameExplosionEffects[i]->m_ppcbMappedInstancingGameObjects);
		}
	}
}

void CScene::ReleaseStartNameExplosionEffects()
{
	if (m_ppStartNameExplosionEffects)
	{
		for (int i = 0; i < m_nStartNameExplosionEffects; i++) if (m_ppStartNameExplosionEffects[i]) m_ppStartNameExplosionEffects[i]->Release();
		delete[] m_ppStartNameExplosionEffects;
		m_ppStartNameExplosionEffects = NULL;
	}
	m_nStartNameExplosionEffects = 0;
}

CEffectObject *CScene::FindInactiveStartNameExplosionEffect()
{
	if (!m_ppStartNameExplosionEffects) return(NULL);
	for (int i = 0; i < m_nStartNameExplosionEffects; i++) if (m_ppStartNameExplosionEffects[i] && !m_ppStartNameExplosionEffects[i]->IsActive()) return(m_ppStartNameExplosionEffects[i]);
	return(NULL);
}

void CScene::SpawnStartNameExplosionEffects()
{
	ResetStartNameExplosionEffects();
	XMFLOAT3 xmf3BasePosition = XMFLOAT3(0.0f, -25.0f, 0.0f);
	if (m_ppGameObjects && m_ppGameObjects[UI_NAME_OBJECT])
	{
		m_ppGameObjects[UI_NAME_OBJECT]->m_xmf4x4Transform = m_xmf4x4StartNameBaseTransform;
		xmf3BasePosition = m_ppGameObjects[UI_NAME_OBJECT]->GetPosition();
	}

	const XMFLOAT4 xmf4Ambients[4] = { XMFLOAT4(1.0f, 0.78f, 0.10f, 1.0f), XMFLOAT4(1.0f, 0.34f, 0.05f, 1.0f), XMFLOAT4(0.85f, 0.08f, 0.03f, 1.0f), XMFLOAT4(0.86f, 0.82f, 0.72f, 1.0f) };
	const XMFLOAT4 xmf4Diffuses[4] = { XMFLOAT4(1.0f, 0.62f, 0.04f, 1.0f), XMFLOAT4(1.0f, 0.24f, 0.03f, 1.0f), XMFLOAT4(0.70f, 0.05f, 0.02f, 1.0f), XMFLOAT4(0.92f, 0.88f, 0.78f, 1.0f) };
	const XMFLOAT4 xmf4Emissives[4] = { XMFLOAT4(0.9f, 0.32f, 0.02f, 1.0f), XMFLOAT4(0.75f, 0.12f, 0.01f, 1.0f), XMFLOAT4(0.38f, 0.02f, 0.01f, 1.0f), XMFLOAT4(0.18f, 0.16f, 0.12f, 1.0f) };
	for (int i = 0; i < START_NAME_EXPLOSION_EFFECT_COUNT; i++)
	{
		CEffectObject *pEffect = FindInactiveStartNameExplosionEffect();
		if (!pEffect) return;
		float fAngle = XM_2PI * (float)i / (float)START_NAME_EXPLOSION_EFFECT_COUNT;
		float fRadiusSpeed = 30.0f + (float)(i % 6) * 5.0f;
		float fDirectionSign = (i % 2) ? -1.0f : 1.0f;
		XMFLOAT3 xmf3Velocity = XMFLOAT3(cosf(fAngle) * fRadiusSpeed, sinf(fAngle) * 24.0f + (float)((i % 5) - 2) * 5.0f, fDirectionSign * (6.0f + (float)(i % 4) * 3.0f));
		float fLifeTime = 0.75f + (float)(i % 5) * 0.07f;
		float fStartScale = 0.5f + (float)(i % 3) * 0.1f;
		float fEndScale = 1.9f + (float)(i % 4) * 0.25f;
		int nColor = i % 4;
		pEffect->Spawn(xmf3BasePosition, xmf3Velocity, fLifeTime, fStartScale, fEndScale, xmf4Ambients[nColor], xmf4Diffuses[nColor], xmf4Emissives[nColor]);
	}
}
void CScene::CheckProjectileTargetCollisions()
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !m_ppProjectiles || !m_pLevel1Targets) return;

	for (int i = 0; i < m_nProjectiles; i++)
	{
		CProjectileObject *pProjectile = m_ppProjectiles[i];
		if (!pProjectile || !pProjectile->IsActive()) continue;

		XMFLOAT3 xmf3ProjectilePosition = pProjectile->GetPosition();
		float fProjectileRadius = pProjectile->GetCollisionRadius();

		for (int j = 0; j < m_nLevel1Targets; j++)
		{
			SLevel1TargetState& target = m_pLevel1Targets[j];
			if (!target.m_bActive || target.m_bDestroying) continue;
			if ((target.m_nObjectIndex < 0) || (target.m_nObjectIndex >= m_nGameObjects)) continue;
			CGameObject *pTargetObject = m_ppGameObjects[target.m_nObjectIndex];
			if (!pTargetObject) continue;

			XMFLOAT3 xmf3TargetPosition = pTargetObject->GetPosition();
			XMFLOAT3 xmf3Difference = Vector3::Subtract(xmf3ProjectilePosition, xmf3TargetPosition);
			float fDistance = Vector3::Length(xmf3Difference);
			float fHitDistance = fProjectileRadius + target.m_fCollisionRadius;
			if (fDistance <= fHitDistance)
			{
				SpawnLevel1HitEffect(xmf3ProjectilePosition);
				pProjectile->Reset();
				ApplyDamageToLevel1Target(j, 1);
				break;
			}
		}
	}
}

void CScene::ApplyDamageToLevel1Target(int nTargetIndex, int nDamage)
{
	if (!m_pLevel1Targets || (nTargetIndex < 0) || (nTargetIndex >= m_nLevel1Targets)) return;

	SLevel1TargetState& target = m_pLevel1Targets[nTargetIndex];
	if (!target.m_bActive || target.m_bDestroying) return;

	target.m_nHP -= nDamage;
	m_nLastHitLevel1TargetIndex = nTargetIndex;
	m_fLastHitTargetDisplayElapsedTime = 0.0f;
	char pstrDebug[160];
	sprintf_s(pstrDebug, "[Level1] Target hit: index=%d, HP=%d\n", nTargetIndex, target.m_nHP);
	::OutputDebugStringA(pstrDebug);

	if (target.m_nHP <= 0)
	{
		target.m_nHP = 0;
		target.m_bDestroying = true;
		target.m_fDestroyElapsedTime = 0.0f;
		if ((target.m_nObjectIndex >= 0) && (target.m_nObjectIndex < m_nGameObjects) && m_ppGameObjects[target.m_nObjectIndex]) SpawnLevel1ExplosionEffect(m_ppGameObjects[target.m_nObjectIndex]->GetPosition());
		sprintf_s(pstrDebug, "[Level1] Target destroyed: wave=%d, objectIndex=%d\n", target.m_nWave, target.m_nObjectIndex);
		::OutputDebugStringA(pstrDebug);
	}
}

bool CScene::IsCurrentLevel1WaveCleared() const
{
	if (!m_pLevel1Targets) return(false);

	bool bHasCurrentWaveTarget = false;
	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		if (m_pLevel1Targets[i].m_nWave != m_nCurrentLevel1Wave) continue;
		bHasCurrentWaveTarget = true;
		if (m_pLevel1Targets[i].m_bActive || m_pLevel1Targets[i].m_bDestroying) return(false);
	}
	return(bHasCurrentWaveTarget);
}

void CScene::AdvanceLevel1WaveIfNeeded()
{
	if (m_bLevel1Cleared || !IsCurrentLevel1WaveCleared()) return;

	if (m_nCurrentLevel1Wave == 1)
	{
		::OutputDebugStringA("[Level1] Activate Wave 2\n");
		ActivateLevel1Wave(2);
	}
	else if (m_nCurrentLevel1Wave == 2)
	{
		::OutputDebugStringA("[Level1] Activate Wave 3\n");
		ActivateLevel1Wave(3);
	}
	else if (m_nCurrentLevel1Wave == 3)
	{
		m_bLevel1Cleared = true;
		m_fLevel1ClearElapsedTime = 0.0f;
	m_bLevel1Failed = false;
	m_fLevel1FailedElapsedTime = 0.0f;
		for (int i = 0; i < m_nLevel1Targets; i++) m_pLevel1Targets[i].m_bActive = false;
		ResetLevel1Effects();
		::OutputDebugStringA("[Level1] Mission Clear\n");
	}
}
int CScene::GetHudEnemyTargetIndex() const
{
	if (m_bLevel1Cleared || !m_pLevel1Targets) return(-1);

	if ((m_nLastHitLevel1TargetIndex >= 0) && (m_nLastHitLevel1TargetIndex < m_nLevel1Targets))
	{
		const SLevel1TargetState& lastHitTarget = m_pLevel1Targets[m_nLastHitLevel1TargetIndex];
		if (lastHitTarget.m_bActive || lastHitTarget.m_bDestroying) return(m_nLastHitLevel1TargetIndex);
	}

	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		const SLevel1TargetState& target = m_pLevel1Targets[i];
		if ((target.m_nWave == m_nCurrentLevel1Wave) && (target.m_bActive || target.m_bDestroying)) return(i);
	}
	return(-1);
}

void CScene::UpdateLevel1HudBars()
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || !m_ppHudBars || (m_nHudBars < HUD_BAR_COUNT)) return;
	if (!m_ppHudBars[HUD_PLAYER_BACKGROUND] || !m_ppHudBars[HUD_PLAYER_GAUGE] || !m_ppHudBars[HUD_ENEMY_BACKGROUND] || !m_ppHudBars[HUD_ENEMY_GAUGE]) return;

	const float fHudFullWidth = 225.0f;
	const float fHudHeight = 3.5f;
	const float fHudDepth = 0.5f;
	const float fPlayerHudY = 60.0f;
	const float fEnemyHudY = 53.0f;
	const float fBackgroundZ = 0.0f;
	const float fGaugeZ = -0.35f;

	XMFLOAT3 xmf3HudRight = XMFLOAT3(1.0f, 0.0f, 0.0f);
	XMFLOAT3 xmf3HudUp = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3 xmf3HudLook = XMFLOAT3(0.0f, 0.0f, 1.0f);

	float fPlayerRatio = (m_nPlayerMaxHP > 0) ? ((float)m_nPlayerHP / (float)m_nPlayerMaxHP) : 0.0f;
	if (fPlayerRatio < 0.0f) fPlayerRatio = 0.0f;
	if (fPlayerRatio > 1.0f) fPlayerRatio = 1.0f;

	int nEnemyTargetIndex = GetHudEnemyTargetIndex();
	float fEnemyRatio = 0.0f;
	if ((nEnemyTargetIndex >= 0) && (nEnemyTargetIndex < m_nLevel1Targets))
	{
		const SLevel1TargetState& target = m_pLevel1Targets[nEnemyTargetIndex];
		fEnemyRatio = (target.m_nMaxHP > 0) ? ((float)target.m_nHP / (float)target.m_nMaxHP) : 0.0f;
	}
	if (fEnemyRatio < 0.0f) fEnemyRatio = 0.0f;
	if (fEnemyRatio > 1.0f) fEnemyRatio = 1.0f;

	float fPlayerGaugeWidth = fHudFullWidth * fPlayerRatio;
	float fEnemyGaugeWidth = fHudFullWidth * fEnemyRatio;
	float fPlayerGaugeCenterX = (-fHudFullWidth * 0.5f) + (fPlayerGaugeWidth * 0.5f);
	float fEnemyGaugeCenterX = (-fHudFullWidth * 0.5f) + (fEnemyGaugeWidth * 0.5f);

	m_ppHudBars[HUD_PLAYER_BACKGROUND]->SetHudTransform(XMFLOAT3(0.0f, fPlayerHudY, fBackgroundZ), xmf3HudRight, xmf3HudUp, xmf3HudLook, fHudFullWidth, fHudHeight, fHudDepth);
	m_ppHudBars[HUD_PLAYER_GAUGE]->SetHudTransform(XMFLOAT3(fPlayerGaugeCenterX, fPlayerHudY, fGaugeZ), xmf3HudRight, xmf3HudUp, xmf3HudLook, fPlayerGaugeWidth, fHudHeight, fHudDepth);
	m_ppHudBars[HUD_ENEMY_BACKGROUND]->SetHudTransform(XMFLOAT3(0.0f, fEnemyHudY, fBackgroundZ), xmf3HudRight, xmf3HudUp, xmf3HudLook, fHudFullWidth, fHudHeight, fHudDepth);
	m_ppHudBars[HUD_ENEMY_GAUGE]->SetHudTransform(XMFLOAT3(fEnemyGaugeCenterX, fEnemyHudY, fGaugeZ), xmf3HudRight, xmf3HudUp, xmf3HudLook, fEnemyGaugeWidth, fHudHeight, fHudDepth);
}

void CScene::RenderLevel1HudBars(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || !m_ppHudBars) return;
	for (int i = 0; i < m_nHudBars; i++)
	{
		if (m_ppHudBars[i])
		{
			m_ppHudBars[i]->UpdateTransform(NULL);
			m_ppHudBars[i]->Render(pd3dCommandList, pCamera, m_ppHudBars[i]->m_ppd3dcbInstancingGameObjects, m_ppHudBars[i]->m_ppcbMappedInstancingGameObjects);
		}
	}
}

void CScene::RenderLevel1HudOverlay(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if ((m_nSceneMode != GAME_SCENE_LEVEL1) || m_bLevel1Cleared || m_bLevel1Failed || !pd3dCommandList || !pCamera || !m_ppHudBars || !m_pHudCamera) return;

	UpdateLevel1HudBars();

	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	m_pHudCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -120.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	m_pHudCamera->SetViewportsAndScissorRects(pd3dCommandList);
	m_pHudCamera->UpdateShaderVariables(pd3dCommandList);
	UpdateShaderVariables(pd3dCommandList);
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, m_pd3dcbLights->GetGPUVirtualAddress());
	RenderLevel1HudBars(pd3dCommandList, m_pHudCamera);
}
bool CScene::IsActiveLevel1TargetObject(int nObjectIndex) const
{
	if (!m_pLevel1Targets) return(false);
	for (int i = 0; i < m_nLevel1Targets; i++)
	{
		if (m_pLevel1Targets[i].m_nObjectIndex == nObjectIndex) return(m_pLevel1Targets[i].m_bActive);
	}
	return(false);
}
bool CScene::IsVisibleObject(int nObject) const
{
	if (m_nSceneMode == GAME_SCENE_START) return((nObject == UI_TITLE_OBJECT) || ((nObject == UI_NAME_OBJECT) && !m_bNameExploding));
	if (m_nSceneMode == GAME_SCENE_MENU) return((nObject >= UI_TUTORIAL_OBJECT) && (nObject <= UI_END_OBJECT));
	if (m_nSceneMode == GAME_SCENE_LEVEL2) return(false);

	if (m_nSceneMode == GAME_SCENE_LEVEL1)
	{
		if (nObject == LEVEL1_CLEAR_OBJECT) return(m_bLevel1Cleared);
		if (nObject == LEVEL1_GAMEOVER_OBJECT) return(m_bLevel1Failed);
		if (nObject == WORLD_OBJECT_START + 4) return(true);
		if ((nObject >= WORLD_OBJECT_START) && (nObject < WORLD_OBJECT_START + 4)) return(IsActiveLevel1TargetObject(nObject));
		return(false);
	}
	return((nObject >= WORLD_OBJECT_START) && (nObject != LEVEL1_CLEAR_OBJECT) && (nObject != LEVEL1_GAMEOVER_OBJECT));
}

bool CScene::IsStartTitleHover(int x, int y) const
{
	return((x >= 220) && (x <= 1060) && (y >= 180) && (y <= 320));
}

bool CScene::IsStartNameHover(int x, int y) const
{
	return((x >= 420) && (x <= 860) && (y >= 385) && (y <= 540));
}

bool CScene::IsMenuStartHover(int x, int y, int *pnMenuItem) const
{
	static const RECT rcMenuStarts[UI_MENU_START_COUNT] =
	{
		{ 760, 80, 1060, 170 },
		{ 760, 210, 1060, 300 },
		{ 760, 340, 1060, 430 },
		{ 760, 470, 1060, 560 }
	};

	for (int i = 0; i < UI_MENU_START_COUNT; i++)
	{
		if ((x >= rcMenuStarts[i].left) && (x <= rcMenuStarts[i].right) && (y >= rcMenuStarts[i].top) && (y <= rcMenuStarts[i].bottom))
		{
			if (pnMenuItem) *pnMenuItem = i;
			return(true);
		}
	}
	return(false);
}

bool CScene::IsMenuEndHover(int x, int y) const
{
	return((x >= 540) && (x <= 740) && (y >= 585) && (y <= 700));
}
void CScene::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	m_nGameObjects = TOTAL_SCENE_OBJECTS;
	m_ppGameObjects = new CGameObject*[m_nGameObjects];
	for (int i = 0; i < m_nGameObjects; i++) m_ppGameObjects[i] = NULL;

	m_ppGameObjects[UI_TITLE_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/3DGameProgramming1.bin", XMFLOAT3(0.0f, 30.0f, 0.0f), 20.0f);
	m_ppGameObjects[UI_NAME_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/MyName.bin", XMFLOAT3(0.0f, -25.0f, 0.0f), 30.0f);
	m_xmf4x4StartTitleBaseTransform = m_ppGameObjects[UI_TITLE_OBJECT]->m_xmf4x4Transform;
	m_xmf4x4StartNameBaseTransform = m_ppGameObjects[UI_NAME_OBJECT]->m_xmf4x4Transform;
	m_ppGameObjects[UI_TUTORIAL_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Tutorial.bin", XMFLOAT3(-75.0f, 45.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_LEVEL1_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Level_1.bin", XMFLOAT3(-75.0f, 20.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_LEVEL2_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Level_2.bin", XMFLOAT3(-75.0f, -5.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_LEVEL3_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Level_3.bin", XMFLOAT3(-75.0f, -30.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_TUTORIAL_START_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Start.bin", XMFLOAT3(45.0f, 45.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_LEVEL1_START_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Start.bin", XMFLOAT3(45.0f, 20.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_LEVEL2_START_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Start.bin", XMFLOAT3(45.0f, -5.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_LEVEL3_START_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Start.bin", XMFLOAT3(45.0f, -30.0f, 0.0f), 11.0f);
	m_ppGameObjects[UI_END_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/End.bin", XMFLOAT3(-10.0f, -55.0f, 0.0f), 11.0f);
	for (int i = 0; i < UI_MENU_START_COUNT; i++) m_xmf4x4MenuStartBaseTransforms[i] = m_ppGameObjects[UI_MENU_START_FIRST_OBJECT + i]->m_xmf4x4Transform;
	m_xmf4x4MenuEndBaseTransform = m_ppGameObjects[UI_END_OBJECT]->m_xmf4x4Transform;
	m_ppGameObjects[LEVEL1_CLEAR_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/Clear.bin", XMFLOAT3(0.0f, 0.0f, 0.0f), 28.0f);
	m_xmf4x4Level1ClearBaseTransform = m_ppGameObjects[LEVEL1_CLEAR_OBJECT]->m_xmf4x4Transform;
	m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT] = CreateTextObject(pd3dDevice, pd3dCommandList, "Model/GameOver.bin", XMFLOAT3(80.0f, 0.0f, 0.0f), 18.0f);
	m_xmf4x4Level1GameOverBaseTransform = m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->m_xmf4x4Transform;
	int nMeshesInHierarchy = 0;
	int pnMaterialsInHierarchy[64]{};
	CGameObject *pApacheModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Apache.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);

	CGameObject *pApacheModel1 = pApacheModel;
	CApacheObject* pApacheObject = new CApacheObject();
	pApacheObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
	pApacheObject->SetChild(pApacheModel1, true);
	pApacheObject->OnInitialize();
	pApacheObject->SetScale(1.0f, 1.0f, 1.0f);
	pApacheObject->Rotate(0.0f, 90.0f, 0.0f);
	m_ppGameObjects[WORLD_OBJECT_START + 0] = pApacheObject;

	nMeshesInHierarchy = 0;
	ZeroMemory(pnMaterialsInHierarchy, sizeof(pnMaterialsInHierarchy));

	CGameObject *pApacheModel2 = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Apache.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);
	pApacheObject = new CApacheObject();
	pApacheObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
	pApacheObject->SetChild(pApacheModel2, true);
	pApacheObject->OnInitialize();
	pApacheObject->SetScale(1.0f, 1.0f, 1.0f);
	pApacheObject->Rotate(0.0f, -90.0f, 0.0f);
	m_ppGameObjects[WORLD_OBJECT_START + 1] = pApacheObject;

	nMeshesInHierarchy = 0;
	ZeroMemory(pnMaterialsInHierarchy, sizeof(pnMaterialsInHierarchy));
	CGameObject *pSuperCobraModel = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SuperCobra.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);

	CSuperCobraObject* pSuperCobraObject = new CSuperCobraObject();
	pSuperCobraObject->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
	pSuperCobraObject->SetChild(pSuperCobraModel, true);
	pSuperCobraObject->OnInitialize();
	pSuperCobraObject->SetScale(1.0f, 1.0f, 1.0f);
	pSuperCobraObject->Rotate(0.0f, -90.0f, 0.0f);
	m_ppGameObjects[WORLD_OBJECT_START + 2] = pSuperCobraObject;

	nMeshesInHierarchy = 0;
	ZeroMemory(pnMaterialsInHierarchy, sizeof(pnMaterialsInHierarchy));
	CGameObject *pMi24Model = CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Mi24.bin", &nMeshesInHierarchy, pnMaterialsInHierarchy);

	CMi24Object* pMi24Object = new CMi24Object();
	pMi24Object->CreateShaderVariables(pd3dDevice, pd3dCommandList, nMeshesInHierarchy, pnMaterialsInHierarchy);
	pMi24Object->SetChild(pMi24Model, true);
	pMi24Object->OnInitialize();
	pMi24Object->SetScale(1.0f, 1.0f, 1.0f);
	pMi24Object->Rotate(0.0f, -90.0f, 0.0f);
	m_ppGameObjects[WORLD_OBJECT_START + 3] = pMi24Object;

	int pnTerrainMaterials[1] = { 1 };
	m_pTerrain = new CTerrainObject(pd3dDevice, pd3dCommandList, "HeightMap/Level_1_terrain.raw");
	m_pTerrain->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnTerrainMaterials);
	m_ppGameObjects[WORLD_OBJECT_START + 4] = m_pTerrain;

	BuildLevel1Decorations(pd3dDevice, pd3dCommandList);
	BuildLevel2Objects(pd3dDevice, pd3dCommandList);
	InitializeLevel1Targets();

	m_nProjectiles = MAX_PROJECTILES;
	m_ppProjectiles = new CProjectileObject*[m_nProjectiles];
	int pnProjectileMaterials[1] = { 1 };
	for (int i = 0; i < m_nProjectiles; i++)
	{
		m_ppProjectiles[i] = new CProjectileObject(pd3dDevice, pd3dCommandList);
		m_ppProjectiles[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnProjectileMaterials);
	}
	m_nEnemyProjectiles = MAX_ENEMY_PROJECTILES;
	m_ppEnemyProjectiles = new CProjectileObject*[m_nEnemyProjectiles];
	int pnEnemyProjectileMaterials[1] = { 1 };
	for (int i = 0; i < m_nEnemyProjectiles; i++)
	{
		m_ppEnemyProjectiles[i] = new CProjectileObject(pd3dDevice, pd3dCommandList, 0.6f, 0.6f, 5.0f, XMFLOAT4(0.9f, 0.20f, 0.08f, 1.0f), XMFLOAT4(1.0f, 0.18f, 0.04f, 1.0f), XMFLOAT4(0.65f, 0.05f, 0.01f, 1.0f), XMFLOAT4(0.6f, 0.10f, 0.02f, 12.0f), 500.0f, 3.0f, 1.8f, 8);
		m_ppEnemyProjectiles[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnEnemyProjectileMaterials);
	}

	BuildLevel1Effects(pd3dDevice, pd3dCommandList);
	BuildStartNameExplosionEffects(pd3dDevice, pd3dCommandList);

	m_nHudBars = HUD_BAR_COUNT;
	m_ppHudBars = new CHudBarObject*[m_nHudBars];
	int pnHudMaterials[1] = { 1 };
	m_ppHudBars[HUD_PLAYER_BACKGROUND] = new CHudBarObject(pd3dDevice, pd3dCommandList, XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f), XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f), XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f));
	m_ppHudBars[HUD_PLAYER_GAUGE] = new CHudBarObject(pd3dDevice, pd3dCommandList, XMFLOAT4(0.35f, 1.0f, 0.35f, 1.0f), XMFLOAT4(0.20f, 1.0f, 0.25f, 1.0f), XMFLOAT4(0.0f, 0.75f, 0.05f, 1.0f));
	m_ppHudBars[HUD_ENEMY_BACKGROUND] = new CHudBarObject(pd3dDevice, pd3dCommandList, XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f), XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f), XMFLOAT4(0.08f, 0.08f, 0.08f, 1.0f));
	m_ppHudBars[HUD_ENEMY_GAUGE] = new CHudBarObject(pd3dDevice, pd3dCommandList, XMFLOAT4(1.0f, 0.25f, 0.08f, 1.0f), XMFLOAT4(1.0f, 0.28f, 0.06f, 1.0f), XMFLOAT4(0.85f, 0.08f, 0.02f, 1.0f));
	for (int i = 0; i < m_nHudBars; i++) if (m_ppHudBars[i]) m_ppHudBars[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList, 1, pnHudMaterials);

	if (!m_pHudCamera)
	{
		m_pHudCamera = new CCamera();
		m_pHudCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		m_pHudCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pHudCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pHudCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		m_pHudCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -120.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	}
	m_fProjectileFireCooldown = 0.0f;

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

/***/	ReleaseShaderVariables();

	if (m_pHudCamera)
	{
		m_pHudCamera->ReleaseShaderVariables();
		delete m_pHudCamera;
		m_pHudCamera = NULL;
	}

	if (m_ppGameObjects)
	{
		for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Release();
		delete[] m_ppGameObjects;
	}

	if (m_ppProjectiles)
	{
		for (int i = 0; i < m_nProjectiles; i++) if (m_ppProjectiles[i]) m_ppProjectiles[i]->Release();
		delete[] m_ppProjectiles;
		m_ppProjectiles = NULL;
		m_nProjectiles = 0;
	}
	if (m_ppEnemyProjectiles)
	{
		for (int i = 0; i < m_nEnemyProjectiles; i++) if (m_ppEnemyProjectiles[i]) m_ppEnemyProjectiles[i]->Release();
		delete[] m_ppEnemyProjectiles;
		m_ppEnemyProjectiles = NULL;
		m_nEnemyProjectiles = 0;
	}

	ReleaseLevel1Decorations();
	ReleaseLevel2Objects();
	ReleaseLevel1Effects();
	ReleaseStartNameExplosionEffects();

	if (m_ppHudBars)
	{
		for (int i = 0; i < m_nHudBars; i++) if (m_ppHudBars[i]) m_ppHudBars[i]->Release();
		delete[] m_ppHudBars;
		m_ppHudBars = NULL;
		m_nHudBars = 0;
	}

	if (m_pLevel1Targets)
	{
		delete[] m_pLevel1Targets;
		m_pLevel1Targets = NULL;
		m_nLevel1Targets = 0;
		m_nCurrentLevel1Wave = 1;
	}

	if (m_pLights) delete[] m_pLights;
	m_pTerrain = NULL;
}

ID3D12RootSignature *CScene::CreateGraphicsRootSignature(ID3D12Device *pd3dDevice)
{
	ID3D12RootSignature *pd3dGraphicsRootSignature = NULL;

	D3D12_ROOT_PARAMETER pd3dRootParameters[4];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[1].Descriptor.ShaderRegister = 2; //GameObject
	pd3dRootParameters[1].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; //Lights
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[3].Constants.Num32BitValues = 1;
	pd3dRootParameters[3].Constants.ShaderRegister = 3; //Framework Constants
	pd3dRootParameters[3].Constants.RegisterSpace = 0;
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 0;
	d3dRootSignatureDesc.pStaticSamplers = NULL;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob *pd3dSignatureBlob = NULL;
	ID3DBlob *pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void **)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256ÀÇ ¹è¼ö
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void **)&m_pcbMappedLights);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);

	if ((m_nSceneMode < GAME_SCENE_TUTORIAL) || ((m_nSceneMode == GAME_SCENE_LEVEL1) && (m_bLevel1Cleared || m_bLevel1Failed)))
	{
		XMFLOAT4 xmf4UiAmbient = XMFLOAT4(5.0f, 5.0f, 5.0f, 1.0f);
		int nUiLights = 0;
		::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &xmf4UiAmbient, sizeof(XMFLOAT4));
		::memcpy(&m_pcbMappedLights->m_nLights, &nUiLights, sizeof(int));
		return;
	}

	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nProjectiles; i++) if (m_ppProjectiles[i]) m_ppProjectiles[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nEnemyProjectiles; i++) if (m_ppEnemyProjectiles[i]) m_ppEnemyProjectiles[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nHudBars; i++) if (m_ppHudBars[i]) m_ppHudBars[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nLevel1Decorations; i++) if (m_ppLevel1Decorations[i]) m_ppLevel1Decorations[i]->ReleaseShaderVariables();
	if (m_pLevel2Terrain) m_pLevel2Terrain->ReleaseShaderVariables();
	if (m_pLevel2PlayerTank) m_pLevel2PlayerTank->ReleaseShaderVariables();
	for (int i = 0; i < m_nLevel2EnemyTanks; i++) if (m_ppLevel2EnemyTanks[i]) m_ppLevel2EnemyTanks[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nLevel2Obstacles; i++) if (m_ppLevel2Obstacles[i]) m_ppLevel2Obstacles[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nLevel1Effects; i++) if (m_ppLevel1Effects[i]) m_ppLevel1Effects[i]->ReleaseShaderVariables();
	for (int i = 0; i < m_nStartNameExplosionEffects; i++) if (m_ppStartNameExplosionEffects[i]) m_ppStartNameExplosionEffects[i]->ReleaseShaderVariables();
}

void CScene::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nProjectiles; i++) if (m_ppProjectiles[i]) m_ppProjectiles[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nEnemyProjectiles; i++) if (m_ppEnemyProjectiles[i]) m_ppEnemyProjectiles[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nHudBars; i++) if (m_ppHudBars[i]) m_ppHudBars[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nLevel1Decorations; i++) if (m_ppLevel1Decorations[i]) m_ppLevel1Decorations[i]->ReleaseUploadBuffers();
	if (m_pLevel2Terrain) m_pLevel2Terrain->ReleaseUploadBuffers();
	if (m_pLevel2PlayerTank) m_pLevel2PlayerTank->ReleaseUploadBuffers();
	for (int i = 0; i < m_nLevel2EnemyTanks; i++) if (m_ppLevel2EnemyTanks[i]) m_ppLevel2EnemyTanks[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nLevel2Obstacles; i++) if (m_ppLevel2Obstacles[i]) m_ppLevel2Obstacles[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nLevel1Effects; i++) if (m_ppLevel1Effects[i]) m_ppLevel1Effects[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nStartNameExplosionEffects; i++) if (m_ppStartNameExplosionEffects[i]) m_ppStartNameExplosionEffects[i]->ReleaseUploadBuffers();
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);
	if (m_nSceneMode == GAME_SCENE_LEVEL2)
	{
		if (nMessageID == WM_LBUTTONDOWN)
		{
			m_bLevel2MouseDragging = true;
			::SetCapture(hWnd);
			::GetCursorPos(&m_ptLevel2OldCursorPos);
			return(true);
		}
		if (nMessageID == WM_LBUTTONUP)
		{
			if (m_bLevel2MouseDragging)
			{
				m_bLevel2MouseDragging = false;
				::ReleaseCapture();
			}
			return(true);
		}
		if ((nMessageID == WM_MOUSEMOVE) && m_bLevel2MouseDragging && (::GetCapture() == hWnd))
		{
			POINT ptCursorPos;
			::GetCursorPos(&ptCursorPos);
			float fYawDelta = (float)(ptCursorPos.x - m_ptLevel2OldCursorPos.x) * 0.28f;
			if (fYawDelta != 0.0f) RotateLevel2PlayerTank(fYawDelta);
			::SetCursorPos(m_ptLevel2OldCursorPos.x, m_ptLevel2OldCursorPos.y);
			return(true);
		}
	}
	if ((m_nSceneMode == GAME_SCENE_START) && (nMessageID == WM_MOUSEMOVE))
	{
		m_bTitleHovered = IsStartTitleHover(x, y);
		m_bNameHovered = IsStartNameHover(x, y);
		return(true);
	}
	if ((m_nSceneMode == GAME_SCENE_MENU) && (nMessageID == WM_MOUSEMOVE))
	{
		int nHoveredMenuItem = -1;
		IsMenuStartHover(x, y, &nHoveredMenuItem);
		for (int i = 0; i < UI_MENU_START_COUNT; i++) m_bMenuStartHovered[i] = (i == nHoveredMenuItem);
		m_bMenuEndHovered = IsMenuEndHover(x, y);
		return(true);
	}
	if (nMessageID != WM_LBUTTONUP) return(false);
	if ((m_nSceneMode == GAME_SCENE_START) && !m_bNameExploding && IsStartNameHover(x, y))
	{
		m_bNameExploding = true;
		m_fNameExplosionElapsedTime = 0.0f;
		SpawnStartNameExplosionEffects();
		return(true);
	}
	if (m_nSceneMode == GAME_SCENE_MENU)
	{
		int nSelectedMenuItem = -1;
		if (IsMenuStartHover(x, y, &nSelectedMenuItem))
		{
			static const GAME_SCENE_MODE pnMenuSceneModes[UI_MENU_START_COUNT] =
			{
				GAME_SCENE_TUTORIAL,
				GAME_SCENE_LEVEL1,
				GAME_SCENE_LEVEL2,
				GAME_SCENE_LEVEL3
			};
			SetSceneMode(pnMenuSceneModes[nSelectedMenuItem]);
			return(true);
		}
		if (IsMenuEndHover(x, y))
		{
			::PostQuitMessage(0);
			return(true);
		}
	}
	return(m_nSceneMode < GAME_SCENE_TUTORIAL);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (nMessageID == WM_KEYUP)
	{
		if ((wParam == VK_ESCAPE) && (m_nSceneMode >= GAME_SCENE_TUTORIAL))
		{
			SetSceneMode(GAME_SCENE_MENU);
			return(true);
		}
		if ((wParam == 'N') && (m_nSceneMode == GAME_SCENE_LEVEL1))
		{
			SetSceneMode(GAME_SCENE_LEVEL2);
			return(true);
		}
		if ((wParam == VK_END) && (m_nSceneMode == GAME_SCENE_MENU))
		{
			::PostQuitMessage(0);
			return(true);
		}
		return(m_nSceneMode < GAME_SCENE_TUTORIAL);
	}

	return(m_nSceneMode < GAME_SCENE_TUTORIAL);
}
bool CScene::ProcessInput(UCHAR *pKeysBuffer)
{
	if (m_nSceneMode == GAME_SCENE_LEVEL2) return(ProcessLevel2Input(pKeysBuffer));
	if ((m_nSceneMode == GAME_SCENE_LEVEL1) && !m_bLevel1Cleared && !m_bLevel1Failed && (pKeysBuffer[VK_SPACE] & 0xF0)) FirePlayerProjectile();
	return(m_nSceneMode < GAME_SCENE_TUTORIAL);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;
	m_fModeElapsedTime += fTimeElapsed;
	if ((m_nSceneMode == GAME_SCENE_LEVEL2) && m_bPendingLevel2Reset && m_pPlayer) ResetLevel2();

	if (m_nSceneMode == GAME_SCENE_START)
	{
		if (m_ppGameObjects[UI_TITLE_OBJECT])
		{
			m_fTitleHoverRotation = (m_bTitleHovered) ? (m_fTitleHoverRotation + 150.0f * fTimeElapsed) : 0.0f;
			m_ppGameObjects[UI_TITLE_OBJECT]->m_xmf4x4Transform = m_xmf4x4StartTitleBaseTransform;
			if (m_bTitleHovered) m_ppGameObjects[UI_TITLE_OBJECT]->Rotate(0.0f, m_fTitleHoverRotation, 0.0f);
			else m_ppGameObjects[UI_TITLE_OBJECT]->UpdateTransform(NULL);
		}

		if (m_ppGameObjects[UI_NAME_OBJECT])
		{
			if (m_bNameExploding)
			{
				m_fNameExplosionElapsedTime += fTimeElapsed;
				UpdateStartNameExplosionEffects(fTimeElapsed);
				if (m_fNameExplosionElapsedTime >= 0.95f) SetSceneMode(GAME_SCENE_MENU);
			}
			else
			{
				m_fNameHoverRotation = (m_bNameHovered) ? (m_fNameHoverRotation + 150.0f * fTimeElapsed) : 0.0f;
				m_ppGameObjects[UI_NAME_OBJECT]->m_xmf4x4Transform = m_xmf4x4StartNameBaseTransform;
				if (m_bNameHovered) m_ppGameObjects[UI_NAME_OBJECT]->Rotate(0.0f, m_fNameHoverRotation, 0.0f);
				else m_ppGameObjects[UI_NAME_OBJECT]->UpdateTransform(NULL);
			}
		}
		return;
	}

	if (m_nSceneMode == GAME_SCENE_MENU)
	{
		for (int i = 0; i < UI_MENU_START_COUNT; i++)
		{
			CGameObject *pStartObject = m_ppGameObjects[UI_MENU_START_FIRST_OBJECT + i];
			if (pStartObject)
			{
				m_fMenuStartHoverRotation[i] = (m_bMenuStartHovered[i]) ? (m_fMenuStartHoverRotation[i] + 150.0f * fTimeElapsed) : 0.0f;
				pStartObject->m_xmf4x4Transform = m_xmf4x4MenuStartBaseTransforms[i];
				if (m_bMenuStartHovered[i]) pStartObject->Rotate(0.0f, m_fMenuStartHoverRotation[i], 0.0f);
				else pStartObject->UpdateTransform(NULL);
			}
		}
		if (m_ppGameObjects[UI_END_OBJECT])
		{
			m_fMenuEndHoverRotation = (m_bMenuEndHovered) ? (m_fMenuEndHoverRotation + 150.0f * fTimeElapsed) : 0.0f;
			m_ppGameObjects[UI_END_OBJECT]->m_xmf4x4Transform = m_xmf4x4MenuEndBaseTransform;
			if (m_bMenuEndHovered) m_ppGameObjects[UI_END_OBJECT]->Rotate(0.0f, m_fMenuEndHoverRotation, 0.0f);
			else m_ppGameObjects[UI_END_OBJECT]->UpdateTransform(NULL);
		}
		return;
	}

	for (int i = WORLD_OBJECT_START; i < m_nGameObjects; i++)
	{
		if (m_ppGameObjects[i]) m_ppGameObjects[i]->Animate(fTimeElapsed, NULL);
	}

	if (m_nSceneMode == GAME_SCENE_LEVEL2) UpdateLevel2Objects(fTimeElapsed);

	if (m_nSceneMode == GAME_SCENE_LEVEL1)
	{
		if (m_fProjectileFireCooldown > 0.0f) m_fProjectileFireCooldown -= fTimeElapsed;
		if (m_nLastHitLevel1TargetIndex >= 0) m_fLastHitTargetDisplayElapsedTime += fTimeElapsed;
		if (m_bLevel1Cleared)
		{
			m_fLevel1ClearElapsedTime += fTimeElapsed;
			ResetLevel1Effects();
			UpdateLevel1ClearText();
			if (m_fLevel1ClearElapsedTime >= 2.0f) SetSceneMode(GAME_SCENE_LEVEL2);
		}
		else if (m_bLevel1Failed)
		{
			m_fLevel1FailedElapsedTime += fTimeElapsed;
			ResetLevel1Effects();
			UpdateLevel1GameOverText();
		}
		else
		{
			for (int i = 0; i < m_nProjectiles; i++) if (m_ppProjectiles[i] && m_ppProjectiles[i]->IsActive()) m_ppProjectiles[i]->Animate(fTimeElapsed, NULL);
			for (int i = 0; i < m_nEnemyProjectiles; i++) if (m_ppEnemyProjectiles[i] && m_ppEnemyProjectiles[i]->IsActive()) m_ppEnemyProjectiles[i]->Animate(fTimeElapsed, NULL);
			UpdateLevel1Effects(fTimeElapsed);
			UpdateLevel1Targets(fTimeElapsed);
			UpdateLevel1EnemyFire(fTimeElapsed);
			CheckProjectileTargetCollisions();
			CheckEnemyProjectilePlayerCollisions();
			AdvanceLevel1WaveIfNeeded();
		}
	}
	ClampPlayerToTerrain();


}
void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	if ((m_nSceneMode < GAME_SCENE_TUTORIAL) || ((m_nSceneMode == GAME_SCENE_LEVEL1) && (m_bLevel1Cleared || m_bLevel1Failed)))
	{
		pCamera->GenerateViewMatrix(XMFLOAT3(0.0f, 0.0f, -120.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f));
	}

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress); //Lights

	if ((m_nSceneMode == GAME_SCENE_LEVEL1) && m_bLevel1Cleared)
	{
		if (m_ppGameObjects[LEVEL1_CLEAR_OBJECT])
		{
			m_ppGameObjects[LEVEL1_CLEAR_OBJECT]->UpdateTransform(NULL);
			m_ppGameObjects[LEVEL1_CLEAR_OBJECT]->Render(pd3dCommandList, pCamera, m_ppGameObjects[LEVEL1_CLEAR_OBJECT]->m_ppd3dcbInstancingGameObjects, m_ppGameObjects[LEVEL1_CLEAR_OBJECT]->m_ppcbMappedInstancingGameObjects);
		}
		return;
	}

	if ((m_nSceneMode == GAME_SCENE_LEVEL1) && m_bLevel1Failed)
	{
		if (m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT])
		{
			m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->UpdateTransform(NULL);
			m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->Render(pd3dCommandList, pCamera, m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->m_ppd3dcbInstancingGameObjects, m_ppGameObjects[LEVEL1_GAMEOVER_OBJECT]->m_ppcbMappedInstancingGameObjects);
		}
		return;
	}

	for (int i = 0; i < m_nGameObjects; i++)
	{
		if (m_ppGameObjects[i] && IsVisibleObject(i))
		{
			m_ppGameObjects[i]->UpdateTransform(NULL);
			m_ppGameObjects[i]->Render(pd3dCommandList, pCamera, m_ppGameObjects[i]->m_ppd3dcbInstancingGameObjects, m_ppGameObjects[i]->m_ppcbMappedInstancingGameObjects);
		}
	}

	if (m_nSceneMode == GAME_SCENE_START) RenderStartNameExplosionEffects(pd3dCommandList, pCamera);


	if (m_nSceneMode == GAME_SCENE_LEVEL2) RenderLevel2Objects(pd3dCommandList, pCamera);

	if (m_nSceneMode == GAME_SCENE_LEVEL1)
	{
		RenderLevel1Decorations(pd3dCommandList, pCamera);
		for (int i = 0; i < m_nProjectiles; i++)
		{
			if (m_ppProjectiles[i] && m_ppProjectiles[i]->IsActive())
			{
				m_ppProjectiles[i]->Render(pd3dCommandList, pCamera, m_ppProjectiles[i]->m_ppd3dcbInstancingGameObjects, m_ppProjectiles[i]->m_ppcbMappedInstancingGameObjects);
			}
		}
		if (!m_bLevel1Cleared && !m_bLevel1Failed)
		{
			for (int i = 0; i < m_nEnemyProjectiles; i++)
			{
				if (m_ppEnemyProjectiles[i] && m_ppEnemyProjectiles[i]->IsActive())
				{
					m_ppEnemyProjectiles[i]->Render(pd3dCommandList, pCamera, m_ppEnemyProjectiles[i]->m_ppd3dcbInstancingGameObjects, m_ppEnemyProjectiles[i]->m_ppcbMappedInstancingGameObjects);
				}
			}
			RenderLevel1Effects(pd3dCommandList, pCamera);
		}
	}
}
