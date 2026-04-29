/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspace_row_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_generated_resources.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/menu_source_type.mojom.h"
#include "ui/color/color_id.h"
#include "ui/gfx/font.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/menu/menu_runner.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"

namespace {

constexpr int kDeleteWorkspaceCommandId = 1;

std::u16string FormatStats(const WorkspaceMetadata& info) {
  if (info.number_of_windows > 1) {
    return base::UTF8ToUTF16(absl::StrFormat(
        "%d windows - %d %s", info.number_of_windows, info.number_of_tabs,
        info.number_of_tabs == 1 ? "tab" : "tabs"));
  }
  return base::UTF8ToUTF16(absl::StrFormat(
      "%d %s", info.number_of_tabs, info.number_of_tabs == 1 ? "tab" : "tabs"));
}

// Clickable two-row view: bold workspace name on top, stats below.
class WorkspaceInfoButton : public views::Button {
  METADATA_HEADER(WorkspaceInfoButton, views::Button)
 public:
  WorkspaceInfoButton(PressedCallback callback, const WorkspaceMetadata& info)
      : Button(std::move(callback)) {
    auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical,
        /*inside_border_insets=*/gfx::Insets::VH(4, 8),
        /*between_child_spacing=*/2));
    layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStart);

    name_label_ = AddChildView(
        std::make_unique<views::Label>(base::UTF8ToUTF16(info.name)));
    name_label_->SetFontList(
        name_label_->font_list().DeriveWithSizeDelta(2).DeriveWithWeight(
            gfx::Font::Weight::BOLD));
    name_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);

    stats_label_ =
        AddChildView(std::make_unique<views::Label>(FormatStats(info)));
    stats_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  }

  void SetSelected(bool selected) {
    ui::ColorVariant color = selected
                                 ? ui::ColorVariant(SK_ColorWHITE)
                                 : ui::ColorVariant(ui::kColorLabelForeground);
    name_label_->SetEnabledColor(color);
    stats_label_->SetEnabledColor(color);
  }

 private:
  raw_ptr<views::Label> name_label_;
  raw_ptr<views::Label> stats_label_;
};

BEGIN_METADATA(WorkspaceInfoButton)
END_METADATA

}  // namespace

WorkspaceRowView::WorkspaceRowView(
    const WorkspaceMetadata& info,
    WorkspaceRowClickedCallback on_workspace_selected,
    WorkspaceRowClickedCallback on_delete_clicked)
    : on_delete_(std::move(on_delete_clicked)) {
  SetNotifyEnterExitOnChild(true);

  auto row_box = std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal, gfx::Insets(), 0);
  row_box->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);
  auto* row_layout = row_box.get();

  SetLayoutManager(std::move(row_box));

  auto info_btn = std::make_unique<WorkspaceInfoButton>(
      std::move(on_workspace_selected), info);
  auto* info_btn_raw = AddChildView(std::move(info_btn));
  row_layout->SetFlexForView(info_btn_raw, 1);

  auto more_btn = views::CreateVectorImageButtonWithNativeTheme(
      base::BindRepeating(&WorkspaceRowView::ShowMoreMenu,
                          base::Unretained(this)),
      kLeoMoreVerticalIcon);
  more_btn->SetPreferredSize(gfx::Size(27, 27));
  more_btn->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_WORKSPACE_ROW_MORE_BUTTON_TOOLTIP));
  more_btn->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(0, 0, 0, 5));
  more_button_ = AddChildView(std::move(more_btn));
}

WorkspaceRowView::~WorkspaceRowView() = default;

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
    if (auto* info_btn = views::AsViewClass<WorkspaceInfoButton>(child)) {
      info_btn->SetSelected(selected_);
    } else if (auto* img_btn = views::AsViewClass<views::ImageButton>(child)) {
      views::SetImageFromVectorIconWithColor(
          img_btn, kLeoMoreVerticalIcon,
          selected_
              ? views::IconColors(SK_ColorWHITE, SK_ColorWHITE)
              : views::IconColors(ui::kColorIcon, ui::kColorIconDisabled));
    }
  }
}

void WorkspaceRowView::ShowMoreMenu() {
  menu_model_ = std::make_unique<ui::SimpleMenuModel>(this);
  menu_model_->AddItemWithStringId(kDeleteWorkspaceCommandId,
                                   IDS_WORKSPACE_ROW_MENU_DELETE_WORKSPACE);
  menu_runner_ = std::make_unique<views::MenuRunner>(
      menu_model_.get(), views::MenuRunner::HAS_MNEMONICS);
  menu_runner_->RunMenuAt(GetWidget(), /*button_controller=*/nullptr,
                          more_button_->GetBoundsInScreen(),
                          views::MenuAnchorPosition::kTopRight,
                          ui::mojom::MenuSourceType::kMouse);
}

void WorkspaceRowView::ExecuteCommand(int command_id, int event_flags) {
  if (command_id == kDeleteWorkspaceCommandId) {
    on_delete_.Run();
  }
}

BEGIN_METADATA(WorkspaceRowView)
END_METADATA
