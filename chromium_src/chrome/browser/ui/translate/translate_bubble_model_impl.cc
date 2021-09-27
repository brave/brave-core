/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/translate/translate_bubble_model_impl.h"

#undef TranslateBubbleModelImpl
#define TranslateBubbleModelImpl ChromiumTranslateBubbleModelImpl
#include "../../../../../../../chrome/browser/ui/translate/translate_bubble_model_impl.cc"
#undef TranslateBubbleModelImpl
#define TranslateBubbleModelImpl BraveTranslateBubbleModelImpl

BraveTranslateBubbleModelImpl::BraveTranslateBubbleModelImpl(
    translate::TranslateStep step,
    std::unique_ptr<translate::TranslateUIDelegate> ui_delegate)
    : ChromiumTranslateBubbleModelImpl(step, std::move(ui_delegate)) {
}

BraveTranslateBubbleModelImpl::~BraveTranslateBubbleModelImpl() {}

int BraveTranslateBubbleModelImpl::GetNumberOfSourceLanguages() const {
  return ui_delegate_->GetNumberOfSourceLanguages();
}

int BraveTranslateBubbleModelImpl::GetNumberOfTargetLanguages() const {
  return ui_delegate_->GetNumberOfTargetLanguages();
}

std::u16string BraveTranslateBubbleModelImpl::GetSourceLanguageNameAt(
    int index) const {
  return ui_delegate_->GetSourceLanguageNameAt(index);
}

std::u16string BraveTranslateBubbleModelImpl::GetTargetLanguageNameAt(
    int index) const {
  return ui_delegate_->GetTargetLanguageNameAt(index);
}

int BraveTranslateBubbleModelImpl::GetTargetLanguageIndex() const {
  return ui_delegate_->GetTargetLanguageIndex();
}

void BraveTranslateBubbleModelImpl::UpdateTargetLanguageIndex(int index) {
  ui_delegate_->UpdateTargetLanguageIndex(index);
}
