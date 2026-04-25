/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.download;

import org.jni_zero.CalledByNative;
import org.jni_zero.JniType;

import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;

public class BraveMimeUtils {
    @CalledByNative
    public static boolean canAutoOpenMimeType(@JniType("std::string") String mimeType) {
        if (ContextUtils.getAppSharedPreferences()
                .getBoolean(
                        BravePreferenceKeys.BRAVE_DOWNLOADS_AUTOMATICALLY_OPEN_WHEN_POSSIBLE,
                        true)) {
            return MimeUtils.canAutoOpenMimeType(mimeType);
        }

        return false;
    }
}
