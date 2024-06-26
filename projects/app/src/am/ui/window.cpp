#include "window.h"

#include "extensions.h"
#include "slwidgets.h"

void rageam::ui::WindowManager::OnRender()
{
	// Add pending windows
	for (WindowPtr& window : m_WindowsToAdd)
	{
		m_Windows.Emplace(std::move(window));
	}
	m_WindowsToAdd.Clear();

	// Update existing windows
	for (const WindowPtr& window : m_Windows)
	{
		bool hasMenu = window->HasMenu();
		bool isDisabled = window->IsDisabled();
		bool isOpen = true;

		ImGuiWindowFlags windowFlags = 0;
		if (hasMenu)
			windowFlags |= ImGuiWindowFlags_MenuBar;

		if (isDisabled) ImGui::BeginDisabled();

		// Setup window start size and position
		ImVec2 defaultSize = window->GetDefaultSize();
		ImGui::SetNextWindowSize(defaultSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

		// Format window title with optional '*' for unsaved documents
		bool unsaved = window->ShowUnsavedChangesInTitle() && window->Undo.HasChanges();
		ConstString title = ImGui::FormatTemp("%s%s###%s",
			window->GetTitle(), unsaved ? "*" : "", window->GetID());

		if (!window->Padding()) ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(1, 1));
		bool begin = ImGui::Begin(title, &isOpen, windowFlags);
		if (!window->Padding()) ImGui::PopStyleVar(); // Window_Padding
		if (begin)
		{
			UndoStack::Push(window->Undo);
			if (!window->m_Started)
			{
				window->OnStart();
				window->m_Started = true;
			}
			if (hasMenu)
				window->OnMenuRender();
			window->OnRender();

			ImGui::HandleUndoHotkeys();

			UndoStack::Pop();
		}
		ImGui::End();

		if (isDisabled) ImGui::EndDisabled();

		if (!isOpen)
			m_WindowsToRemove.Add(window);
	}

	// Remove windows that been closed
	for (WindowPtr& window : m_WindowsToRemove)
		m_Windows.Remove(window);
	m_WindowsToRemove.Destroy();
}

rageam::ui::WindowPtr rageam::ui::WindowManager::Add(Window* window)
{
	return m_WindowsToAdd.Emplace(amPtr<Window>(window));
}

void rageam::ui::WindowManager::Close(const WindowPtr& ptr)
{
	if (ptr)
		m_WindowsToRemove.Add(ptr);
}

void rageam::ui::WindowManager::Focus(const WindowPtr& window) const
{
	ConstString title = ImGui::FormatTemp("###%s", window->GetTitle());
	ImGui::SetWindowFocus(title);
}

rageam::ui::WindowPtr rageam::ui::WindowManager::FindByTitle(ConstString title) const
{
	for (WindowPtr& window : m_Windows)
	{
		if (String::Equals(window->GetTitle(), title))
			return window;
	}
	return nullptr;
}
