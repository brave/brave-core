/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.speedreader;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.content_public.browser.WebContents;
import org.chromium.url.GURL;

@JNINamespace("speedreader")
public class BraveSpeedReaderUtils {
    public static void toggleEnabledForWebContent(WebContents webContents, boolean enabled) {
        BraveSpeedReaderUtilsJni.get().toggleEnabledForWebContent(webContents, enabled);
    }

    public static boolean tabStateIsDistilled(Tab tab) {
        return BraveSpeedReaderUtilsJni.get().tabStateIsDistilled(tab);
    }

    public static boolean tabSupportsDistillation(Tab tab) {
        return BraveSpeedReaderUtilsJni.get().tabSupportsDistillation(tab);
    }

    public static void singleShotSpeedreaderForWebContent(WebContents webContents) {
        BraveSpeedReaderUtilsJni.get().singleShotSpeedreaderForWebContent(webContents);
    }

    public static boolean tabProbablyReadable(Tab tab) {
        return BraveSpeedReaderUtilsJni.get().tabProbablyReadable(tab);
    }

    public static void enableSpeedreaderMode(Tab tab) {
        if (tab == null || tab.getWebContents() == null) return;
        WebContents webContents = tab.getWebContents();

        // Enable on tab
        toggleEnabledForWebContent(webContents, true);

        // Enable from original page
        if (tabSupportsDistillation(tab)) {
            if (tabProbablyReadable(tab)) {
                singleShotSpeedreaderForWebContent(webContents);
            } else {
                tab.reload();
            }
        }
    }

    @NativeMethods
    interface Natives {
        void singleShotSpeedreaderForWebContent(WebContents webContents);
        void toggleEnabledForWebContent(WebContents webContents, boolean enabled);
        boolean tabProbablyReadable(Tab tab);
        boolean tabStateIsDistilled(Tab tab);
        boolean tabSupportsDistillation(Tab tab);
    }
}
