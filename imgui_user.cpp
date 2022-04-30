#include "imgui.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include "imgui_user.h"

// System includes
#include <ctype.h>      // toupper
#include <stdio.h>      // vsnprintf, sscanf, printf
#if defined(_MSC_VER) && _MSC_VER <= 1500 // MSVC 2008 or earlier
#include <stddef.h>     // intptr_t
#else
#include <stdint.h>     // intptr_t
#endif

namespace ImGui
{
	ImVec2 ImGui::GetLineStart()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		
		return ImVec2(window->Pos.x - window->Scroll.x + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x, window->DC.CursorPosPrevLine.y);
	}

	ImVec2 ImGui::GetRemainingSpace()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		return window->DC.CursorMaxPos - window->DC.CursorPos;
	}

	ImVec2 ImGui::GetCurrentLineSize()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		return window->DC.CurrLineSize;
	}

	ImVec2 ImGui::GetCursorMaxPos()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		return window->DC.CursorMaxPos;
	}

	void ImGui::SetCurrentLineSize(const ImVec2& size)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		window->DC.CurrLineSize = size;
	}

	void ImGui::ResetLine(float offset_x, float offset_y)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		
		/*window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + offset_x + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
		window->DC.CursorPos.y = window->Pos.y - offset_y;*/

		//window->DC.CursorPos.x = window->DC.CursorPosPrevLine.x + offset_x;
		window->DC.CursorPos.x = window->Pos.x - window->Scroll.x + offset_x + window->DC.GroupOffset.x + window->DC.ColumnsOffset.x;
		window->DC.CursorPos.y = window->DC.CursorPosPrevLine.y + offset_y;

		window->DC.CurrLineSize = window->DC.PrevLineSize - ImVec2(offset_x, offset_y);
		window->DC.CurrLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
	}


	void ImGui::NewLine2(float height)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) return;

		const ImGuiLayoutType backup_layout_type = window->DC.LayoutType;
		window->DC.LayoutType = ImGuiLayoutType_Vertical;
		ItemSize(ImVec2(0, height));
		window->DC.LayoutType = backup_layout_type;
	}

	void ImGui::BeginGroup2(const ImVec2& size)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		g.GroupStack.resize(g.GroupStack.Size + 1);
		ImGuiGroupData& group_data = g.GroupStack.back();
		group_data.WindowID = window->ID;
		group_data.BackupCursorPos = window->DC.CursorPos;
		group_data.BackupCursorMaxPos = window->DC.CursorMaxPos;
		group_data.BackupIndent = window->DC.Indent;
		group_data.BackupGroupOffset = window->DC.GroupOffset;
		group_data.BackupCurrLineSize = window->DC.CurrLineSize;
		group_data.BackupCurrLineTextBaseOffset = window->DC.CurrLineTextBaseOffset;
		group_data.BackupActiveIdIsAlive = g.ActiveIdIsAlive;
		group_data.BackupHoveredIdIsAlive = g.HoveredId != 0;
		group_data.BackupActiveIdPreviousFrameIsAlive = g.ActiveIdPreviousFrameIsAlive;
		group_data.EmitItem = true;

		window->DC.GroupOffset.x = window->DC.CursorPos.x - window->Pos.x - window->DC.ColumnsOffset.x;
		window->DC.Indent = window->DC.GroupOffset;
		window->DC.CursorMaxPos = window->DC.CursorPos + size;
		window->DC.CurrLineSize = ImVec2(0.0f, 0.0f);
		if (g.LogEnabled)
			g.LogLinePosY = -FLT_MAX; // To enforce a carriage return
	}

	void ImGui::EndGroup2()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		IM_ASSERT(g.GroupStack.Size > 0); // Mismatched BeginGroup()/EndGroup() calls

		ImGuiGroupData& group_data = g.GroupStack.back();
		IM_ASSERT(group_data.WindowID == window->ID); // EndGroup() in wrong window?

		ImRect group_bb(group_data.BackupCursorPos, ImMax(window->DC.CursorMaxPos, group_data.BackupCursorPos));

		window->DC.CursorPos = group_data.BackupCursorPos;
		window->DC.CursorMaxPos = ImMax(group_data.BackupCursorMaxPos, window->DC.CursorMaxPos);
		window->DC.Indent = group_data.BackupIndent;
		window->DC.GroupOffset = group_data.BackupGroupOffset;
		window->DC.CurrLineSize = group_data.BackupCurrLineSize;
		window->DC.CurrLineTextBaseOffset = group_data.BackupCurrLineTextBaseOffset;
		if (g.LogEnabled)
			g.LogLinePosY = -FLT_MAX; // To enforce a carriage return

		if (!group_data.EmitItem)
		{
			g.GroupStack.pop_back();
			return;
		}

		window->DC.CurrLineTextBaseOffset = ImMax(window->DC.PrevLineTextBaseOffset, group_data.BackupCurrLineTextBaseOffset);      // FIXME: Incorrect, we should grab the base offset from the *first line* of the group but it is hard to obtain now.
		ItemSize(group_bb.GetSize());
		ItemAdd(group_bb, 0);

		// If the current ActiveId was declared within the boundary of our group, we copy it to LastItemId so IsItemActive(), IsItemDeactivated() etc. will be functional on the entire group.
		// It would be be neater if we replaced window.DC.LastItemId by e.g. 'bool LastItemIsActive', but would put a little more burden on individual widgets.
		// Also if you grep for LastItemId you'll notice it is only used in that context.
		// (The two tests not the same because ActiveIdIsAlive is an ID itself, in order to be able to handle ActiveId being overwritten during the frame.)
		const bool group_contains_curr_active_id = (group_data.BackupActiveIdIsAlive != g.ActiveId) && (g.ActiveIdIsAlive == g.ActiveId) && g.ActiveId;
		const bool group_contains_prev_active_id = (group_data.BackupActiveIdPreviousFrameIsAlive == false) && (g.ActiveIdPreviousFrameIsAlive == true);
		if (group_contains_curr_active_id)
			window->DC.LastItemId = g.ActiveId;
		else if (group_contains_prev_active_id)
			window->DC.LastItemId = g.ActiveIdPreviousFrame;
		window->DC.LastItemRect = group_bb;

		// Forward Hovered flag
		const bool group_contains_curr_hovered_id = (group_data.BackupHoveredIdIsAlive == false) && g.HoveredId != 0;
		if (group_contains_curr_hovered_id)
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HoveredWindow;

		// Forward Edited flag
		if (group_contains_curr_active_id && g.ActiveIdHasBeenEditedThisFrame)
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Edited;

		// Forward Deactivated flag
		window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDeactivated;
		if (group_contains_prev_active_id && g.ActiveId != g.ActiveIdPreviousFrame)
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_Deactivated;

		g.GroupStack.pop_back();
		//window->DrawList->AddRect(group_bb.Min, group_bb.Max, IM_COL32(255,0,255,255));   // [Debug]
	}


	void ImGui::TextEx2(const char* text, const char* text_end, ImGuiTextFlags flags, ImFont* font, float font_size, ImU32 color, ImU32 color_bg, ImVec2 offset_bg)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext& g = *GImGui;
		IM_ASSERT(text != NULL);
		const char* text_begin = text;
		if (text_end == NULL)
			text_end = text + strlen(text); // FIXME-OPT

		const bool has_bg = offset_bg.x != 0.00f || offset_bg.y != 0.00f;

		const ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
		const float wrap_pos_x = window->DC.TextWrapPos;
		const bool wrap_enabled = (wrap_pos_x >= 0.0f);
		if (text_end - text > 2000 && !wrap_enabled)
		{
			// Long text!
			// Perform manual coarse clipping to optimize for long multi-line text
			// - From this point we will only compute the width of lines that are visible. Optimization only available when word-wrapping is disabled.
			// - We also don't vertically center the text within the line full height, which is unlikely to matter because we are likely the biggest and only item on the line.
			// - We use memchr(), pay attention that well optimized versions of those str/mem functions are much faster than a casually written loop.
			const char* line = text;
			const float line_height = font_size; // GetTextLineHeight();
			ImVec2 text_size(0, 0);

			// Lines to skip (can't skip when logging text)
			ImVec2 pos = text_pos;
			if (!g.LogEnabled)
			{
				int lines_skippable = (int)((window->ClipRect.Min.y - text_pos.y) / line_height);
				if (lines_skippable > 0)
				{
					int lines_skipped = 0;
					while (line < text_end && lines_skipped < lines_skippable)
					{
						const char* line_end = (const char*)memchr(line, '\n', text_end - line);
						if (!line_end)
							line_end = text_end;
						if ((flags & ImGuiTextFlags_NoWidthForLargeClippedText) == 0)
							text_size.x = ImMax(text_size.x, CalcTextSize2(line, line_end, false, -1, font_size, font).x);
						line = line_end + 1;
						lines_skipped++;
					}
					pos.y += lines_skipped * line_height;
				}
			}

			// Lines to render
			if (line < text_end)
			{
				ImRect line_rect(pos, pos + ImVec2(FLT_MAX, line_height));
				while (line < text_end)
				{
					if (IsClippedEx(line_rect, 0, false))
						break;

					const char* line_end = (const char*)memchr(line, '\n', text_end - line);
					if (!line_end)
						line_end = text_end;
					text_size.x = ImMax(text_size.x, CalcTextSize2(line, line_end, false, -1, font_size, font).x);

					if (has_bg) RenderText2(pos + offset_bg, line, line_end, false, font, font_size, color_bg);
					RenderText2(pos, line, line_end, false, font, font_size, color);

					line = line_end + 1;
					line_rect.Min.y += line_height;
					line_rect.Max.y += line_height;
					pos.y += line_height;
				}

				// Count remaining lines
				int lines_skipped = 0;
				while (line < text_end)
				{
					const char* line_end = (const char*)memchr(line, '\n', text_end - line);
					if (!line_end)
						line_end = text_end;
					if ((flags & ImGuiTextFlags_NoWidthForLargeClippedText) == 0)
						text_size.x = ImMax(text_size.x, CalcTextSize2(line, line_end, false, -1, font_size, font).x);
					line = line_end + 1;
					lines_skipped++;
				}
				pos.y += lines_skipped * line_height;
			}
			text_size.y = (pos - text_pos).y;

			ImRect bb(text_pos, text_pos + text_size);
			ItemSize(text_size, 0.0f);
			ItemAdd(bb, 0);
		}
		else
		{
			const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
			const ImVec2 text_size = CalcTextSize2(text_begin, text_end, false, wrap_width, font_size, font);

			ImRect bb(text_pos, text_pos + text_size);
			ItemSize(text_size, 0.0f);
			if (!ItemAdd(bb, 0))
				return;

			if (has_bg) RenderTextWrapped2(bb.Min + offset_bg, text_begin, text_end, wrap_width, font, font_size, color_bg);
			RenderTextWrapped2(bb.Min, text_begin, text_end, wrap_width, font, font_size, color);
		}
	}

	void ImGui::RenderTextWrapped2(ImVec2 pos, const char* text, const char* text_end, float wrap_width, ImFont* font, float font_size, ImU32 color)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		if (!text_end)
			text_end = text + strlen(text); // FIXME-OPT

		if (text != text_end)
		{
			window->DrawList->AddText(font, font_size, pos, color, text, text_end, wrap_width);
			if (g.LogEnabled)
				LogRenderedText(&pos, text, text_end);
		}
	}

	ImVec2 ImGui::CalcTextSize2(const char* text, const char* text_end, bool hide_text_after_double_hash, float wrap_width, float font_size, ImFont* font)
	{
		ImGuiContext& g = *GImGui;

		const char* text_display_end;
		if (hide_text_after_double_hash)
			text_display_end = FindRenderedTextEnd(text, text_end);      // Hide anything after a '##' string
		else
			text_display_end = text_end;

		if (text == text_display_end)
			return ImVec2(0.0f, font_size);
		ImVec2 text_size = font->CalcTextSizeA2(font_size, FLT_MAX, wrap_width, text, text_display_end, NULL);

		// Round
		// FIXME: This has been here since Dec 2015 (7b0bf230) but down the line we want this out.
		// FIXME: Investigate using ceilf or e.g.
		// - https://git.musl-libc.org/cgit/musl/tree/src/math/ceilf.c
		// - https://embarkstudios.github.io/rust-gpu/api/src/libm/math/ceilf.rs.html
		text_size.x = IM_FLOOR(text_size.x + 0.99999f);

		return text_size;
	}

	void ImGui::RenderText2(ImVec2 pos, const char* text, const char* text_end, bool hide_text_after_hash, ImFont* font, float font_size, ImU32 color)
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		// Hide anything after a '##' string
		const char* text_display_end;
		if (hide_text_after_hash)
		{
			text_display_end = FindRenderedTextEnd(text, text_end);
		}
		else
		{
			if (!text_end)
				text_end = text + strlen(text); // FIXME-OPT
			text_display_end = text_end;
		}

		if (text != text_display_end)
		{
			window->DrawList->AddText(font, font_size, pos, color, text, text_display_end);
			if (g.LogEnabled)
				LogRenderedText(&pos, text, text_display_end);
		}
	}
}