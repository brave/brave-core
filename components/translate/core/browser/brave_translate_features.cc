/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/translate/core/browser/brave_translate_features.h"

#include "brave/components/translate/core/browser/buildflags.h"

namespace translate {

namespace features {
const base::Feature kUseBraveTranslateGo {
  "UseBraveTranslateGo", base::FeatureState::FEATURE_DISABLED_BY_DEFAULT
};
}  // namespace features

bool IsBraveTranslateGoAvailable() {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  return base::FeatureList::IsEnabled(features::kUseBraveTranslateGo);
#else   // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_GO)
}

bool IsTranslateExtensionAvailable() {
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  return !base::FeatureList::IsEnabled(features::kUseBraveTranslateGo);
#else   // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  return false;
#endif  // BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
}

}  // namespace translate
