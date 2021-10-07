/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_manager.h"

#include "brave/components/translate/core/browser/brave_translate_language_filter.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_prefs.h"

namespace translate {

class BraveIsSupportedTargetLanguageProxy : public TranslateDownloadManager {
 public:
  static bool IsSupportedLanguage(const std::string& lang) {
    return IsTargetLanguageCodeSupported(lang);
  }
  ~BraveIsSupportedTargetLanguageProxy() override;
};

}  // namespace translate
#define ASSERT_INSIDE_FUNCTION(fname)                     \
  static_assert(base::StringPiece(__FUNCTION__) == fname, \
                "should work only within " fname " function");

#define GetRecentTargetLanguage                                         \
  GetRecentTargetLanguage();                                            \
  ASSERT_INSIDE_FUNCTION("GetTargetLanguage")\
  using TranslateDownloadManager = BraveIsSupportedTargetLanguageProxy; \
  void
#define HasAPIKeyConfigured BraveHasAPIKeyConfigured
#define TranslateManager ChromiumTranslateManager
#include "../../../../../../components/translate/core/browser/translate_manager.cc"
#undef HasAPIKeyConfigured
#undef TranslateManager
#undef ASSERT_INSIDE_FUNCTION

namespace translate {

void TranslateManager::FilterIsTranslatePossible(
    TranslateTriggerDecision* decision,
    TranslatePrefs* translate_prefs,
    const std::string& page_language_code,
    const std::string& target_lang) {
  ChromiumTranslateManager::FilterIsTranslatePossible(
      decision, translate_prefs, page_language_code, target_lang);
  // The source language is not supported by Brave backend. Current we allow a
  // user to trigger a manual translation to have a chance to fix if it wasn't
  // recognized correctly (in that case it will shown as "Detect Language").
  if (!IsSourceLanguageCodeSupported(page_language_code)) {
    decision->PreventAutoTranslate();
    decision->PreventShowingUI();
    decision->initiation_statuses.push_back(
        TranslateBrowserMetrics::INITIATION_STATUS_LANGUAGE_IS_NOT_SUPPORTED);
    decision->ranker_events.push_back(
        metrics::TranslateEventProto::UNSUPPORTED_LANGUAGE);
    GetActiveTranslateMetricsLogger()->LogTriggerDecision(
        TriggerDecision::kDisabledUnsupportedLanguage);
  }

  // Just in case. In general a user can't trigger a translation to an
  // unsupported target language. But some new entry points can be added so
  // block translation in that case.
  if (!IsTargetLanguageCodeSupported(target_lang)) {
    decision->PreventAllTriggering();
    decision->initiation_statuses.push_back(
        TranslateBrowserMetrics::INITIATION_STATUS_LANGUAGE_IS_NOT_SUPPORTED);
    decision->ranker_events.push_back(
        metrics::TranslateEventProto::UNSUPPORTED_LANGUAGE);
    GetActiveTranslateMetricsLogger()->LogTriggerDecision(
        TriggerDecision::kDisabledUnsupportedLanguage);
  }
}

base::WeakPtr<TranslateManager> TranslateManager::GetWeakPtr() {
  return AsWeakPtr();
}

}  // namespace translate

namespace google_apis {

bool BraveHasAPIKeyConfigured() {
  // Google API key is not used in brave for translation service, always return
  // true for the API key check so the flow won't be blocked because of missing
  // keys.
  return true;
}

}  // namespace google_apis
