/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/time/time.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/menu_button_controller.h"

SidebarItemAddButton::SidebarItemAddButton(
    BraveBrowser* browser,
    const std::u16string& accessible_name)
    : SidebarButtonView(nullptr, accessible_name), browser_(browser) {
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
  SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(
      IDS_SIDEBAR_ADD_ITEM_BUTTON_TOOLTIP));
}

SidebarItemAddButton::~SidebarItemAddButton() = default;

void SidebarItemAddButton::OnButtonPressed() {
  if (IsBubbleVisible())
    return;

  ShowBubble();
}

void SidebarItemAddButton::OnThemeChanged() {
  View::OnThemeChanged();

  UpdateButtonImages();
}

void SidebarItemAddButton::AddedToWidget() {
  UpdateButtonImages();
}

void SidebarItemAddButton::OnWidgetDestroying(views::Widget* widget) {
  observation_.Reset();
}

void SidebarItemAddButton::ShowBubble() {
  auto* bubble = views::BubbleDialogDelegateView::CreateBubble(
      new SidebarAddItemBubbleDelegateView(browser_, this));
  observation_.Observe(bubble);
  bubble->Show();
}

bool SidebarItemAddButton::IsBubbleVisible() const {
  return observation_.IsObserving();
}

void SidebarItemAddButton::UpdateButtonImages() {
  SkColor button_base_color = SK_ColorWHITE;
  SkColor button_disabled_color = SK_ColorWHITE;
  if (const ui::ColorProvider* color_provider = GetColorProvider()) {
    button_base_color = color_provider->GetColor(kColorSidebarButtonBase);
    button_disabled_color =
        color_provider->GetColor(kColorSidebarAddButtonDisabled);
  }

  // Update add button image based on enabled state.
  SetImage(views::Button::STATE_NORMAL, nullptr);
  SetImage(views::Button::STATE_DISABLED, nullptr);
  SetImage(views::Button::STATE_HOVERED, nullptr);
  SetImage(views::Button::STATE_PRESSED, nullptr);
  auto& bundle = ui::ResourceBundle::GetSharedInstance();
  if (GetEnabled()) {
    SetImage(views::Button::STATE_NORMAL,
             gfx::CreateVectorIcon(kSidebarAddItemIcon, button_base_color));
    SetImage(views::Button::STATE_HOVERED,
             bundle.GetImageSkiaNamed(IDR_SIDEBAR_ITEM_ADD_FOCUSED));
    SetImage(views::Button::STATE_PRESSED,
             bundle.GetImageSkiaNamed(IDR_SIDEBAR_ITEM_ADD_FOCUSED));
  } else {
    SetImage(views::Button::STATE_NORMAL,
             gfx::CreateVectorIcon(kSidebarAddItemIcon, button_disabled_color));
  }
}

BEGIN_METADATA(SidebarItemAddButton, SidebarButtonView)
END_METADATA
