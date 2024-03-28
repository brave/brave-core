/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/infobars/core/brave_confirm_infobar_delegate.h"

BraveConfirmInfoBarDelegate::BraveConfirmInfoBarDelegate() = default;
BraveConfirmInfoBarDelegate::~BraveConfirmInfoBarDelegate() = default;

int BraveConfirmInfoBarDelegate::GetButtons() const {
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  return BUTTON_OK | BUTTON_CANCEL | BUTTON_EXTRA;
#else
  return BUTTON_OK | BUTTON_CANCEL;
#endif
}

std::vector<int> BraveConfirmInfoBarDelegate::GetButtonsOrder() const {
#if !BUILDFLAG(IS_IOS) && !BUILDFLAG(IS_ANDROID)
  return {BUTTON_OK, BUTTON_EXTRA, BUTTON_CANCEL};
#else
  return {BUTTON_OK, BUTTON_CANCEL};
#endif
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

bool BraveConfirmInfoBarDelegate::ExtraButtonPressed() {
  return true;
}
