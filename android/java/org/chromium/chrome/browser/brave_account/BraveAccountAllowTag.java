/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_account;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.build.annotations.NullMarked;
import org.chromium.content_public.browser.WebContents;

@JNINamespace("brave_account")
@NullMarked
public final class BraveAccountAllowTag {
    private BraveAccountAllowTag() {}

    public static void mark(WebContents wc) {
        if (wc != null) {
            BraveAccountAllowTagJni.get().mark(wc);
        }
    }

    @NativeMethods
    public interface Natives {
        void mark(WebContents webContents);
    }
}
