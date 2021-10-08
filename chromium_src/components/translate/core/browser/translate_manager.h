/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_MANAGER_H_

namespace translate {
class TranslateManager;
using BraveTranslateManager = TranslateManager;
}  // namespace translate

#define TranslateManager ChromiumTranslateManager
#define ignore_missing_key_for_testing_ \
  ignore_missing_key_for_testing_;      \
  friend BraveTranslateManager
#define FilterIsTranslatePossible virtual FilterIsTranslatePossible
#include "../../../../../../components/translate/core/browser/translate_manager.h"
#undef FilterIsTranslatePossible
#undef ignore_missing_key_for_testing_
#undef TranslateManager

namespace translate {

// Brave customization of TranslateManager that limit the number of supported
// languages. Also two independet lists are used for source and target
// languages.
class TranslateManager : public ChromiumTranslateManager,
                         public base::SupportsWeakPtr<TranslateManager> {
 public:
  using ChromiumTranslateManager::ChromiumTranslateManager;
  void FilterIsTranslatePossible(TranslateTriggerDecision* decision,
                                 TranslatePrefs* translate_prefs,
                                 const std::string& page_language_code,
                                 const std::string& target_lang) override;
  base::WeakPtr<TranslateManager> GetWeakPtr();
};

}  // namespace translate

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_MANAGER_H_
