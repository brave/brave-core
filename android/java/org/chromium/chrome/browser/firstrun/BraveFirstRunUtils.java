/**
 * Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.firstrun;

import org.jni_zero.NativeMethods;

/** Provides first run related utility functions. */
public class BraveFirstRunUtils {
    /**
     * @return The default value for metrics reporting state
     */
    public static boolean isMetricsReportingOptIn() {
        return BraveFirstRunUtilsJni.get().isMetricsReportingOptIn();
    }

    @NativeMethods
    public interface Natives {
        boolean isMetricsReportingOptIn();
    }
}
