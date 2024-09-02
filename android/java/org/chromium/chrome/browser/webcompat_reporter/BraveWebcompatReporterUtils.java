/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.webcompat_reporter;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;
import org.jni_zero.JniType;

@JNINamespace("webcompat_reporter")
public class BraveWebcompatReporterUtils {
    public static String getChannel() {
        return BraveWebcompatReporterUtilsJni.get().getChannel();
    }
    public static String getBraveVersion() {
        return BraveWebcompatReporterUtilsJni.get().getBraveVersion();
    }

    @NativeMethods
    interface Natives {
        @JniType("std::string")
        String getChannel();
        @JniType("std::string")
        String getBraveVersion();
    }
}
