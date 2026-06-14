#include "stdafx.h"
#include "Terrain.h"

#include <fstream>

static float ClampFloat(float fValue, float fMin, float fMax)
{
	if (fValue < fMin) return(fMin);
	if (fValue > fMax) return(fMax);
	return(fValue);
}

static int ClampInt(int nValue, int nMin, int nMax)
{
	if (nValue < nMin) return(nMin);
	if (nValue > nMax) return(nMax);
	return(nValue);
}

CHeightMapImage::CHeightMapImage(const char *pstrFileName, int nWidth, int nLength, const XMFLOAT3& xmf3Scale)
{
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;
	m_pHeightMapPixels.resize(m_nWidth * m_nLength);

	const size_t nExpectedBytes = size_t(m_nWidth) * size_t(m_nLength);
	std::ifstream file(pstrFileName, std::ios::binary | std::ios::ate);
	if (file)
	{
		const std::streamoff nFileSize = file.tellg();
		if (nFileSize == std::streamoff(nExpectedBytes))
		{
			std::vector<BYTE> rawPixels(nExpectedBytes);
			file.seekg(0, std::ios::beg);
			file.read(reinterpret_cast<char *>(rawPixels.data()), nExpectedBytes);
			if (file.gcount() == std::streamsize(nExpectedBytes))
			{
				for (int z = 0; z < m_nLength; z++)
				{
					for (int x = 0; x < m_nWidth; x++)
					{
						m_pHeightMapPixels[(m_nLength - 1 - z) * m_nWidth + x] = rawPixels[z * m_nWidth + x];
					}
				}
				m_bLoaded = true;
			}
		}
	}

	if (!m_bLoaded) BuildFallbackHeightMap();
}

void CHeightMapImage::BuildFallbackHeightMap()
{
	for (int z = 0; z < m_nLength; z++)
	{
		for (int x = 0; x < m_nWidth; x++)
		{
			float fx = float(x) / float(m_nWidth - 1);
			float fz = float(z) / float(m_nLength - 1);
			float fHill = (sinf(fx * XM_2PI * 3.0f) * cosf(fz * XM_2PI * 3.0f) * 0.5f) + 0.5f;
			BYTE nHeight = BYTE(40.0f + (fHill * 120.0f));
			m_pHeightMapPixels[(m_nLength - 1 - z) * m_nWidth + x] = nHeight;
		}
	}
}

BYTE CHeightMapImage::GetPixel(int x, int z) const
{
	x = ClampInt(x, 0, m_nWidth - 1);
	z = ClampInt(z, 0, m_nLength - 1);
	return(m_pHeightMapPixels[z * m_nWidth + x]);
}

float CHeightMapImage::GetHeight(float x, float z) const
{
	x = ClampFloat(x, 0.0f, float(m_nWidth - 2));
	z = ClampFloat(z, 0.0f, float(m_nLength - 2));

	int ix = int(floorf(x));
	int iz = int(floorf(z));
	float fx = x - float(ix);
	float fz = z - float(iz);

	float h00 = float(GetPixel(ix, iz));
	float h10 = float(GetPixel(ix + 1, iz));
	float h01 = float(GetPixel(ix, iz + 1));
	float h11 = float(GetPixel(ix + 1, iz + 1));

	float h0 = h00 + ((h10 - h00) * fx);
	float h1 = h01 + ((h11 - h01) * fx);
	return((h0 + ((h1 - h0) * fz)) * m_xmf3Scale.y);
}

XMFLOAT3 CHeightMapImage::GetHeightMapNormal(int x, int z) const
{
	if ((x <= 0) || (x >= (m_nWidth - 1)) || (z <= 0) || (z >= (m_nLength - 1)))
	{
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));
	}

	float fLeft = float(GetPixel(x - 1, z)) * m_xmf3Scale.y;
	float fRight = float(GetPixel(x + 1, z)) * m_xmf3Scale.y;
	float fBack = float(GetPixel(x, z - 1)) * m_xmf3Scale.y;
	float fForward = float(GetPixel(x, z + 1)) * m_xmf3Scale.y;

	XMFLOAT3 xmf3TangentX = XMFLOAT3(2.0f * m_xmf3Scale.x, fRight - fLeft, 0.0f);
	XMFLOAT3 xmf3TangentZ = XMFLOAT3(0.0f, fForward - fBack, 2.0f * m_xmf3Scale.z);
	XMVECTOR xmvNormal = XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&xmf3TangentZ), XMLoadFloat3(&xmf3TangentX)));

	XMFLOAT3 xmf3Normal;
	XMStoreFloat3(&xmf3Normal, xmvNormal);
	if (xmf3Normal.y < 0.0f)
	{
		xmf3Normal.x = -xmf3Normal.x;
		xmf3Normal.y = -xmf3Normal.y;
		xmf3Normal.z = -xmf3Normal.z;
	}
	return(xmf3Normal);
}

