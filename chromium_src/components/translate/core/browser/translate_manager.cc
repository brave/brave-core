/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/translate_metrics.h"

#define BRAVE_P3A_RECORD_PAGE_TRANSLATION        \
  if (translate_metrics_) {                      \
    translate_metrics_->RecordPageTranslation(); \
  }
#define HasAPIKeyConfigured BraveHasAPIKeyConfigured

#include "src/components/translate/core/browser/translate_manager.cc"

namespace translate {

void TranslateManager::RegisterTranslateMetrics(
    misc_metrics::TranslateMetrics* translate_metrics) {
  translate_metrics_ = translate_metrics;
}

}  // namespace translate

#undef HasAPIKeyConfigured
#undef BRAVE_P3A_RECORD_PAGE_TRANSLATION
