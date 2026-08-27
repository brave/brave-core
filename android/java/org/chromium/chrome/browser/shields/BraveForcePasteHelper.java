/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.content_public.browser.WebContents;

@JNINamespace("brave_shields")
@NullMarked
public class BraveForcePasteHelper {
    public static void forcePaste(WebContents webContents) {
        BraveForcePasteHelperJni.get().forcePaste(webContents);
    }

    @NativeMethods
    interface Natives {
        void forcePaste(WebContents webContents);
    }
}
