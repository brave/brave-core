/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_ui_delegate.h"

#undef TranslateUIDelegate
#define TranslateUIDelegate ChromiumTranslateUIDelegate
#include "../../../../../../../components/translate/core/browser/translate_ui_delegate.cc"
#undef TranslateUIDelegate
#define TranslateUIDelegate BraveTranslateUIDelegate

#include <algorithm>
#include <string>

namespace translate {

BraveTranslateUIDelegate::BraveTranslateUIDelegate(
    const base::WeakPtr<TranslateManager>& translate_manager,
    const std::string& source_language,
    const std::string& target_language)
    : ChromiumTranslateUIDelegate(translate_manager, source_language, target_language) {
  DCHECK(translate_driver_);
  DCHECK(translate_manager_);

  if (base::FeatureList::IsEnabled(
          language::kContentLanguagesInLanguagePicker)) {
    MaybeSetContentLanguages();
    // Also start listening for changes in the accept languages.
    PrefService* pref_service =
        translate_manager->translate_client()->GetPrefs();
    pref_change_registrar_.Init(pref_service);
    pref_change_registrar_.Add(
        language::prefs::kAcceptLanguages,
        base::BindRepeating(&ChromiumTranslateUIDelegate::MaybeSetContentLanguages,
                            base::Unretained(this)));
  }

  std::string locale =
      TranslateDownloadManager::GetInstance()->application_locale();

  std::vector<std::string> language_codes;
  TranslateDownloadManager::GetSupportedLanguages(
      prefs_->IsTranslateAllowedByPolicy(), &language_codes);
  // Reserve additional space for unknown language option on Android if feature
  // is enabled, and on Desktop always.
  std::vector<std::string>::size_type languages_size = language_codes.size();
#if defined(OS_ANDROID)
  if (base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption))
    languages_size += 1;
#elif !defined(OS_IOS)
  languages_size += 1;
#endif
  languages_.reserve(languages_size);

  // Preparing for the alphabetical order in the locale.
  std::unique_ptr<icu::Collator> collator = CreateCollator(locale);
  for (std::string& language_code : language_codes) {
    std::u16string language_name =
        l10n_util::GetDisplayNameForLocale(language_code, locale, true);
    languages_.emplace_back(std::move(language_code), std::move(language_name));
  }

  // Sort |languages_| in alphabetical order according to the display name.
  std::sort(
      languages_.begin(), languages_.end(),
      [&collator](const LanguageNamePair& lhs, const LanguageNamePair& rhs) {
        if (collator) {
          switch (base::i18n::CompareString16WithCollator(*collator, lhs.second,
                                                          rhs.second)) {
            case UCOL_LESS:
              return true;
            case UCOL_GREATER:
              return false;
            case UCOL_EQUAL:
              break;
          }
        } else {
          // |locale| may not be supported by ICU collator (crbug/54833). In
          // this case, let's order the languages in UTF-8.
          int result = lhs.second.compare(rhs.second);
          if (result != 0)
            return result < 0;
        }
        // Matching display names will be ordered alphabetically according to
        // the language codes.
        return lhs.first < rhs.first;
      });

  // Add unknown language option to the front of the list on Android if feature
  // is enabled, and on Desktop always.
  bool add_unknown_language_option = true;
#if defined(OS_IOS)
  add_unknown_language_option = false;
#elif defined(OS_ANDROID)
  if (!base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption))
    add_unknown_language_option = false;
#endif
  if (add_unknown_language_option) {
    //  Experiment in place to replace the "Unknown" string with "Detected
    //  Language".
    std::u16string unknown_language_string =
        base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption)
            ? l10n_util::GetStringUTF16(IDS_TRANSLATE_DETECTED_LANGUAGE)
            : l10n_util::GetStringUTF16(IDS_TRANSLATE_UNKNOWN_SOURCE_LANGUAGE);
    languages_.emplace_back(kUnknownLanguageCode, unknown_language_string);
    std::rotate(languages_.rbegin(), languages_.rbegin() + 1,
                languages_.rend());
  }

  // source languages

  language_codes.clear();
  TranslateDownloadManager::GetSupportedSourceLanguages(
      prefs_->IsTranslateAllowedByPolicy(), &language_codes);
  // Reserve additional space for unknown language option on Android if feature
  // is enabled, and on Desktop always.
  languages_size = language_codes.size();
#if defined(OS_ANDROID)
  if (base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption))
    languages_size += 1;
#elif !defined(OS_IOS)
  languages_size += 1;
