/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_language_list.h"

#undef TranslateLanguageList
#define TranslateLanguageList ChromiumTranslateLanguageList
#include "../../../../../../../components/translate/core/browser/translate_language_list.cc"
#undef TranslateLanguageList
#define TranslateLanguageList BraveTranslateLanguageList

#include <algorithm>
#include <iterator>

namespace translate {

const char BraveTranslateLanguageList::kSourceLanguagesKey[] = "sl";

BraveTranslateLanguageList::BraveTranslateLanguageList() : ChromiumTranslateLanguageList() {}

BraveTranslateLanguageList::~BraveTranslateLanguageList() {}

void BraveTranslateLanguageList::GetSupportedLanguages(
    bool translate_allowed,
    std::vector<std::string>* languages) {
  DCHECK(languages && languages->empty());
  *languages = supported_languages_;

  // Update language lists if they are not updated after Chrome was launched
  // for later requests.
  if (translate_allowed && !update_is_disabled && language_list_fetcher_.get())
    RequestLanguageList();
}

void BraveTranslateLanguageList::GetSupportedSourceLanguages(
    bool translate_allowed,
    std::vector<std::string>* languages) {
  DCHECK(languages && languages->empty());
  *languages = supported_source_languages_;

  // Update language lists if they are not updated after Chrome was launched
  // for later requests.
  if (translate_allowed && !update_is_disabled && language_list_fetcher_.get())
    RequestLanguageList();
}

void BraveTranslateLanguageList::GetSupportedTargetLanguages(
    bool translate_allowed,
    std::vector<std::string>* languages) {
  DCHECK(languages && languages->empty());
  *languages = supported_target_languages_;

  // Update language lists if they are not updated after Chrome was launched
  // for later requests.
  if (translate_allowed && !update_is_disabled && language_list_fetcher_.get())
    RequestLanguageList();
}

bool BraveTranslateLanguageList::IsSupportedSourceLanguage(base::StringPiece language) {
  return std::binary_search(supported_source_languages_.begin(),
                            supported_source_languages_.end(), language);
}

bool BraveTranslateLanguageList::IsSupportedTargetLanguage(base::StringPiece language) {
  return std::binary_search(supported_target_languages_.begin(),
                            supported_target_languages_.end(), language);
}

bool BraveTranslateLanguageList::SetSupportedLanguages(
    base::StringPiece language_list) {
  // The format is in JSON as:
  // {
  //   "sl": {"XX": "LanguageName", ...},
  //   "tl": {"XX": "LanguageName", ...}
  // }
  // Where "tl" is set in kTargetLanguagesKey.
  absl::optional<base::Value> json_value =
      base::JSONReader::Read(language_list, base::JSON_ALLOW_TRAILING_COMMAS);

  if (!json_value || !json_value->is_dict()) {
    NotifyEvent(__LINE__, "Language list is invalid");
    NOTREACHED();
    return false;
  }
  // The first level dictionary contains two sub-dicts, first for source
  // languages and second for target languages. We want to use the target
  // languages.
  base::Value* target_languages =
      json_value->FindDictPath(BraveTranslateLanguageList::kTargetLanguagesKey);
  if (!target_languages) {
    NotifyEvent(__LINE__, "Target languages are not found in the response");
    NOTREACHED();
    return false;
  }

  const std::string& locale =
      TranslateDownloadManager::GetInstance()->application_locale();

  // Now we can clear language list.
  supported_target_languages_.clear();
  // ... and replace it with the values we just fetched from the server.
  for (auto kv_pair : target_languages->DictItems()) {
    const std::string& lang = kv_pair.first;
    if (!l10n_util::IsLocaleNameTranslated(lang.c_str(), locale)) {
      // Don't include languages not displayable in current UI language.
      continue;
    }
    supported_target_languages_.push_back(lang);
  }

  base::Value* source_languages =
      json_value->FindDictPath(BraveTranslateLanguageList::kSourceLanguagesKey);
  if (!source_languages) {
    NotifyEvent(__LINE__, "Source languages are not found in the response");
    NOTREACHED();
    return false;
  }

  supported_source_languages_.clear();
  for (auto kv_pair : source_languages->DictItems()) {
    const std::string& lang = kv_pair.first;
    if (!l10n_util::IsLocaleNameTranslated(lang.c_str(), locale)) {
      continue;
    }
    supported_source_languages_.push_back(lang);
  }

  std::set<std::string> supported_lang_set(
      supported_target_languages_.begin(),
      supported_target_languages_.end());

  supported_lang_set.insert(
      supported_source_languages_.begin(),
      supported_source_languages_.end());

  supported_languages_ = std::vector<std::string>(
      supported_lang_set.begin(),
      supported_lang_set.end());

  // Since the DictionaryValue was sorted by key, |supported_languages_| should
  // already be sorted and have no duplicate values.
  DCHECK(
      std::is_sorted(supported_languages_.begin(), supported_languages_.end()));
  DCHECK(supported_languages_.end() ==
         std::adjacent_find(supported_languages_.begin(),
                            supported_languages_.end()));

  NotifyEvent(__LINE__, base::JoinString(supported_languages_, ", "));
  return true;
}

void BraveTranslateLanguageList::RequestLanguageList() {
  // If resource requests are not allowed, we'll get a callback when they are.
  if (!resource_requests_allowed_) {
    request_pending_ = true;
    return;
  }

  request_pending_ = false;

  if (language_list_fetcher_.get() &&
      (language_list_fetcher_->state() == TranslateURLFetcher::IDLE ||
       language_list_fetcher_->state() == TranslateURLFetcher::FAILED)) {
    GURL url = TranslateLanguageUrl();
    url = AddHostLocaleToUrl(url);
    url = AddApiKeyToUrl(url);

    NotifyEvent(__LINE__,
                base::StringPrintf("Language list fetch starts (URL: %s)",
                                   url.spec().c_str()));

    bool result = language_list_fetcher_->Request(
        url,
        base::BindOnce(&TranslateLanguageList::OnLanguageListFetchComplete,
                       base::Unretained(this)),
        // Use the strictest mode for request headers, since incognito state is
        // not known.
        /*is_incognito=*/true);
    if (!result)
      NotifyEvent(__LINE__, "Request is omitted due to retry limit");
  }
}

void BraveTranslateLanguageList::OnLanguageListFetchComplete(
    bool success,
    const std::string& data) {
  if (!success) {
    // Since it fails just now, omit to schedule resource requests if
    // ResourceRequestAllowedNotifier think it's ready. Otherwise, a callback
    // will be invoked later to request resources again.
    // The TranslateURLFetcher has a limit for retried requests and aborts
    // re-try not to invoke OnLanguageListFetchComplete anymore if it's asked to
    // re-try too many times.
    NotifyEvent(__LINE__, "Failed to fetch languages");
    return;
  }

  NotifyEvent(__LINE__, "Language list is updated");

  bool parsed_correctly = SetSupportedLanguages(data);
  language_list_fetcher_.reset();

  if (parsed_correctly)
    last_updated_ = base::Time::Now();
}

}  // namespace translate
