/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/translate/core/common/brave_translate_features.h"

#include "base/command_line.h"
#include "brave/components/translate/core/common/brave_translate_switches.h"

namespace translate {

namespace features {
BASE_FEATURE(kUseBraveTranslateGo,
             "UseBraveTranslateGo",
             base::FeatureState::FEATURE_ENABLED_BY_DEFAULT);

const base::FeatureParam<bool> kUpdateLanguageListParam{
    &kUseBraveTranslateGo, "update-languages", false};

}  // namespace features

bool IsBraveTranslateGoAvailable() {
  return base::FeatureList::IsEnabled(features::kUseBraveTranslateGo);
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

}  // namespace translate
