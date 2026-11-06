/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/peek/peek_shields_button.h"

#include <utility>

#include "brave/browser/ui/views/brave_actions/brave_shields_action_controller.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/animation/ink_drop.h"
#include "ui/views/animation/ink_drop_host.h"
#include "ui/views/border.h"
#include "ui/views/controls/highlight_path_generator.h"

namespace {

constexpr int kIconSize = 16;
constexpr gfx::Size kButtonSize(24, 24);

}  // namespace

PeekShieldsButton::PeekShieldsButton(BrowserWindowInterface& browser,
                                     views::Button::PressedCallback callback)
    : LabelButton(std::move(callback), std::u16string()), browser_(browser) {
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  SetTooltipText(l10n_util::GetStringUTF16(IDS_BRAVE_SHIELDS));
  SetHorizontalAlignment(gfx::ALIGN_CENTER);
  SetPreferredSize(kButtonSize);
  SetBorder(views::CreateEmptyBorder(gfx::Insets()));
  views::InstallCircleHighlightPathGenerator(this);
  views::InkDrop::Get(this)->SetMode(views::InkDropHost::InkDropMode::ON);
  SetHasInkDropActionOnClick(true);
}

PeekShieldsButton::~PeekShieldsButton() = default;

void PeekShieldsButton::SetWebContents(content::WebContents* contents) {
  web_contents_ = contents ? contents->GetWeakPtr() : nullptr;
  UpdateImage();
}

void PeekShieldsButton::UpdateImage() {
  SetImageModel(views::Button::STATE_NORMAL,
                BraveShieldsActionController::GetImageModelForWebContents(
                    web_contents_.get(), *browser_->GetProfile(), kButtonSize,
                    kIconSize));
}

void PeekShieldsButton::OnThemeChanged() {
  LabelButton::OnThemeChanged();
  UpdateImage();
}

BEGIN_METADATA(PeekShieldsButton)
END_METADATA
