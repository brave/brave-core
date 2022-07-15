/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/sidebar/sidebar_item_add_button.h"

#include "base/bind.h"
#include "base/time/time.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/views/sidebar/sidebar_add_item_bubble_delegate_view.h"
#include "brave/grit/brave_theme_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/paint_vector_icon.h"

SidebarItemAddButton::SidebarItemAddButton(
    BraveBrowser* browser,
    const std::u16string& accessible_name)
    : SidebarButtonView(nullptr, accessible_name), browser_(browser) {
  UpdateButtonImages();

  on_enabled_changed_subscription_ =
      AddEnabledChangedCallback(base::BindRepeating(
          &SidebarItemAddButton::UpdateButtonImages, base::Unretained(this)));

  SetCallback(base::BindRepeating(&SidebarItemAddButton::OnButtonPressed,
                                  base::Unretained(this)));
}

SidebarItemAddButton::~SidebarItemAddButton() = default;

void SidebarItemAddButton::OnButtonPressed() {
  if (IsBubbleVisible())
    return;

  if (timer_.IsRunning())
    timer_.Stop();

  DoShowBubble();
}

void SidebarItemAddButton::OnMouseEntered(const ui::MouseEvent& event) {
  SidebarButtonView::OnMouseEntered(event);
  ShowBubbleWithDelay();
}

void SidebarItemAddButton::OnMouseExited(const ui::MouseEvent& event) {
  SidebarButtonView::OnMouseExited(event);
  // Don't show bubble if user goes outo from add item quickly.
  timer_.Stop();
}

void SidebarItemAddButton::OnGestureEvent(ui::GestureEvent* event) {
  SidebarButtonView::OnGestureEvent(event);
  if (event->type() == ui::ET_GESTURE_TAP) {
    // Show bubble immediately after tapping.
    DoShowBubble();
  }
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

void SidebarItemAddButton::ShowBubbleWithDelay() {
  if (IsBubbleVisible())
    return;

  if (timer_.IsRunning())
    timer_.Stop();

  constexpr int kBubbleLaunchDelayInMS = 200;
  timer_.Start(FROM_HERE, base::Milliseconds(kBubbleLaunchDelayInMS), this,
               &SidebarItemAddButton::DoShowBubble);
}

void SidebarItemAddButton::DoShowBubble() {
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
