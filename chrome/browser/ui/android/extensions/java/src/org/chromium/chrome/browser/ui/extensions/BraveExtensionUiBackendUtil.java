/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ui.extensions;

import org.chromium.base.ContextUtils;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.ui.base.DeviceFormFactor;

/** Backs {@link ExtensionUiBackendImpl#isEnabled}. */
@NullMarked
public class BraveExtensionUiBackendUtil {
    private static final String BRAVE_ANDROID_EXTENSIONS = "BraveAndroidExtensions";

    public static boolean isEnabled() {
        // The Extensions entry in the 3-dot menu and its UI aren't designed for
        // phones yet, so they're kept tablet-only for now even though the
        // underlying extension support (enable_extensions_core) is compiled in
        // for all Android form factors.
        return ChromeFeatureList.isEnabled(BRAVE_ANDROID_EXTENSIONS)
                && DeviceFormFactor.isNonMultiDisplayContextOnTablet(
                        ContextUtils.getApplicationContext());
    }
}
