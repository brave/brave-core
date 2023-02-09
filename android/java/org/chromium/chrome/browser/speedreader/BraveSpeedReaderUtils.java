/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.speedreader;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.content_public.browser.WebContents;

@JNINamespace("speedreader")
public class BraveSpeedReaderUtils {
    public static void toggleOnWebContent(WebContents webContents) {
        BraveSpeedReaderUtilsJni.get().toggleOnWebContent(webContents);
    }

    @NativeMethods
    interface Natives {
        void toggleOnWebContent(WebContents webContents);
    }
}
