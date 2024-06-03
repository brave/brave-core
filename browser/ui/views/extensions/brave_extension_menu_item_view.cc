/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/extensions/brave_extension_menu_item_view.h"

#include <string>
#include <utility>

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "chrome/browser/ui/views/controls/hover_button.h"
#include "chrome/browser/ui/views/extensions/extensions_menu_button.h"
#include "chrome/browser/ui/views/extensions/extensions_menu_item_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "extensions/common/extension_features.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"

BraveExtensionMenuItemView::BraveExtensionMenuItemView(
    Browser* browser,
    bool is_enterprise,
    std::unique_ptr<ToolbarActionViewController> controller,
    base::RepeatingCallback<void(bool)> site_access_toggle_callback,
    views::Button::PressedCallback site_permissions_button_callback)
    : ExtensionMenuItemView(browser,
                            is_enterprise,
                            std::move(controller),
                            std::move(site_access_toggle_callback),
                            std::move(site_permissions_button_callback)) {
  CHECK(base::FeatureList::IsEnabled(
      extensions_features::kExtensionsMenuAccessControl));

  views::InstallRoundRectHighlightPathGenerator(site_permissions_button_,
                                                gfx::Insets(), 4);

  site_access_toggle_->SetProperty(views::kMarginsKey,
                                   gfx::Insets().set_left(20));
}

BraveExtensionMenuItemView::~BraveExtensionMenuItemView() = default;

void BraveExtensionMenuItemView::UpdateContextMenuButton(
    bool is_action_pinned) {
  ExtensionMenuItemView::UpdateContextMenuButton(is_action_pinned);

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

  // We want pin button to be shown if it's pinned
  UpdatePinButton(model_ && model_->IsActionForcePinned(controller_->GetId()),
                  is_action_pinned);
}

void BraveExtensionMenuItemView::UpdatePinButton(bool is_force_pinned,
                                                 bool is_pinned) {
  if (!is_force_pinned && !is_pinned) {
    // We don't show pinned icon if it's unpinned
    if (pin_button_) {
      pin_button_->parent()->RemoveChildViewT(pin_button_);
      pin_button_ = nullptr;
    }
    return;
  }

  // Show pin button if it's pinned
  if (!pin_button_) {
    auto pin_button = std::make_unique<HoverButton>(
        base::BindRepeating(&ExtensionMenuItemView::OnPinButtonPressed,
                            base::Unretained(this)),
        std::u16string());
    pin_button_ = pin_button.get();
    auto* parent = site_access_toggle_->parent();
    parent->AddChildViewAt(std::move(pin_button),
                           *parent->GetIndexOf(site_access_toggle_));
  }

  ExtensionMenuItemView::UpdatePinButton(is_force_pinned, is_pinned);

  UpdatePinButtonIcon();
  pin_button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(0, 4)));
  views::InstallRoundRectHighlightPathGenerator(pin_button_, gfx::Insets(), 4);
}

void BraveExtensionMenuItemView::OnThemeChanged() {
  ExtensionMenuItemView::OnThemeChanged();

  if (pin_button_) {
    UpdatePinButtonIcon();
  }

  auto* cp = GetColorProvider();
  CHECK(cp);
  site_access_toggle_->SetThumbOnColor(SK_ColorWHITE);
  site_access_toggle_->SetTrackOnColor(
      cp->GetColor(kColorBraveExtensionMenuIcon));
}

void BraveExtensionMenuItemView::UpdatePinButtonIcon() {
  CHECK(pin_button_);

  if (auto* cp = GetColorProvider()) {
    // Update icon with our asset.
    for (auto state : views::Button::kButtonStates) {
      pin_button_->SetImageModel(
          state, ui::ImageModel::FromVectorIcon(
                     kLeoPinIcon, cp->GetColor(kColorBraveExtensionMenuIcon)));
    }
  }
}

BEGIN_METADATA(BraveExtensionMenuItemView)
END_METADATA
