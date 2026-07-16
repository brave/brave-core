/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.app.flags;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.ntp.BraveFreshNtpHelper;
import org.chromium.chrome.browser.util.BraveDynamicColors;
import org.chromium.components.cached_flags.BraveCachedFeatureParam;
import org.chromium.components.cached_flags.BraveCachedFlagUtils;
import org.chromium.components.cached_flags.CachedFeatureParam;
import org.chromium.components.cached_flags.CachedFlag;

import java.util.List;

@NullMarked
public class BraveCachedFlags extends ChromeCachedFlags {
    // List of cached flags for Brave features - safe to access before native is ready
    private static final List<CachedFlag> sBraveFlagsCached =
            List.of(
                    BraveDynamicColors.sDynamicColorsEnabled,
                    BraveFreshNtpHelper.sBraveFreshNtpAfterIdleExperimentEnabled);
    // List of cached feature params for Brave features - safe to access before native is ready
    private static final List<CachedFeatureParam<?>> sBraveFeatureParamsCached =
            List.of(BraveFreshNtpHelper.sBraveFreshNtpAfterIdleExperimentVariant);

    // The build-time bytecode rewriter changes the allocation in
    // `ChromeCachedFlags.<clinit>` from `new ChromeCachedFlags()` to `new BraveCachedFlags()`.
    // Set Brave's cached flags and parameters here, after the lists above. Do not
    // set them in the constructor, which can run before those lists initialize.
    static {
        BraveCachedFeatureParam.setBraveParams(sBraveFeatureParamsCached);
        BraveCachedFlagUtils.setBraveFlags(sBraveFlagsCached);
        BraveCachedFlagUtils.setBraveParams(sBraveFeatureParamsCached);
    }

    BraveCachedFlags() {}
}
