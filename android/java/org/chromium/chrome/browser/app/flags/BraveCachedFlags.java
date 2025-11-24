/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.flags;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureMap;
import org.chromium.components.cached_flags.BraveCachedFeatureParam;
import org.chromium.components.cached_flags.BraveCachedFlagUtils;
import org.chromium.components.cached_flags.CachedFeatureParam;
import org.chromium.components.cached_flags.CachedFlag;
import org.chromium.components.cached_flags.StringCachedFeatureParam;

import java.util.List;

@NullMarked
public class BraveCachedFlags extends ChromeCachedFlags {
    // Cached feature flag for fresh NTP after idle expiration - safe to access before native is
    // ready
    public static final CachedFlag sBraveFreshNtpAfterIdleExperimentEnabled =
            new CachedFlag(
                    ChromeFeatureMap.getInstance(),
                    BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT,
                    false);

    // Cached variant parameter for fresh NTP experiment - safe to access before native is ready
    public static final StringCachedFeatureParam sBraveFreshNtpAfterIdleExperimentVariant =
            new StringCachedFeatureParam(
                    ChromeFeatureMap.getInstance(),
                    BraveFeatureList.BRAVE_FRESH_NTP_AFTER_IDLE_EXPERIMENT,
                    "variant",
                    "A");

    // List of cached flags for Brave features - safe to access before native is ready
    private static final List<CachedFlag> sBraveFlagsCached =
            List.of(sBraveFreshNtpAfterIdleExperimentEnabled);
    // List of cached feature params for Brave features - safe to access before native is ready
    private static final List<CachedFeatureParam<?>> sBraveFeatureParamsCached =
            List.of(sBraveFreshNtpAfterIdleExperimentVariant);

    BraveCachedFlags() {
        BraveCachedFeatureParam.setBraveParams(sBraveFeatureParamsCached);
        BraveCachedFlagUtils.setBraveFlags(sBraveFlagsCached);
        BraveCachedFlagUtils.setBraveParams(sBraveFeatureParamsCached);
    }
}