CHeightMapTerrainMesh::CHeightMapTerrainMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const CHeightMapImage& heightMapImage)
{
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_nType = VERTEXT_POSITION | VERTEXT_NORMAL;

	int nWidth = heightMapImage.GetWidth();
	int nLength = heightMapImage.GetLength();
	XMFLOAT3 xmf3Scale = heightMapImage.GetScale();
	m_nVertices = nWidth * nLength;

	std::vector<XMFLOAT3> positions(m_nVertices);
	std::vector<XMFLOAT3> normals(m_nVertices);
	for (int z = 0; z < nLength; z++)
	{
		for (int x = 0; x < nWidth; x++)
		{
			int nIndex = z * nWidth + x;
			positions[nIndex] = XMFLOAT3((x - ((nWidth - 1) * 0.5f)) * xmf3Scale.x, heightMapImage.GetHeight(float(x), float(z)), (z - ((nLength - 1) * 0.5f)) * xmf3Scale.z);
			normals[nIndex] = heightMapImage.GetHeightMapNormal(x, z);
		}
	}

	m_nIndices = (nWidth - 1) * (nLength - 1) * 6;
	std::vector<UINT> indices(m_nIndices);
	UINT nIndex = 0;
	for (int z = 0; z < (nLength - 1); z++)
	{
		for (int x = 0; x < (nWidth - 1); x++)
		{
			UINT nTopLeft = z * nWidth + x;
			UINT nTopRight = nTopLeft + 1;
			UINT nBottomLeft = (z + 1) * nWidth + x;
			UINT nBottomRight = nBottomLeft + 1;

			indices[nIndex++] = nTopLeft;
			indices[nIndex++] = nBottomLeft;
			indices[nIndex++] = nTopRight;
			indices[nIndex++] = nTopRight;
			indices[nIndex++] = nBottomLeft;
			indices[nIndex++] = nBottomRight;
		}
	}

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, positions.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);
	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;

	m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, normals.data(), sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);
	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;

	m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, indices.data(), sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CHeightMapTerrainMesh::~CHeightMapTerrainMesh()
{
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CHeightMapTerrainMesh::ReleaseUploadBuffers()
{
	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	m_pd3dPositionUploadBuffer = NULL;

	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;

	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dIndexUploadBuffer = NULL;
}

void CHeightMapTerrainMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList)
{
	Render(pd3dCommandList, 0);
}

void CHeightMapTerrainMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW d3dVertexBufferViews[2] = { m_d3dPositionBufferView, m_d3dNormalBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 2, d3dVertexBufferViews);
	pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
	pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

CTerrainObject::CTerrainObject(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, const char *pstrHeightMapFileName, const XMFLOAT3& xmf3Scale) : m_HeightMapImage(pstrHeightMapFileName, TERRAIN_HEIGHT_MAP_WIDTH, TERRAIN_HEIGHT_MAP_LENGTH, xmf3Scale)
{
	SetMesh(new CHeightMapTerrainMesh(pd3dDevice, pd3dCommandList, m_HeightMapImage));

	m_nMaterials = 1;
	m_ppMaterials = new CMaterial*[m_nMaterials];
	m_ppMaterials[0] = new CMaterial();
	m_ppMaterials[0]->SetIlluminatedShader();

	CMaterialColors *pMaterialColors = new CMaterialColors();
	pMaterialColors->m_xmf4Ambient = XMFLOAT4(0.22f, 0.28f, 0.16f, 1.0f);
	pMaterialColors->m_xmf4Diffuse = XMFLOAT4(0.30f, 0.42f, 0.18f, 1.0f);
	pMaterialColors->m_xmf4Specular = XMFLOAT4(0.08f, 0.08f, 0.06f, 8.0f);
	pMaterialColors->m_xmf4Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_ppMaterials[0]->SetMaterialColors(pMaterialColors);
}

float CTerrainObject::GetHeight(float worldX, float worldZ) const
{
	XMFLOAT3 xmf3Scale = m_HeightMapImage.GetScale();
	float mapX = (worldX / xmf3Scale.x) + ((m_HeightMapImage.GetWidth() - 1) * 0.5f);
	float mapZ = (worldZ / xmf3Scale.z) + ((m_HeightMapImage.GetLength() - 1) * 0.5f);
	return(m_HeightMapImage.GetHeight(mapX, mapZ));
}
