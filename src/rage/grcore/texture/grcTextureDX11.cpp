#include "grcTextureDX11.h"

#include "rage_hook/grcore/rageD3D.h"

void rage::grcTextureDX11::GetUsageAndAccessFlags(u8 createFlags, D3D11_USAGE& usage, UINT& access)
{
	switch (createFlags)
	{
	case 1:
	case 2:
		usage = D3D11_USAGE_DEFAULT;
		access = 0;
		break;
	case 3:
	case 4:
	case 5:
		usage = D3D11_USAGE_DYNAMIC;
		access = (u8)D3D11_CPU_ACCESS_WRITE;
		break;
	default:
		usage = D3D11_USAGE_IMMUTABLE;
		access = 0;
	}
}

u8 rage::grcTextureDX11::GetD3DArraySize() const
{
	return m_ArraySize + 1;
}

void rage::grcTextureDX11::ConvertFormat()
{
	m_Format = D3D::TextureFormatToDXGI((u32)m_Format, m_PcFlags2);
	if (m_Format == DXGI_FORMAT_UNKNOWN)
		m_Format = GRC_TX_FALLBACK_FORMAT;
}

bool rage::grcTextureDX11::ComputePitch(u8 mip, u32* pRowPitch, u32* pSlicePitch) const
{
	HRESULT result = D3D::ComputePitch(
		m_Format, m_Width, m_Height, mip, pRowPitch, pSlicePitch);
	return result == S_OK;
}

D3D11_TEXTURE3D_DESC rage::grcTextureDX11::GetDesk3D(u8 createFlags, u8 bindFlags) const
{
	D3D11_TEXTURE3D_DESC desk{};
	desk.Format = m_Format;
	desk.Width = m_Width;
	desk.Height = m_Height;
	desk.MipLevels = m_MipLevels;
	desk.BindFlags = bindFlags | D3D11_BIND_SHADER_RESOURCE;
	GetUsageAndAccessFlags(createFlags, desk.Usage, desk.CPUAccessFlags);
	return desk;
}

D3D11_TEXTURE2D_DESC rage::grcTextureDX11::GetDesc2D(u8 createFlags, u8 bindFlags) const
{
	D3D11_TEXTURE2D_DESC desk{};
	desk.Format = m_Format;
	desk.Width = m_Width;
	desk.Height = m_Height;
	desk.MipLevels = m_MipLevels;
	desk.BindFlags = bindFlags | D3D11_BIND_SHADER_RESOURCE;
	desk.ArraySize = m_ArraySize + 1;
	GetUsageAndAccessFlags(createFlags, desk.Usage, desk.CPUAccessFlags);

	desk.SampleDesc.Count = 1;
	desk.SampleDesc.Quality = 0;

	if (IsCubeMap())
		desk.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	return desk;
}

void rage::grcTextureDX11::GetViewDesc3D(D3D11_SHADER_RESOURCE_VIEW_DESC& viewDesk) const
{
	viewDesk.Format = m_Format;
	viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
	viewDesk.Texture3D.MipLevels = m_MipLevels;
}

void rage::grcTextureDX11::GetViewDesc2D(D3D11_SHADER_RESOURCE_VIEW_DESC& viewDesk) const
{
	viewDesk.Format = m_Format;
	u8 arraySize = GetD3DArraySize();

	if (IsCubeMap())
	{
		viewDesk.Texture2D.MipLevels = m_MipLevels;
		if (arraySize <= 6)
		{
			viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			return;
		}

		viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
		viewDesk.Texture2DArray.ArraySize = arraySize / 6;
		return;
	}

	viewDesk.Texture2D.MipLevels = m_MipLevels;
	if (arraySize <= 1)
	{
		viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		return;
	}

	viewDesk.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesk.Texture2DArray.ArraySize = arraySize;
}

