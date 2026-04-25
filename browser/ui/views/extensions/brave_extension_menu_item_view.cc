/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/extensions/brave_extension_menu_item_view.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_model.h"
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
    std::unique_ptr<ToolbarActionViewModel> view_model,
    bool allow_pinning)
    : ExtensionMenuItemView(browser, std::move(view_model), allow_pinning) {
  CHECK(!base::FeatureList::IsEnabled(
      extensions_features::kExtensionsMenuAccessControl));
  // Upstream code initially calls UpdatePinButton from c'tor which means our
  // override doesn't get called so we have to call it manually.
  if (allow_pinning) {
    bool is_pinned = model_ && model_->IsActionPinned(view_model_->GetId());
    bool is_force_pinned =
        model_ && model_->IsActionForcePinned(view_model_->GetId());
    UpdatePinButton(is_force_pinned, is_pinned);
  }
}

BraveExtensionMenuItemView::~BraveExtensionMenuItemView() = default;

void BraveExtensionMenuItemView::UpdatePinButton(bool is_force_pinned,
                                                 bool is_pinned) {
  ExtensionMenuItemView::UpdatePinButton(is_force_pinned, is_pinned);
  if (pin_button_) {
    // Update icon with our asset.
    for (auto state : views::Button::kButtonStates) {
      pin_button_->SetImageModel(
          state, ui::ImageModel::FromVectorIcon(
                     is_pinned ? kLeoPinDisableIcon : kLeoPinIcon,
                     kColorBraveExtensionMenuIcon));
    }
    pin_button_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(0, 4)));
    views::InstallRoundRectHighlightPathGenerator(pin_button_, gfx::Insets(),
                                                  4);
  }
}

BEGIN_METADATA(BraveExtensionMenuItemView)
END_METADATA
