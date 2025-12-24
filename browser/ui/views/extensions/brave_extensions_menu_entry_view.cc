// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/extensions/brave_extensions_menu_entry_view.h"

#include "chrome/browser/ui/views/controls/hover_button.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/toggle_button.h"
#include "ui/views/controls/highlight_path_generator.h"

BraveExtensionsMenuEntryView::BraveExtensionsMenuEntryView(
    Browser* browser,
    bool is_enterprise,
    ToolbarActionViewModel* view_model,
    base::RepeatingCallback<void(bool)> site_access_toggle_callback,
    views::Button::PressedCallback site_permissions_button_callback)
    : ExtensionsMenuEntryView(browser,
                              is_enterprise,
                              view_model,
                              std::move(site_access_toggle_callback),
                              std::move(site_permissions_button_callback)) {
  views::InstallRoundRectHighlightPathGenerator(site_permissions_button_,
                                                gfx::Insets(), 4);

  site_access_toggle_->SetProperty(views::kMarginsKey,
                                   gfx::Insets().set_left(20));
}
BraveExtensionsMenuEntryView::~BraveExtensionsMenuEntryView() = default;

void BraveExtensionsMenuEntryView::UpdateContextMenuButton(
    ExtensionsMenuViewModel::ControlState button_state) {
  ExtensionsMenuEntryView::UpdateContextMenuButton(button_state);

  // Unlike upstream, we always show three dot icon regardless of pinned state.
  context_menu_button_->SetImageModel(
      views::Button::STATE_NORMAL,
      *context_menu_button_->GetImageModel(views::Button::STATE_HOVERED));
  context_menu_button_->SetPreferredSize(gfx::Size(26, 26));
  context_menu_button_->SetProperty(views::kMarginsKey,
                                    gfx::Insets().set_left(20));
  context_menu_button_->SetHorizontalAlignment(
      gfx::HorizontalAlignment::ALIGN_CENTER);

  views::InstallCircleHighlightPathGenerator(context_menu_button_.get());
}

BEGIN_METADATA(BraveExtensionsMenuEntryView)
END_METADATA
