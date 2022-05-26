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

	bool ImGui::Selectable2(ImGuiID id, bool selected, ImGuiSelectableFlags flags, const ImRect& rect)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		//const ImGuiStyle& style = g.Style;

		// Submit label or explicit size to ItemSize(), whereas ItemAdd() will submit a larger/spanning rectangle.
		//ImGuiID id = window->GetID(label);
		//ImVec2 label_size = CalcTextSize(label, NULL, true);
		//ImVec2 size = rect.GetSize(); // (size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
		//ImVec2 pos = rect.GetTL(); // window->DC.CursorPos;
		//pos.y += window->DC.CurrLineTextBaseOffset;

		//ItemSize(rect);
		//ItemSize(size, 0.0f);

		// Fill horizontal space
		// We don't support (size < 0.0f) in Selectable() because the ItemSpacing extension would make explicitly right-aligned sizes not visibly match other widgets.
		const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
		//const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
		//const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
		/*if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth))
			size.x = ImMax(label_size.x, max_x - min_x);*/

		// Text stays at the submission position, but bounding box may be extended on both sides
		//const ImVec2 text_min = pos;
		//const ImVec2 text_max(min_x + size.x, pos.y + size.y);

		//// Selectables are meant to be tightly packed together with no click-gap, so we extend their box to cover spacing between selectable.
		//ImRect bb(min_x, pos.y, text_max.x, text_max.y);
		//if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
		//{
		//	const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
		//	const float spacing_y = style.ItemSpacing.y;
		//	const float spacing_L = IM_FLOOR(spacing_x * 0.50f);
		//	const float spacing_U = IM_FLOOR(spacing_y * 0.50f);
		//	bb.Min.x -= spacing_L;
		//	bb.Min.y -= spacing_U;
		//	bb.Max.x += (spacing_x - spacing_L);
		//	bb.Max.y += (spacing_y - spacing_U);
		//}
		////if (g.IO.KeyCtrl) { GetForegroundDrawList()->AddRect(bb.Min, bb.Max, IM_COL32(0, 255, 0, 255)); }

		// Modify ClipRect for the ItemAdd(), faster than doing a PushColumnsBackground/PushTableBackground for every Selectable..
		const float backup_clip_rect_min_x = window->ClipRect.Min.x;
		const float backup_clip_rect_max_x = window->ClipRect.Max.x;
		if (span_all_columns)
		{
			window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
			window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
		}

		bool item_add;
		if (flags & ImGuiSelectableFlags_Disabled)
		{
			ImGuiItemFlags backup_item_flags = window->DC.ItemFlags;
			window->DC.ItemFlags |= ImGuiItemFlags_Disabled | ImGuiItemFlags_NoNavDefaultFocus;
			item_add = ItemAdd(rect, id);
			window->DC.ItemFlags = backup_item_flags;
		}
		else
		{
			item_add = ItemAdd(rect, id);
		}

		if (span_all_columns)
		{
			window->ClipRect.Min.x = backup_clip_rect_min_x;
			window->ClipRect.Max.x = backup_clip_rect_max_x;
		}

		if (!item_add)
			return false;

		// FIXME: We can standardize the behavior of those two, we could also keep the fast path of override ClipRect + full push on render only,
		// which would be advantageous since most selectable are not selected.
		if (span_all_columns && window->DC.CurrentColumns)
			PushColumnsBackground();
		else if (span_all_columns && g.CurrentTable)
			TablePushBackgroundChannel();

		// We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
		ImGuiButtonFlags button_flags = 0;
		if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
		if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
		if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
		if (flags & ImGuiSelectableFlags_Disabled) { button_flags |= ImGuiButtonFlags_Disabled; }
		if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
		if (flags & ImGuiSelectableFlags_AllowItemOverlap) { button_flags |= ImGuiButtonFlags_AllowItemOverlap; }

		if (flags & ImGuiSelectableFlags_Disabled)
			selected = false;

		const bool was_selected = selected;
		bool hovered, held;
		bool pressed = ButtonBehavior(rect, id, &hovered, &held, button_flags);

		// Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with gamepad/keyboard
		if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
		{
			if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
			{
				SetNavID(id, window->DC.NavLayerCurrent, window->DC.NavFocusScopeIdCurrent, ImRect(rect.Min - window->Pos, rect.Max - window->Pos));
				g.NavDisableHighlight = true;
			}
		}
		if (pressed)
			MarkItemEdited(id);

		if (flags & ImGuiSelectableFlags_AllowItemOverlap)
			SetItemAllowOverlap();

		// In this branch, Selectable() cannot toggle the selection so this will never trigger.
		if (selected != was_selected) //-V547
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

		// Render
		if (held && (flags & ImGuiSelectableFlags_DrawHoveredWhenHeld))
			hovered = true;
		if (hovered || selected)
		{
			const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			RenderFrame(rect.Min, rect.Max, col, false, 0.0f);
			RenderNavHighlight(rect, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
		}

		if (span_all_columns && window->DC.CurrentColumns)
			PopColumnsBackground();
		else if (span_all_columns && g.CurrentTable)
			TablePopBackgroundChannel();

		//if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
		//RenderTextClipped(text_min, text_max, label, NULL, &label_size, style.SelectableTextAlign, &bb);
		//if (flags & ImGuiSelectableFlags_Disabled) PopStyleColor();

		// Automatically close popups
		if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
			CloseCurrentPopup();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags);
		return pressed;
	}

	ImRect ImGui::GetLastItemRect()
	{
		ImGuiWindow* window = GetCurrentWindowRead();
		return window->DC.LastItemRect;
	}
}