HRESULT rage::grcTextureDX11::CreateFromData(const std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> subData,
	u8 createFlags, u8 cacheFlags, D3D11_BIND_FLAG bindFlags)
{
	HRESULT result;

	// TODO: RAGE cache device
	ID3D11Device* pDevice = rh::D3D::GetDevice();

	// Alternative versions of code below:

	// m_PcFlags2 &= 0xB1u;
	// v18 = createFlags >= 3u;
	// m_PcFlags2 |= 2 * ((32 * v18) | createFlags & 7);

	// if ( createFlags >= 3u )
	//     v9 = 32;
	// m_PcFlags2 = m_PcFlags2 & 0xB1 | (2 * (createFlags & 7 | v9));

	m_PcFlags2 &= 0xB1;
	m_PcFlags2 |= 2 * (createFlags & 7 | 32 * (createFlags >= 0x3));

	D3D11_SUBRESOURCE_DATA* pInitialData = subData.get();

	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesk{};
	if (IsVolume())
	{
		const D3D11_TEXTURE3D_DESC desk = GetDesk3D(createFlags, bindFlags);
		if (desk.Usage != D3D11_USAGE_IMMUTABLE)
			pInitialData = nullptr;

		ID3D11Texture3D* pTexture;
		result = pDevice->CreateTexture3D(&desk, pInitialData, &pTexture);
		m_Resource = pTexture;

		GetViewDesc3D(viewDesk);

		// TODO: Cache entry
	}
	else
	{
		const D3D11_TEXTURE2D_DESC desk = GetDesc2D(createFlags, bindFlags);
		if (desk.Usage == D3D11_USAGE_DYNAMIC)
			pInitialData = nullptr;

		ID3D11Texture2D* pTexture;
		result = pDevice->CreateTexture2D(&desk, pInitialData, &pTexture);
		m_Resource = pTexture;

		GetViewDesc2D(viewDesk);

		// TODO: Cache entry
	}

	if (result == S_OK)
	{
		result = pDevice->CreateShaderResourceView((ID3D11Resource*)m_Resource, &viewDesk, &m_ShaderView);
	}

	// Also can be seen as flag removal &= ~0x80u;
	m_PcFlags2 &= 0x7F;

	// TODO: There's some weird code...

	return result;
}

std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> rage::grcTextureDX11::GetInitialData() const
{
	u8* pTextureData = m_TextureData;
	auto pSubData = std::make_shared<D3D11_SUBRESOURCE_DATA[]>(m_MipLevels);
	for (u8 i = 0; i < m_MipLevels; i++)
	{
		u32 rowPitch;
		u32 slicePitch;
		ComputePitch(i, &rowPitch, &slicePitch);

		D3D11_SUBRESOURCE_DATA& subData = pSubData[i];
		subData.pSysMem = pTextureData;
		subData.SysMemPitch = rowPitch;
		subData.SysMemSlicePitch = slicePitch;

		pTextureData += slicePitch;
	}
	return pSubData;
}

void rage::grcTextureDX11::Init()
{
	if (!m_TextureData)
		return;

	if (m_Format <= DXGI_FORMAT_BC7_UNORM_SRGB)
		m_MipLevels = D3D::ClampMipLevels(m_Width, m_Height, m_MipLevels);

	std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> pSubData = GetInitialData();

	u8 createFlags = 0; // Immutable?
	if (m_PcFlags1 != 0)
	{
		createFlags = 1; // Create cache entry?
		m_TextureData = nullptr;
	}

	CreateFromData(pSubData, createFlags, 0, D3D11_BIND_SHADER_RESOURCE);
}

rage::grcTextureDX11::grcTextureDX11(const datResource& rsc) : grcTexturePC(rsc)
{
	rsc.Fixup(m_TextureData);

	ConvertFormat();
	Init();

	if ((m_PcFlags2 & 0x10) == 0) // 0x10 - Keep memory?
	{
		// TODO: Remove texture data.
	}
}

ID3D11ShaderResourceView* rage::grcTextureDX11::GetShaderResourceView() const
{
	return (ID3D11ShaderResourceView*)GetResourceView();
}
