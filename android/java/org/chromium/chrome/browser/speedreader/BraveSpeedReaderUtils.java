/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.speedreader;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.WebContents;

@JNINamespace("speedreader")
public class BraveSpeedReaderUtils {
    public static boolean isEnabledForWebContent(WebContents webContents) {
        return BraveSpeedReaderUtilsJni.get().isEnabledForWebContent(webContents);
    }

    public static void toggleEnabledForWebContent(WebContents webContents, boolean enabled) {
        BraveSpeedReaderUtilsJni.get().toggleEnabledForWebContent(webContents, enabled);
    }

    public static boolean isTabDistilled(Tab tab) {
        return BraveSpeedReaderUtilsJni.get().isTabDistilled(tab);
    }

    public static boolean tabSupportsDistillation(Tab tab) {
        return BraveSpeedReaderUtilsJni.get().tabSupportsDistillation(tab);
    }

    public static boolean tabWantsDistill(Tab tab) {
        return BraveSpeedReaderUtilsJni.get().tabWantsDistill(tab);
    }

    @NativeMethods
    interface Natives {
        boolean isEnabledForWebContent(WebContents webContents);
        void toggleEnabledForWebContent(WebContents webContents, boolean enabled);
        boolean isTabDistilled(Tab tab);
        boolean tabSupportsDistillation(Tab tab);
        boolean tabWantsDistill(Tab tab);
    }
}