#endif
  source_languages_.reserve(languages_size);

  // Preparing for the alphabetical order in the locale.
  collator = CreateCollator(locale);
  for (std::string& language_code : language_codes) {
    std::u16string language_name =
        l10n_util::GetDisplayNameForLocale(language_code, locale, true);
    source_languages_.emplace_back(std::move(language_code), std::move(language_name));
  }

  // Sort |source_languages_| in alphabetical order according to the display name.
  std::sort(
      source_languages_.begin(), source_languages_.end(),
      [&collator](const LanguageNamePair& lhs, const LanguageNamePair& rhs) {
        if (collator) {
          switch (base::i18n::CompareString16WithCollator(*collator, lhs.second,
                                                          rhs.second)) {
            case UCOL_LESS:
              return true;
            case UCOL_GREATER:
              return false;
            case UCOL_EQUAL:
              break;
          }
        } else {
          // |locale| may not be supported by ICU collator (crbug/54833). In
          // this case, let's order the languages in UTF-8.
          int result = lhs.second.compare(rhs.second);
          if (result != 0)
            return result < 0;
        }
        // Matching display names will be ordered alphabetically according to
        // the language codes.
        return lhs.first < rhs.first;
      });

  // Add unknown language option to the front of the list on Android if feature
  // is enabled, and on Desktop always.
  add_unknown_language_option = true;
#if defined(OS_IOS)
  add_unknown_language_option = false;
#elif defined(OS_ANDROID)
  if (!base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption))
    add_unknown_language_option = false;
#endif
  if (add_unknown_language_option) {
    //  Experiment in place to replace the "Unknown" string with "Detected
    //  Language".
    std::u16string unknown_language_string =
        base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption)
            ? l10n_util::GetStringUTF16(IDS_TRANSLATE_DETECTED_LANGUAGE)
            : l10n_util::GetStringUTF16(IDS_TRANSLATE_UNKNOWN_SOURCE_LANGUAGE);
    source_languages_.emplace_back(kUnknownLanguageCode, unknown_language_string);
    std::rotate(source_languages_.rbegin(), source_languages_.rbegin() + 1,
                source_languages_.rend());
  }

  // eof source languages

  // target languages

  language_codes.clear();
  TranslateDownloadManager::GetSupportedTargetLanguages(
      prefs_->IsTranslateAllowedByPolicy(), &language_codes);
  // Reserve additional space for unknown language option on Android if feature
  // is enabled, and on Desktop always.
  languages_size = language_codes.size();
#if defined(OS_ANDROID)
  if (base::FeatureList::IsEnabled(language::kDetectedSourceLanguageOption))
    languages_size += 1;
#elif !defined(OS_IOS)
  languages_size += 1;
#endif
  target_languages_.reserve(languages_size);

  // Preparing for the alphabetical order in the locale.
  collator = CreateCollator(locale);
  for (std::string& language_code : language_codes) {
    std::u16string language_name =
        l10n_util::GetDisplayNameForLocale(language_code, locale, true);
    target_languages_.emplace_back(std::move(language_code), std::move(language_name));
  }

  // Sort |target_languages_| in alphabetical order according to the display name.
  std::sort(
      target_languages_.begin(), target_languages_.end(),
      [&collator](const LanguageNamePair& lhs, const LanguageNamePair& rhs) {
        if (collator) {
          switch (base::i18n::CompareString16WithCollator(*collator, lhs.second,
                                                          rhs.second)) {
            case UCOL_LESS:
              return true;
            case UCOL_GREATER:
              return false;
            case UCOL_EQUAL:
              break;
          }
        } else {
          // |locale| may not be supported by ICU collator (crbug/54833). In
          // this case, let's order the languages in UTF-8.
          int result = lhs.second.compare(rhs.second);
          if (result != 0)
            return result < 0;
        }
        // Matching display names will be ordered alphabetically according to
        // the language codes.
        return lhs.first < rhs.first;
      });

  // eof target languages

  for (std::vector<LanguageNamePair>::const_iterator iter = source_languages_.begin();
       iter != source_languages_.end(); ++iter) {
    const std::string& language_code = iter->first;
    if (language_code == source_language) {
      source_language_index_ = iter - source_languages_.begin();
      initial_source_language_index_ = source_language_index_;
    }
  }

  for (std::vector<LanguageNamePair>::const_iterator iter = target_languages_.begin();
       iter != target_languages_.end(); ++iter) {
    const std::string& language_code = iter->first;
    if (language_code == target_language) {
      target_language_index_ = iter - target_languages_.begin();
    }
  }
}

BraveTranslateUIDelegate::~BraveTranslateUIDelegate() = default;

size_t BraveTranslateUIDelegate::GetNumberOfSourceLanguages() const {
  return source_languages_.size();
}

size_t BraveTranslateUIDelegate::GetNumberOfTargetLanguages() const {
  return target_languages_.size();
}

