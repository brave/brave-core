/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_shields_button.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/peek/public/peek_shields_panel_provider.h"
#include "brave/browser/ui/views/brave_actions/brave_shields_action_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/bubble/webui_bubble_manager.h"
#include "chrome/browser/ui/webui/top_chrome/webui_contents_wrapper.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host.h"
#include "ui/views/border.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/menu_button_controller.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace {

constexpr int kIconSize = 16;
constexpr gfx::Size kButtonSize(24, 24);

}  // namespace

PeekShieldsButton::PeekShieldsButton(BrowserWindowInterface& browser,
                                     PeekShieldsPanelProvider& provider)
    : LabelButton(views::Button::PressedCallback(), std::u16string()),
      browser_(browser),
      provider_(provider) {
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetPreferredSize(kButtonSize);
  SetBorder(views::CreateEmptyBorder(gfx::Insets()));
  views::InstallCircleHighlightPathGenerator(this);
  views::InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::ON);
  SetHasInkDropActionOnClick(true);

  // Pressing the button while the panel is open closes it.
  auto menu_button_controller = std::make_unique<views::MenuButtonController>(
      this,
      base::BindRepeating(&PeekShieldsButton::ButtonPressed,
                          weak_factory_.GetWeakPtr()),
      std::make_unique<views::Button::DefaultButtonControllerDelegate>(this));
  menu_button_controller_ = menu_button_controller.get();
  SetButtonController(std::move(menu_button_controller));
}

PeekShieldsButton::~PeekShieldsButton() = default;

void PeekShieldsButton::SetWebContents(content::WebContents* contents) {
  // The panel must not outlive the contents it controls.
  bubble_manager_.reset();
  shields_observation_.Reset();
  web_contents_ = contents ? contents->GetWeakPtr() : nullptr;

  if (contents) {
    if (auto* shields_tab_helper =
            brave_shields::BraveShieldsTabHelper::FromWebContents(contents)) {
      shields_observation_.Observe(shields_tab_helper);
    }
  }
  UpdateImage();
}

views::Widget* PeekShieldsButton::GetBubbleWidget() {
  return bubble_manager_ ? bubble_manager_->GetBubbleWidget() : nullptr;
}

std::u16string PeekShieldsButton::GetRenderedTooltipText(
    const gfx::Point& p) const {
  return BraveShieldsActionController::GetTooltipTextForWebContents(
      web_contents_.get());
}

void PeekShieldsButton::OnThemeChanged() {
  LabelButton::OnThemeChanged();
  UpdateImage();
}

void PeekShieldsButton::ButtonPressed() {
  if (!web_contents_) {
    return;
  }

  if (views::Widget* bubble = GetBubbleWidget()) {
    // The panel stays hidden until its page loads; ignore presses until then.
    if (bubble->IsVisible()) {
      bubble_manager_->CloseBubble();
    }
    return;
  }

  if (!bubble_manager_) {
    bubble_manager_ = provider_->CreateBubbleManager(*browser_);
  }
  bubble_manager_->ShowBubble(this);

  // The panel binds its data handler once its page loads, which happens after
  // this task.
  provider_->SetPanelTarget(
      bubble_manager_->GetContentsWrapper()->web_contents(),
      web_contents_.get());

  if (views::Widget* bubble = GetBubbleWidget()) {
    bubble_widget_observation_.Observe(bubble);
    pressed_lock_ = menu_button_controller_->TakeLock();
  }
}

void PeekShieldsButton::OnWidgetDestroying(views::Widget* widget) {
  bubble_widget_observation_.Reset();
  pressed_lock_.reset();
}

void PeekShieldsButton::UpdateImage() {
  SetImageModel(views::Button::STATE_NORMAL,
                BraveShieldsActionController::GetImageModelForWebContents(
                    web_contents_.get(), *browser_->GetProfile(), kButtonSize,
                    kIconSize));
}

void PeekShieldsButton::OnResourcesChanged() {
  UpdateImage();
}

void PeekShieldsButton::OnShieldsEnabledChanged() {
  UpdateImage();
}

BEGIN_METADATA(PeekShieldsButton)
END_METADATA
