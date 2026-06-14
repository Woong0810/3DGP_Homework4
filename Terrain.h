#pragma once

#include <vector>
#include "Object.h"

static const int TERRAIN_HEIGHT_MAP_WIDTH = 257;
static const int TERRAIN_HEIGHT_MAP_LENGTH = 257;
static const float TERRAIN_SCALE_X = 8.0f;
static const float TERRAIN_SCALE_Y = 0.15f;
static const float TERRAIN_SCALE_Z = 8.0f;

class CHeightMapImage
{
public:
	CHeightMapImage(const char *pstrFileName = "HeightMap/Level_1_terrain.raw", int nWidth = TERRAIN_HEIGHT_MAP_WIDTH, int nLength = TERRAIN_HEIGHT_MAP_LENGTH, const XMFLOAT3& xmf3Scale = XMFLOAT3(TERRAIN_SCALE_X, TERRAIN_SCALE_Y, TERRAIN_SCALE_Z));
	virtual ~CHeightMapImage() { }

	bool IsLoaded() const { return(m_bLoaded); }
	int GetWidth() const { return(m_nWidth); }
	int GetLength() const { return(m_nLength); }
	XMFLOAT3 GetScale() const { return(m_xmf3Scale); }

	float GetHeight(float x, float z) const;
	XMFLOAT3 GetHeightMapNormal(int x, int z) const;

private:
	BYTE GetPixel(int x, int z) const;
	void BuildFallbackHeightMap();

	std::vector<BYTE>				m_pHeightMapPixels;
	int								m_nWidth = 0;
	int								m_nLength = 0;
	XMFLOAT3						m_xmf3Scale = XMFLOAT3(TERRAIN_SCALE_X, TERRAIN_SCALE_Y, TERRAIN_SCALE_Z);
	bool							m_bLoaded = false;
};

class CHeightMapTerrainMesh : public CMesh
{
public:
	CHeightMapTerrainMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const CHeightMapImage& heightMapImage);
	virtual ~CHeightMapTerrainMesh();

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

class CTerrainObject : public CGameObject
{
public:
	CTerrainObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const char *pstrHeightMapFileName = "HeightMap/Level_1_terrain.raw");
	virtual ~CTerrainObject() { }

	float GetHeight(float worldX, float worldZ) const;

private:
	CHeightMapImage					m_HeightMapImage;
};
