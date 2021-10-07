/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/translate/translate_bubble_model_impl.h"

#define TranslateBubbleModelImpl ChromiumTranslateBubbleModelImpl
#include "../../../../../../chrome/browser/ui/translate/translate_bubble_model_impl.cc"
#undef TranslateBubbleModelImpl

#include "base/containers/contains.h"
#include "brave/components/translate/core/browser/brave_translate_language_filter.h"

namespace {
const int kNoIndex = static_cast<int>(translate::TranslateUIDelegate::kNoIndex);
}  // namespace

// A mapping between a chromium languages list in |ui_delegate| and a brave
// languages list (a limited subset of chromium list preassigned by |filter|).
// Note: mixin of int/size_t types is used because of chromium implementation.
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
      const auto core_ind = to_core_index_[i];
      DCHECK_EQ(to_ui_index_[to_core_index_[i]], i);
      DCHECK_EQ(to_core_index_[to_ui_index_[core_ind]], core_ind);
    }
#endif  // DCHECK_IS_ON()
  }

  // Coverts index in brave list (index in [0, GetSize() - 1]) to a correspoding
  // chromium index.
  size_t ToCoreIndex(int index) const {
    if (index == kNoIndex)
      return translate::TranslateUIDelegate::kNoIndex;
    const auto it = to_core_index_.find(index);
    return it != to_core_index_.end()
               ? it->second
               : translate::TranslateUIDelegate::kNoIndex;
  }

  // An inverse function to ToCoreIndex(). Coverts chromium index
  // (form [0, ui_delegate->GetNumberOfLanguages()] to a
  // correspoding index in brave list.
  int FromCoreIndex(size_t index) const {
    if (index == translate::TranslateUIDelegate::kNoIndex)
      return kNoIndex;
    const auto it = to_ui_index_.find(index);
    return it != to_ui_index_.end() ? it->second : kNoIndex;
  }

  int GetSize() const { return to_core_index_.size(); }

 private:
  std::map<int, size_t> to_core_index_;
  std::map<size_t, int> to_ui_index_;
};

TranslateBubbleModelImpl::TranslateBubbleModelImpl(
    translate::TranslateStep step,
    std::unique_ptr<translate::TranslateUIDelegate> ui_delegate)
    : ChromiumTranslateBubbleModelImpl(step, std::move(ui_delegate)) {
  source_language_map_ = std::make_unique<BraveLanguageMap>(
      ui_delegate_.get(),
      base::BindRepeating(&translate::IsSourceLanguageCodeSupported));
  target_language_map_ = std::make_unique<BraveLanguageMap>(
      ui_delegate_.get(),
      base::BindRepeating(&translate::IsTargetLanguageCodeSupported));

  // If the source language is unsupported then drop it to "und".
  // Theoretically it isn't the same as creating ui_delegate with source_lang ==
  // und, because |initial_source_language_index_| hasn't been updated. But in
  // fact chromium doesn't use |initial_source_language_index_| at all.
  if (!translate::IsSourceLanguageCodeSupported(
          ui_delegate_->GetSourceLanguageCode())) {
    ui_delegate_->UpdateSourceLanguageIndex(0u);
  }
}

TranslateBubbleModelImpl::~TranslateBubbleModelImpl() = default;

int TranslateBubbleModelImpl::GetNumberOfSourceLanguages() const {
  return source_language_map_->GetSize();
}

int TranslateBubbleModelImpl::GetNumberOfTargetLanguages() const {
  return target_language_map_->GetSize();
}

std::u16string TranslateBubbleModelImpl::GetSourceLanguageNameAt(
    int index) const {
  return ui_delegate_->GetLanguageNameAt(
      source_language_map_->ToCoreIndex(index));
}

std::u16string TranslateBubbleModelImpl::GetTargetLanguageNameAt(
    int index) const {
  return ui_delegate_->GetLanguageNameAt(
      target_language_map_->ToCoreIndex(index));
}

int TranslateBubbleModelImpl::GetSourceLanguageIndex() const {
  return source_language_map_->FromCoreIndex(
      ui_delegate_->GetSourceLanguageIndex());
}

int TranslateBubbleModelImpl::GetTargetLanguageIndex() const {
  return target_language_map_->FromCoreIndex(
      ui_delegate_->GetTargetLanguageIndex());
}

void TranslateBubbleModelImpl::UpdateSourceLanguageIndex(int index) {
  ui_delegate_->UpdateSourceLanguageIndex(
      source_language_map_->ToCoreIndex(index));
}

void TranslateBubbleModelImpl::UpdateTargetLanguageIndex(int index) {
  ui_delegate_->UpdateTargetLanguageIndex(
      target_language_map_->ToCoreIndex(index));
}
