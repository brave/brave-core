/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_account;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;

@JNINamespace("chrome::android")
@NullMarked
public class BraveAccountFeature {
    public static boolean isBraveAccountEnabled() {
        return BraveAccountFeatureJni.get().isBraveAccountEnabled();
    }

    @NativeMethods
    interface Natives {
        boolean isBraveAccountEnabled();
    }
}
