/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_MANAGER_H_

namespace misc_metrics {
class TranslateMetrics;
}  // namespace misc_metrics

#define language_state_ \
  language_state_;      \
  raw_ptr<misc_metrics::TranslateMetrics> translate_metrics_
#define GetLanguageState                                  \
  GetLanguageState();                                     \
  void RegisterTranslateMetrics(                          \
      misc_metrics::TranslateMetrics* translate_metrics); \
  void DummyFunc
#include "src/components/translate/core/browser/translate_manager.h"  // IWYU pragma: export
#undef language_state_
#undef GetLanguageState

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_TRANSLATE_CORE_BROWSER_TRANSLATE_MANAGER_H_
