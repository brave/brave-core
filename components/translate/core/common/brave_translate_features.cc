/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/translate/core/common/brave_translate_features.h"

#include "base/command_line.h"
#include "brave/components/translate/core/common/brave_translate_switches.h"
#include "brave/components/translate/core/common/buildflags.h"

namespace translate {

namespace features {
const base::Feature kUseBraveTranslateGo{
    "UseBraveTranslateGo", base::FeatureState::FEATURE_DISABLED_BY_DEFAULT};

const base::FeatureParam<bool> kUpdateLanguageListParam{
    &kUseBraveTranslateGo, "update-languages", false};

const base::FeatureParam<bool> kUseBergamotLanguageList{
    &kUseBraveTranslateGo, "use-bergamot-language-list", true};

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

bool ShouldUpdateLanguagesList() {
  return IsBraveTranslateGoAvailable() &&
         features::kUpdateLanguageListParam.Get();
}

bool UseGoogleTranslateEndpoint() {
  return IsBraveTranslateGoAvailable() &&
         base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kBraveTranslateUseGoogleEndpoint);
}

bool ShouldUseBergamotLanguageList() {
  return IsBraveTranslateGoAvailable() &&
         features::kUseBergamotLanguageList.Get();
}

}  // namespace translate
