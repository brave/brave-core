/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"
#include <utility>

#include "brave/browser/ui/views/infobars/brave_confirm_infobar.h"
#include "brave/browser/ui/views/infobars/brave_global_confirm_infobar.h"
#include "components/infobars/content/content_infobar_manager.h"

BraveConfirmInfoBarDelegate::BraveConfirmInfoBarDelegate() = default;
BraveConfirmInfoBarDelegate::~BraveConfirmInfoBarDelegate() = default;

int BraveConfirmInfoBarDelegate::GetButtons() const {
  return BUTTON_OK | BUTTON_CANCEL | BUTTON_EXTRA;
}

std::vector<int> BraveConfirmInfoBarDelegate::GetButtonsOrder() const {
  return {BUTTON_OK, BUTTON_EXTRA, BUTTON_CANCEL};
}

bool BraveConfirmInfoBarDelegate::IsProminent(int id) const {
  return id == BUTTON_OK;
}

bool BraveConfirmInfoBarDelegate::HasCheckbox() const {
  return false;
}

std::u16string BraveConfirmInfoBarDelegate::GetCheckboxText() const {
  return std::u16string();
}

void BraveConfirmInfoBarDelegate::SetCheckboxChecked(bool checked) {}

bool BraveConfirmInfoBarDelegate::InterceptClosing() {
  return false;
}

BraveConfirmInfoBarDelegateFactory::BraveConfirmInfoBarDelegateFactory(
    infobars::ContentInfoBarManager* infobar_manager,
    std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
    : infobar_manager_(infobar_manager),
      delegate_(std::move(delegate)) {}

BraveConfirmInfoBarDelegateFactory::BraveConfirmInfoBarDelegateFactory(std::unique_ptr<BraveConfirmInfoBarDelegate> delegate)
: delegate_(std::move(delegate)) {}

BraveConfirmInfoBarDelegateFactory::~BraveConfirmInfoBarDelegateFactory() = default;

void BraveConfirmInfoBarDelegateFactory::Create() {
  if(!delegate_) {
    return;
  }

  if (!infobar_manager_) {
    // show infobar as global
    BraveGlobalConfirmInfoBar::Show(std::move(delegate_));
    return;
  }

  // show infobar for one tab
  infobar_manager_->AddInfoBar(
      std::make_unique<BraveConfirmInfoBar>(std::move(delegate_)), true);
}
