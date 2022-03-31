/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.search_engines.settings;

import androidx.annotation.NonNull;

import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;

public class BraveSearchEnginePrefHelper {
    private static BraveSearchEnginePrefHelper sInstance;

    private BraveSearchEnginePrefHelper() {}

    public static BraveSearchEnginePrefHelper getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new BraveSearchEnginePrefHelper();
        }
        return sInstance;
    }

    public void setFetchSEFromNative(boolean value) {
        BraveSearchEnginePrefHelperJni.get().setFetchSEFromNative(value);
    }

    public boolean getFetchSEFromNative() {
        return BraveSearchEnginePrefHelperJni.get().getFetchSEFromNative();
    }

    @NativeMethods
    interface Natives {
        void setFetchSEFromNative(boolean value);
        boolean getFetchSEFromNative();
    }
}
