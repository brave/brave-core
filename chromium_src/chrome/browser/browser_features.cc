/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/browser_features.cc"

#include "base/feature_override.h"

namespace features {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kCertificateTransparencyAskBeforeEnabling,
     base::FEATURE_ENABLED_BY_DEFAULT},

    {kBookmarkTriggerForPrerender2, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDestroyProfileOnBrowserClose, base::FEATURE_DISABLED_BY_DEFAULT},
    {kDevToolsConsoleInsights, base::FEATURE_DISABLED_BY_DEFAULT},
    // Google has asked embedders not to enforce these pins:
    // https://groups.google.com/a/chromium.org/g/embedder-dev/c/XsNTwEiN1lI/m/TMXh-ZvOAAAJ
    {kKeyPinningComponentUpdater, base::FEATURE_DISABLED_BY_DEFAULT},
    {kNewTabPageTriggerForPrerender2, base::FEATURE_DISABLED_BY_DEFAULT},
    {kSupportSearchSuggestionForPrerender2, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace features
