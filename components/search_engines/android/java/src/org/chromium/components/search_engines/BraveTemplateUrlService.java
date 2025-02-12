/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.search_engines;

import org.jni_zero.NativeMethods;

import org.chromium.base.Log;

public class BraveTemplateUrlService extends TemplateUrlService {

    // Overridden Chromium's TemplateUrlService.mNativeTemplateUrlServiceAndroid
    private long mNativeTemplateUrlServiceAndroid;

    public BraveTemplateUrlService(long nativeTemplateUrlServiceAndroid) {
        Log.e("brave_search", "BraveTemplateUrlService constructor");
        super(nativeTemplateUrlServiceAndroid);
    }

    public boolean addSearchEngine() {
        Log.e("brave_search", "addSearchEngine");
        return BraveTemplateUrlServiceJni.get().addSearchEngine(mNativeTemplateUrlServiceAndroid);
    }

    @NativeMethods
    public interface Natives {
        boolean addSearchEngine(long nativeTemplateUrlServiceAndroid);
    }
}
