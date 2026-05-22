/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/workspaces/workspaces_bubble_view.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "brave/browser/workspaces/workspace_service.h"
#include "brave/browser/workspaces/workspace_service_factory.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/mojom/dialog_button.mojom.h"
#include "ui/gfx/font.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/separator.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/view_class_properties.h"

// static
void WorkspacesBubbleView::Show(views::View* anchor_view, Profile* profile) {
  views::Widget* widget = views::BubbleDialogDelegateView::CreateBubble(
      std::make_unique<WorkspacesBubbleView>(anchor_view, profile));
  widget->Show();
}

WorkspacesBubbleView::WorkspacesBubbleView(views::View* anchor_view,
                                           Profile* profile)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_LEFT) {
  SetButtons(static_cast<int>(ui::mojom::DialogButton::kNone));
  SetShowCloseButton(false);
  set_margins(gfx::Insets::VH(8, 15));
  set_fixed_width(253);

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kVertical,
      /*inside_border_insets=*/gfx::Insets(),
      /*between_child_spacing=*/0));

  // Header row: "SPACES" label + new workspace button.
  auto* header_row = AddChildView(std::make_unique<views::View>());
  auto* row_layout =
      header_row->SetLayoutManager(std::make_unique<views::BoxLayout>(
          views::BoxLayout::Orientation::kHorizontal,
          /*inside_border_insets=*/gfx::Insets(),
          /*between_child_spacing=*/8));
  row_layout->set_cross_axis_alignment(
      views::BoxLayout::CrossAxisAlignment::kCenter);

  auto* spaces_label = header_row->AddChildView(std::make_unique<views::Label>(
      l10n_util::GetStringUTF16(IDS_WORKSPACES_BUBBLE_TITLE)));
  spaces_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  row_layout->SetFlexForView(spaces_label, 1);

  auto* add_button =
      header_row->AddChildView(views::CreateVectorImageButtonWithNativeTheme(
          views::Button::PressedCallback(), kLeoPlusAddCircleIcon,
          /*dip_size=*/16));
  add_button->SetPreferredSize(gfx::Size(19, 19));
  add_button->SetTooltipText(u"New workspace");

  auto* separator = AddChildView(std::make_unique<views::Separator>());
  separator->SetProperty(views::kMarginsKey, gfx::Insets::TLBR(8, 0, 4, 0));

  auto* service = WorkspaceServiceFactory::GetForProfile(profile);
  if (!service) {
    return;
  }

  for (const auto& workspace : service->ListWorkspaces()) {
    auto* row = AddChildView(std::make_unique<views::View>());
    row->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical,
        /*inside_border_insets=*/gfx::Insets::VH(4, 0),
        /*between_child_spacing=*/2));

    auto* name_label = row->AddChildView(
        std::make_unique<views::Label>(base::UTF8ToUTF16(workspace.name)));
    name_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    name_label->SetFontList(
        name_label->font_list().DeriveWithWeight(gfx::Font::Weight::BOLD));

    const std::string subtitle =
        absl::StrFormat("%d %s - %d %s", workspace.number_of_windows,
                        workspace.number_of_windows == 1 ? "window" : "windows",
                        workspace.number_of_tabs,
                        workspace.number_of_tabs == 1 ? "tab" : "tabs");
    auto* subtitle_label = row->AddChildView(
        std::make_unique<views::Label>(base::UTF8ToUTF16(subtitle)));
    subtitle_label->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  }
}

BEGIN_METADATA(WorkspacesBubbleView)
END_METADATA
