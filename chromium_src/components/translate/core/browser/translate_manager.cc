/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/translate/core/browser/translate_manager.h"

#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/components/translate/core/browser/brave_translate_language_filter.h"
#include "components/translate/core/browser/translate_download_manager.h"
#include "components/translate/core/browser/translate_prefs.h"

namespace translate {

class BraveIsSupportedTargetLanguageProxy : public TranslateDownloadManager {
 public:
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  static bool IsSupportedLanguage(const std::string& lang) {
    return IsTargetLanguageCodeSupported(lang);
  }
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  ~BraveIsSupportedTargetLanguageProxy() override;
};

}  // namespace translate
#define ASSERT_INSIDE_FUNCTION(fname)                     \
  static_assert(base::StringPiece(__FUNCTION__) == fname, \
                "should work only within " fname " function");

#define GetRecentTargetLanguage                                         \
  GetRecentTargetLanguage();                                            \
  ASSERT_INSIDE_FUNCTION("GetTargetLanguage")                           \
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
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  // The source language is not supported by Brave backend. Currently we allow a
  // user to trigger a manual translation to have a chance to change the
  // incorrectly recognized source language to the correct one.
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
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
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
