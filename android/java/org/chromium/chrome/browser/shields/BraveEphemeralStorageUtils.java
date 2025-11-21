/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.base.Log;
import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.tab.Tab;

@JNINamespace("ephemeral_storage")
@NullMarked
public class BraveEphemeralStorageUtils {
    private static final String TAG = "EphemeralStorageUtil";

    public static void cleanupTLDEphemeralStorage(Tab tab) {
        BraveEphemeralStorageUtilsJni.get().cleanupTLDEphemeralStorage(tab);
    }

    @NativeMethods
    interface Natives {
        void cleanupTLDEphemeralStorage(Tab tab);
    }
}
