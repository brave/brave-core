/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/browser_features.cc"

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
#if BUILDFLAG(IS_WIN)
    {kAppBoundEncryptionMetrics, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
#if !BUILDFLAG(IS_ANDROID)
    {kCopyLinkToText, base::FEATURE_DISABLED_BY_DEFAULT},
#endif
    {kDestroyProfileOnBrowserClose, base::FEATURE_DISABLED_BY_DEFAULT},
    {kFedCmWithoutThirdPartyCookies, base::FEATURE_DISABLED_BY_DEFAULT},
    // Google has asked embedders not to enforce these pins:
    // https://groups.google.com/a/chromium.org/g/embedder-dev/c/XsNTwEiN1lI/m/TMXh-ZvOAAAJ
    {kKeyPinningComponentUpdater, base::FEATURE_DISABLED_BY_DEFAULT},
    {kOmniboxTriggerForNoStatePrefetch, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
