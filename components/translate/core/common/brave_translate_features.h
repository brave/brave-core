/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_FEATURES_H_
#define BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace translate {

namespace features {
extern const base::Feature kUseBraveTranslateGo;

extern const base::FeatureParam<bool> kUpdateLanguageListParam;
extern const base::FeatureParam<bool> kReplaceSecurityOriginParam;
extern const base::FeatureParam<bool>
    kDisableTranslateLibraryNetworkRedirectsParam;

}  // namespace features

// The translate engine can work the folowing ways:
// 1. IsTranslateExtensionAvailable() == true: Show a bubble to suggest a user
// installation of Google translate extension.
// 2. IsBraveTranslateGoAvailable() == true: The internal translation engine
// is used instead of the old bubble.
// 3. The other: no translation is available.
bool IsBraveTranslateGoAvailable();
bool IsTranslateExtensionAvailable();

// True if the translate urls in native code are redirected to
// translate-relay.brave.com. True by default, use false only for local testing.
bool UseBraveTranslateRelay();

// True if the supported language list can be updated from the backend. False by
// default.
bool ShouldUpdateLanguagesList();

}  // namespace translate

#endif  // BRAVE_COMPONENTS_TRANSLATE_CORE_COMMON_BRAVE_TRANSLATE_FEATURES_H_
