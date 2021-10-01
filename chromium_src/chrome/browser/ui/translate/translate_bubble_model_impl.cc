/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/translate/translate_bubble_model_impl.h"

#define TranslateBubbleModelImpl ChromiumTranslateBubbleModelImpl
#include "../../../../../../../chrome/browser/ui/translate/translate_bubble_model_impl.cc"
#undef TranslateBubbleModelImpl

#include "base/containers/contains.h"

namespace {
const int kNoIndex = static_cast<int>(translate::TranslateUIDelegate::kNoIndex);
}  // namespace

bool IsSourceLanguageSupported(const std::string& lang) {
  return lang == "fr" || lang == "en" || lang == "de" || lang == "und";
}

bool IsTargetLanguageSupported(const std::string& lang) {
  return lang == "en" || lang == "ru";
}

class BraveLanguageMap {
 public:
  BraveLanguageMap(const translate::TranslateUIDelegate* ui_delegate,
                   base::RepeatingCallback<bool(const std::string&)> filter) {
    int ui_index = 0;
    for (size_t core_index = 0;
         core_index < ui_delegate->GetNumberOfLanguages(); ++core_index) {
      const auto lang = ui_delegate->GetLanguageCodeAt(core_index);
      if (!filter.Run(lang))
        continue;
      to_core_index_[ui_index] = core_index;
      to_ui_index_[core_index] = ui_index;
      ++ui_index;
    }
    DCHECK_EQ(to_ui_index_.size(), to_core_index_.size());
#if DCHECK_IS_ON()
    for (int i = 0; i < static_cast<int>(to_core_index_.size()); ++i) {
      const int core_ind = to_core_index_[i];
      DCHECK_EQ(to_ui_index_[to_core_index_[i]], i);
      DCHECK_EQ(to_core_index_[to_ui_index_[core_ind]], core_ind);
    }
#endif  // DCHECK_IS_ON()
  }
  size_t ToCoreIndex(int index) const {
    if (index == kNoIndex || !base::Contains(to_core_index_, index))
      return translate::TranslateUIDelegate::kNoIndex;
    return to_core_index_.at(index);
  }
  size_t ToUiIndex(int index) const {
    if (index == kNoIndex || !base::Contains(to_ui_index_, index))
      return translate::TranslateUIDelegate::kNoIndex;
    return to_ui_index_.at(index);
  }

  int GetSize() const { return to_core_index_.size(); }

 private:
  std::map<int, int> to_core_index_, to_ui_index_;
};

BraveTranslateBubbleModelImpl::BraveTranslateBubbleModelImpl(
    translate::TranslateStep step,
    std::unique_ptr<translate::TranslateUIDelegate> ui_delegate)
    : ChromiumTranslateBubbleModelImpl(step, std::move(ui_delegate)) {
  source_language_map_ = std::make_unique<BraveLanguageMap>(
      ui_delegate_.get(), base::BindRepeating(&IsSourceLanguageSupported));
  target_language_map_ = std::make_unique<BraveLanguageMap>(
      ui_delegate_.get(), base::BindRepeating(&IsTargetLanguageSupported));

  // If the source language is unsupported the drop it to unknown.
  // TODO(atuchin): is it good place to call this?
  if (!IsSourceLanguageSupported(ui_delegate_->GetSourceLanguageCode())) {
    ui_delegate_->UpdateSourceLanguageIndex(0u);
  }
}

BraveTranslateBubbleModelImpl::~BraveTranslateBubbleModelImpl() = default;

int BraveTranslateBubbleModelImpl::GetNumberOfSourceLanguages() const {
  return source_language_map_->GetSize();
}

int BraveTranslateBubbleModelImpl::GetNumberOfTargetLanguages() const {
  return target_language_map_->GetSize();
}

std::u16string BraveTranslateBubbleModelImpl::GetSourceLanguageNameAt(
    int index) const {
  return ui_delegate_->GetLanguageNameAt(
      source_language_map_->ToCoreIndex(index));
}

std::u16string BraveTranslateBubbleModelImpl::GetTargetLanguageNameAt(
    int index) const {
  return ui_delegate_->GetLanguageNameAt(
      target_language_map_->ToCoreIndex(index));
}

int BraveTranslateBubbleModelImpl::GetSourceLanguageIndex() const {
  return source_language_map_->ToUiIndex(
      ui_delegate_->GetSourceLanguageIndex());
}

int BraveTranslateBubbleModelImpl::GetTargetLanguageIndex() const {
  return target_language_map_->ToUiIndex(
      ui_delegate_->GetTargetLanguageIndex());
}

void BraveTranslateBubbleModelImpl::UpdateSourceLanguageIndex(int index) {
  ui_delegate_->UpdateSourceLanguageIndex(
      source_language_map_->ToCoreIndex(index));
}

void BraveTranslateBubbleModelImpl::UpdateTargetLanguageIndex(int index) {
  ui_delegate_->UpdateTargetLanguageIndex(
      target_language_map_->ToCoreIndex(index));
}
