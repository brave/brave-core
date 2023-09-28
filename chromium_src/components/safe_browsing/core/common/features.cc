/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/components/safe_browsing/core/common/features.cc"

#include "base/feature_override.h"
#include "build/build_config.h"

namespace safe_browsing {

OVERRIDE_FEATURE_DEFAULT_STATES({{
// Download bubble is desktop only feature.
#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
    {kDownloadBubble, base::FEATURE_ENABLED_BY_DEFAULT},
    {kDownloadBubbleV2, base::FEATURE_ENABLED_BY_DEFAULT},
#endif
    {kExtensionTelemetry, base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionTelemetryDeclarativeNetRequestSignal,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionTelemetryDisableOffstoreExtensions,
     base::FEATURE_DISABLED_BY_DEFAULT},
    {kExtensionTelemetryTabsApiSignal, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace safe_browsing
