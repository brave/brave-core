/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspace/workspace_row_view.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_utils.h"

WorkspaceRowView::WorkspaceRowView(
    const int dialog_width,
    const int padding,
    const int row_height,
    const WorkspaceInfo& info,
    WorkspaceRowClickedCallback OnWorkspaceSelected,
    WorkspaceRowClickedCallback OnDeleteClicked)
    : kDialogWidth(dialog_width), kPadding(padding), kRowHeight(row_height) {
  SetNotifyEnterExitOnChild(true);

  auto row_box = std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 0);
  row_box->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  auto* row_layout = row_box.get();

  SetLayoutManager(std::move(row_box));
  SetPreferredSize(gfx::Size(kRowContentWidth, kRowHeight));
  FormatStats(info);

  auto name_btn = std::make_unique<views::LabelButton>(
      OnWorkspaceSelected,
      base::UTF8ToUTF16(info.name) + u"  " + workspace_stats_text_);
  name_btn->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  auto* name_btn_raw = AddChildView(std::move(name_btn));
  row_layout->SetFlexForView(name_btn_raw, 1);

  auto del_btn = views::CreateVectorImageButtonWithNativeTheme(OnDeleteClicked,
                                                               kTrashCanIcon);
  del_btn->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_OPEN_DIALOG_ROW_DELETE_BUTTON));

  AddChildView(std::move(del_btn));
}

void WorkspaceRowView::SetSelected(bool selected) {
  if (selected_ == selected) {
    return;
  }
  selected_ = selected;
  UpdateBackground();
}

void WorkspaceRowView::OnMouseEntered(const ui::MouseEvent& event) {
  views::View::OnMouseEntered(event);
  hovered_ = true;
  UpdateBackground();
}

void WorkspaceRowView::OnMouseExited(const ui::MouseEvent& event) {
  views::View::OnMouseExited(event);
  hovered_ = false;
  UpdateBackground();
}

void WorkspaceRowView::FormatStats(const WorkspaceInfo& info) {
  std::u16string stats_text;
  if (info.number_of_windows > 1) {
    workspace_stats_text_ = base::UTF8ToUTF16(absl::StrFormat(
        "%d windows - %d ", info.number_of_windows, info.number_of_tabs));
  } else {
    workspace_stats_text_ =
        base::UTF8ToUTF16(absl::StrFormat("%d ", info.number_of_tabs));
  }
  workspace_stats_text_ +=
      base::UTF8ToUTF16((info.number_of_tabs == 1 ? "tab" : "tabs"));
}

void WorkspaceRowView::UpdateBackground() {
  if (selected_) {
    SetBackground(
        views::CreateSolidBackground(ui::kColorSysOnSurfaceSecondary));
  } else if (hovered_) {
    SetBackground(
        views::CreateSolidBackground(ui::kColorSysStateHoverOnSubtle));
  } else {
    SetBackground(nullptr);
  }
  for (views::View* child : children()) {
    if (auto* btn = views::AsViewClass<views::LabelButton>(child)) {
      btn->SetEnabledTextColors(
          selected_ ? std::optional<ui::ColorVariant>(SK_ColorWHITE)
                    : std::nullopt);
    } else if (auto* img_btn = views::AsViewClass<views::ImageButton>(child)) {
      views::SetImageFromVectorIconWithColor(
          img_btn, kTrashCanIcon,
          selected_
              ? views::IconColors(SK_ColorWHITE, SK_ColorWHITE)
              : views::IconColors(ui::kColorIcon, ui::kColorIconDisabled));
    }
  }
}

BEGIN_METADATA(WorkspaceRowView)
END_METADATA
