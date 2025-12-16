/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ephemeral_storage;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.tab.Tab;

@JNINamespace("brave_shields")
@NullMarked
public class BraveFirstPartyStorageCleanerUtils {
    private static final String TAG = "FirstPartyStorageCleanerUtils";

    public static void cleanupTLDFirstPartyStorage(Tab tab) {
        BraveFirstPartyStorageCleanerUtilsJni.get().cleanupTLDFirstPartyStorage(tab);
    }

    @NativeMethods
    interface Natives {
        void cleanupTLDFirstPartyStorage(Tab tab);
    }
}