void BraveTranslateUIDelegate::UpdateSourceLanguageIndex(size_t language_index) {
  if (source_language_index_ == language_index)
    return;

  UMA_HISTOGRAM_BOOLEAN(kModifySourceLang, true);
  source_language_index_ = language_index;

  std::string language_code = kUnknownLanguageCode;
  if (language_index < GetNumberOfSourceLanguages())
    language_code = GetSourceLanguageCodeAt(language_index);
  if (translate_manager_) {
    translate_manager_->GetActiveTranslateMetricsLogger()->LogSourceLanguage(
        language_code);
  }
}

void BraveTranslateUIDelegate::UpdateSourceLanguage(
    const std::string& language_code) {
  for (size_t i = 0; i < source_languages_.size(); ++i) {
    if (source_languages_[i].first.compare(language_code) == 0) {
      UpdateSourceLanguageIndex(i);
      if (translate_manager_) {
        translate_manager_->mutable_translate_event()
            ->set_modified_source_language(language_code);
      }
      return;
    }
  }
}

void BraveTranslateUIDelegate::UpdateTargetLanguageIndex(size_t language_index) {
  if (target_language_index_ == language_index)
    return;

  DCHECK_LT(language_index, GetNumberOfTargetLanguages());
  UMA_HISTOGRAM_BOOLEAN(kModifyTargetLang, true);
  target_language_index_ = language_index;

  if (translate_manager_) {
    translate_manager_->GetActiveTranslateMetricsLogger()->LogTargetLanguage(
        GetTargetLanguageCodeAt(language_index),
        TranslateBrowserMetrics::TargetLanguageOrigin::kChangedByUser);
  }
}

void BraveTranslateUIDelegate::UpdateTargetLanguage(
    const std::string& language_code) {
  for (size_t i = 0; i < target_languages_.size(); ++i) {
    if (target_languages_[i].first.compare(language_code) == 0) {
      UpdateTargetLanguageIndex(i);
      if (translate_manager_) {
        translate_manager_->mutable_translate_event()
            ->set_modified_target_language(language_code);
      }
      return;
    }
  }
}

std::string BraveTranslateUIDelegate::GetSourceLanguageCodeAt(size_t index) const {
  DCHECK_LT(index, GetNumberOfSourceLanguages());
  return source_languages_[index].first;
}

std::string BraveTranslateUIDelegate::GetTargetLanguageCodeAt(size_t index) const {
  DCHECK_LT(index, GetNumberOfTargetLanguages());
  return target_languages_[index].first;
}

std::u16string BraveTranslateUIDelegate::GetSourceLanguageNameAt(size_t index) const {
  if (index == kNoIndex)
    return std::u16string();
  DCHECK_LT(index, GetNumberOfSourceLanguages());
  return source_languages_[index].second;
}

std::u16string BraveTranslateUIDelegate::GetTargetLanguageNameAt(size_t index) const {
  if (index == kNoIndex)
    return std::u16string();
  DCHECK_LT(index, GetNumberOfTargetLanguages());
  return target_languages_[index].second;
}

std::string BraveTranslateUIDelegate::GetSourceLanguageCode() const {
  return (GetSourceLanguageIndex() == kNoIndex)
             ? translate::kUnknownLanguageCode
             : GetSourceLanguageCodeAt(GetSourceLanguageIndex());
}

std::string BraveTranslateUIDelegate::GetTargetLanguageCode() const {
  return (GetTargetLanguageIndex() == kNoIndex)
             ? translate::kUnknownLanguageCode
             : GetTargetLanguageCodeAt(GetTargetLanguageIndex());
}

void BraveTranslateUIDelegate::Translate() {
  if (!translate_driver_->IsIncognito()) {
    prefs_->ResetTranslationDeniedCount(GetSourceLanguageCode());
    prefs_->ResetTranslationIgnoredCount(GetSourceLanguageCode());
    prefs_->IncrementTranslationAcceptedCount(GetSourceLanguageCode());
    prefs_->SetRecentTargetLanguage(GetTargetLanguageCode());
  }

  if (translate_manager_) {
    translate_manager_->RecordTranslateEvent(
        metrics::TranslateEventProto::USER_ACCEPT);
    translate_manager_->TranslatePage(
        GetSourceLanguageCode(), GetTargetLanguageCode(), false,
        translate_manager_->GetActiveTranslateMetricsLogger()
            ->GetNextManualTranslationType(
                /*is_context_menu_initiated_translation=*/false));
    UMA_HISTOGRAM_BOOLEAN(kPerformTranslate, true);
    if (IsLikelyAmpCacheUrl(translate_driver_->GetLastCommittedURL()))
      UMA_HISTOGRAM_BOOLEAN(kPerformTranslateAmpCacheUrl, true);
  }
}

}  // namespace translate
