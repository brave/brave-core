/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "android_webview/common/aw_features.h"

#include "src/android_webview/common/aw_features.cc"

#include "base/feature_override.h"

namespace android_webview {
namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kWebViewMediaIntegrityApiBlinkExtension,
     base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
}  // namespace android_webview
