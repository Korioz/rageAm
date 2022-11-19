#pragma once

#include "../Component.h"
#include "../ComponentMgr.h"
#include "../ImGui/ImGuiGta.h"
#include "../Memory/Hooking.h"
#include "../ImGui/Imgui_impl_gta.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplGta_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

extern std::shared_ptr<GtaCommon> g_gtaCommon;

class GtaWindow : public Component
{
	typedef LRESULT(*gDef_WndProc)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	typedef HWND(*gDef_CreateGameWindow)(__int64 a1);

	inline static gDef_WndProc gImpl_WndProc = nullptr;

	static LRESULT aImpl_WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		ImGui_ImplGta_WndProcHandler(hWnd, Msg, wParam, lParam);
		return gImpl_WndProc(hWnd, Msg, wParam, lParam);
	}

	HWND hWnd;

public:
	GtaWindow()
	{
		// CreateGameWindow
		// 40 55 41 54 41 55 41 56 41 57 48 81 EC D0 00 00 00 48 8D 6C 24 60

		const intptr_t createWndAndGfx = g_gtaCommon->gPtr_CreateGameWindowAndGraphics;

		hWnd = *g_hook->FindOffset<HWND*>("CreateGameWindowAndGraphics_hWnd", createWndAndGfx + 0x46A + 0x3);

		g_hook->SetHook(0x7FF71FBEB6E4, &aImpl_WndProc, &gImpl_WndProc);

		g_logger->Log(std::format("GTA5.exe hWND: {:x}",(int) hWnd));
	}

	HWND GetHwnd() const
	{
		return hWnd;
	}
};