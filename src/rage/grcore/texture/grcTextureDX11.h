#pragma once

#include <d3d11.h>

#include "grcTexturePC.h"
#include "TextureUtils.h"
#include "paging/datResource.h"

namespace rage
{
	class grcTextureDX11 : public grcTexturePC
	{
		static constexpr DXGI_FORMAT GRC_TX_FALLBACK_FORMAT = DXGI_FORMAT_R8G8B8A8_UINT;

		static void GetUsageAndAccessFlags(u8 createFlags, D3D11_USAGE& usage, UINT& access);

		u8 GetD3DArraySize() const;

		// Tries to convert format from legacy to DXGI. If unsuccessful, uses R8G8B8A8.
		void ConvertFormat();

		bool ComputePitch(u8 mip, u32* pRowPitch, u32* pSlicePitch) const;

		D3D11_TEXTURE3D_DESC GetDesk3D(u8 createFlags, u8 bindFlags) const;
		D3D11_TEXTURE2D_DESC GetDesc2D(u8 createFlags, u8 bindFlags) const;
		void GetViewDesc3D(D3D11_SHADER_RESOURCE_VIEW_DESC& viewDesk) const;
		void GetViewDesc2D(D3D11_SHADER_RESOURCE_VIEW_DESC& viewDesk) const;

		// Creates resource and shader view.
		HRESULT CreateFromData(std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> subData, u8 createFlags, u8 cacheFlags, D3D11_BIND_FLAG bindFlags);

		// Gets sub resource data to initialize texture with.
		std::shared_ptr<D3D11_SUBRESOURCE_DATA[]> GetInitialData() const;

		// Initializes texture from parsed resource data.
		void Init();
	public:
		grcTextureDX11(const datResource& rsc);

		void* GetResourceView() const override { return m_ShaderView; }

		ID3D11ShaderResourceView* GetShaderResourceView() const;

	private:
		u8* m_TextureData;
		ID3D11ShaderResourceView* m_ShaderView;
		int64_t m_CacheEntry;
		int32_t unk88;
	};

	static_assert(sizeof(grcTextureDX11) == 0x90);
}