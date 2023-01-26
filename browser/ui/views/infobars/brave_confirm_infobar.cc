/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/controls/button/checkbox.h"

namespace {

constexpr int kCheckboxSpacing = 20;

}  // namespace

std::unique_ptr<infobars::InfoBar> CreateBraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate) {
  return std::make_unique<BraveConfirmInfoBar>(std::move(delegate));
}

BraveConfirmInfoBar::BraveConfirmInfoBar(
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
    : ConfirmInfoBar(std::move(delegate)) {
  auto* delegate_ptr = GetBraveDelegate();
  DCHECK(delegate_ptr);

  if (delegate_ptr->HasCheckbox()) {
    // We don't consider case where ok/cancel button and check box exist
    // together.
    DCHECK(!ok_button_ && !cancel_button_);
    checkbox_ = AddChildView(std::make_unique<views::Checkbox>(
        delegate_ptr->GetCheckboxText(),
        base::BindRepeating(&BraveConfirmInfoBar::CheckboxPressed,
                            base::Unretained(this))));
  }
}

BraveConfirmInfoBar::~BraveConfirmInfoBar() = default;

void BraveConfirmInfoBar::Layout() {
  ConfirmInfoBar::Layout();

  // Early return when checkbox is not used.
  // This class is only valid when this infobar has checkbox now.
  // NOTE: Revisit when we want to use other buttons together with checkbox.
  if (!checkbox_) {
    return;
  }

  checkbox_->SizeToPreferredSize();

  const int x = label_->bounds().right() + kCheckboxSpacing;
  checkbox_->SetPosition(gfx::Point(x, OffsetY(checkbox_)));
}

BraveConfirmInfoBarDelegate* BraveConfirmInfoBar::GetBraveDelegate() {
  return static_cast<BraveConfirmInfoBarDelegate*>(
      ConfirmInfoBar::GetDelegate());
}

void BraveConfirmInfoBar::CheckboxPressed() {
  GetBraveDelegate()->SetCheckboxChecked(checkbox_->GetChecked());
}

int BraveConfirmInfoBar::NonLabelWidth() const {
  const int width = ConfirmInfoBar::NonLabelWidth();

  // Early return when checkbox is not used.
  // This class is only valid when this infobar has checkbox now.
  if (!checkbox_) {
    return width;
  }

  return width + checkbox_->width() + kCheckboxSpacing;
}

void BraveConfirmInfoBar::CloseButtonPressed() {
  if (GetBraveDelegate()->InterceptClosing()) {
    return;
  }

  ConfirmInfoBar::CloseButtonPressed();
}

BEGIN_METADATA(BraveConfirmInfoBar, ConfirmInfoBar)
END_METADATA
