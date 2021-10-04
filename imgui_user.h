
#include "imgui.h"
#include "cimgui.h"

namespace ImGui
{
	IMGUI_API void          BeginGroup2(const ImVec2& size);
	IMGUI_API void          EndGroup2();

	IMGUI_API ImVec2		GetLineStart();
	IMGUI_API ImVec2        GetRemainingSpace();
	IMGUI_API ImVec2        GetCurrentLineSize();
	IMGUI_API void          SetCurrentLineSize(const ImVec2& size);
	IMGUI_API void          ResetLine(float offset_x, float offset_y);
	IMGUI_API void          NewLine2(float height);

	IMGUI_API void          RenderTextWrapped2(ImVec2 pos, const char* text, const char* text_end, float wrap_width, ImFont* font, float font_size, ImU32 color);
	IMGUI_API void          RenderText2(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash, ImFont* font, float font_size, ImU32 color);

	IMGUI_API void          TextEx2(const char* text, const char* text_end, ImGuiTextFlags flags, ImFont* font, float font_size, ImU32 color, ImU32 color_bg, ImVec2 offset_bg);
}

CIMGUI_API void igBeginGroup2(const ImVec2 size)
{
	return ImGui::BeginGroup2(size);
}

CIMGUI_API void igEndGroup2()
{
	return ImGui::EndGroup2();
}

CIMGUI_API void igResetLine(float offset_x, float offset_y)
{
	return ImGui::ResetLine(offset_x, offset_y);
}

CIMGUI_API void igNewLine2(float height)
{
	return ImGui::NewLine2(height);
}

CIMGUI_API void igTextEx2(const char* text, const char* text_end, ImGuiTextFlags flags, ImFont* font, float font_size, ImU32 color, ImU32 color_bg, ImVec2 offset_bg)
{
	return ImGui::TextEx2(text, text_end, flags, font, font_size, color, color_bg, offset_bg);
}

CIMGUI_API void igGetLineStart(ImVec2* pOut)
{
	*pOut = ImGui::GetLineStart();
}

CIMGUI_API void igGetRemainingSpace(ImVec2* pOut)
{
	*pOut = ImGui::GetRemainingSpace();
}

CIMGUI_API void igGetCurrentLineSize(ImVec2* pOut)
{
	*pOut = ImGui::GetCurrentLineSize();
}

CIMGUI_API void igSetCurrentLineSize(const ImVec2 size)
{
	return ImGui::SetCurrentLineSize(size);
}
