#include "gamedrawlists.h"

#ifdef AM_INTEGRATED

#include "memory/address.h"
#include "memory/hook.h"

#include <easy/profiler.h>

namespace
{
	std::atomic_bool s_ShuttingDown;
	gmAddress		 s_RenderMarkersAddr;
}

void DrawListExecuteCommand()
{
	rageam::integration::GameDrawLists::GetInstance()->ExecuteAll();
}

// This function is a good fit for us, the same function is used to batch lines from DRAW_LINE native
// Called after PostFX pass, no AA
void (*gImpl_CVisualEffects_RenderMarkers)(u32 mask);
void aImpl_CVisualEffects_RenderMarkers(u32 mask)
{
	gImpl_CVisualEffects_RenderMarkers(mask);

	if (s_ShuttingDown)
	{
		Hook::Remove(s_RenderMarkersAddr);
		s_ShuttingDown = false;
		return;
	}

	// rage::DLC_AddInternal<void(*)>(void(*), const char* name)>
	static auto dlc_AddInternal = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"75 AF 48 83 C4 38", "rage::DLC_AddInternal<void(*)(void)>+0x5D")
		.GetAt(-0x5D)
#else
		"81 20 04 20 F3 FF", "rage::DLC_AddInternal<void(*)(void)>+0x2D")
		.GetAt(-0x2D)
#endif
		.ToFunc<void(void(*)(), const char*)>();
	dlc_AddInternal(DrawListExecuteCommand, "DrawListExecuteCommand");
}

rageam::integration::GameDrawLists::GameDrawLists() :
	Game("Game", 0x1000, 0x1000),
	GameUnlit("Game Unlit", 0x1000, 0x1000),
	Overlay("Overlay (Foreground)", 0x1000, 0x1000),
	CollisionMesh("Collision Mesh", 0x10000, 0x10000), // Collision need much larger buffer
	Gizmo("Gizmo", 0x1000, 0x1000)
{
	GameUnlit.Unlit = true;
	Overlay.NoDepth = true;
	Overlay.Unlit = true;
	CollisionMesh.Wireframe = true;
	Gizmo.NoDepth = true;
	Gizmo.BackfaceCull = false;
	Gizmo.Unlit = true; // TODO: For now, plane gizmo have shading issues on back side because of shading

	s_RenderMarkersAddr = gmAddress::Scan(
#if APP_BUILD_2699_16_RELEASE_NO_OPT
		"83 E0 08 85 C0 74 1F E8", "CVisualEffects::RenderMarkers+0x5E").GetAt(-0x5E);
#else
		"F6 C3 04 74 05", "CVisualEffects::RenderMarkers+0x2F").GetAt(-0x2F);
#endif
	Hook::Create(s_RenderMarkersAddr, aImpl_CVisualEffects_RenderMarkers, &gImpl_CVisualEffects_RenderMarkers);
}

rageam::integration::GameDrawLists::~GameDrawLists()
{
	// NOTE: This will hang if called during game pause because it skips building scene draw list,
	// but we can't risk game calling draw command after we unloaded & crashing the game
	s_ShuttingDown = true;
	while (s_ShuttingDown) {}
}

void rageam::integration::GameDrawLists::EndFrame()
{
	EASY_BLOCK("GameDrawLists::EndFrame");
	Game.EndFrame();
	GameUnlit.EndFrame();
	Overlay.EndFrame();
	CollisionMesh.EndFrame();
	Gizmo.EndFrame();
}

void rageam::integration::GameDrawLists::ExecuteAll()
{
	EASY_BLOCK("GameDrawLists::ExecuteAll");
	m_Executor.Execute(Game);
	m_Executor.Execute(GameUnlit);
	m_Executor.Execute(Overlay);
	m_Executor.Execute(CollisionMesh);
	m_Executor.Execute(Gizmo);
}
#endif
