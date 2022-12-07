#include "gmHook.h"

void gm::gmHook::SetHook_Internal(LPVOID func, LPVOID detour, LPVOID* orig)
{
	MH_STATUS hookStatus = MH_CreateHook(func, detour, orig);
	MH_STATUS enableStatus = MH_EnableHook(func);

	if (hookStatus != MH_OK || enableStatus != MH_OK)
	{
		g_Log.LogE("gmHook::SetHook({:X}) -> Failed. Hook Status: {}. Enable Status: {}",
			reinterpret_cast<uintptr_t>(func),
			static_cast<int>(hookStatus),
			static_cast<int>(enableStatus));
		return;
	}
	g_Log.LogT("gmHook::SetHook({:X})", reinterpret_cast<uintptr_t>(func));
	m_hookedFuncs.emplace(func);
}

gm::gmHook::gmHook()
{
	MH_STATUS status = MH_Initialize();

	if (status == MH_OK)
	{
		g_Log.LogT("gmHook::gmHook() -> MinHook initialized with status OK");
		return;
	}

	g_Log.LogE("gmHook::gmHook() -> MinHook failed to initialize with status {}", static_cast<int>(status));
}

gm::gmHook::~gmHook()
{
	g_Log.LogT("gmHook::~gmHook()");

	for (auto& func : m_hookedFuncs)
	{
		MH_DisableHook(func);
	}
	m_hookedFuncs.clear();

	MH_Uninitialize();
}

void gm::gmHook::UnHook(LPVOID addr)
{
	MH_DisableHook(addr);
	m_hookedFuncs.erase(addr);
}