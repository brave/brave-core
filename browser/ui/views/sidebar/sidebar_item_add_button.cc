/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/menu_button_controller.h"

SidebarItemAddButton::SidebarItemAddButton(
    BraveBrowser* browser,
    const std::u16string& accessible_name)
    : SidebarButtonView(accessible_name), browser_(browser) {
  UpdateButtonImages();

  on_enabled_changed_subscription_ =
      AddEnabledChangedCallback(base::BindRepeating(
          &SidebarItemAddButton::UpdateButtonImages, base::Unretained(this)));

  // The MenuButtonController makes sure the bubble closes when clicked if the
  // bubble is already open.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&SidebarItemAddButton::OnButtonPressed,
                          base::Unretained(this)),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  SetButtonController(std::move(menu_button_controller));
}

SidebarItemAddButton::~SidebarItemAddButton() = default;

void SidebarItemAddButton::OnButtonPressed() {
  if (IsBubbleVisible()) {
    return;
  }

  ShowBubble();
}

void SidebarItemAddButton::OnWidgetDestroying(views::Widget* widget) {
  observation_.Reset();
}

void SidebarItemAddButton::ShowBubble() {
  auto* bubble = SidebarAddItemBubbleDelegateView::Create(browser_, this);
  observation_.Observe(bubble);
  bubble->Show();
}

bool SidebarItemAddButton::IsBubbleVisible() const {
  return observation_.IsObserving();
}

void SidebarItemAddButton::UpdateButtonImages() {
  SetImageModel(STATE_NORMAL, ui::ImageModel::FromVectorIcon(
                                  kLeoPlusAddIcon, kColorSidebarButtonBase,
                                  kDefaultIconSize));
  SetImageModel(STATE_PRESSED, ui::ImageModel::FromVectorIcon(
                                   kLeoPlusAddIcon, kColorSidebarButtonPressed,
                                   kDefaultIconSize));
  SetImageModel(
      STATE_DISABLED,
      ui::ImageModel::FromVectorIcon(
          kLeoPlusAddIcon, kColorSidebarAddButtonDisabled, kDefaultIconSize));
}

BEGIN_METADATA(SidebarItemAddButton, SidebarButtonView)
END_METADATA
