/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.search_engines;

import org.jni_zero.NativeMethods;

public class BraveTemplateUrlService extends TemplateUrlService {

    // Overridden Chromium's TemplateUrlService.mNativeTemplateUrlServiceAndroid
    private long mNativeTemplateUrlServiceAndroid;

    public BraveTemplateUrlService(long nativeTemplateUrlServiceAndroid) {
        super(nativeTemplateUrlServiceAndroid);
        mNativeTemplateUrlServiceAndroid = nativeTemplateUrlServiceAndroid;
    }

    public boolean add(String title, String keyword, String url) {
        return BraveTemplateUrlServiceJni.get()
                .add(mNativeTemplateUrlServiceAndroid, title, keyword, url);
    }

    public boolean update(String existingKeyword, String title, String keyword, String url) {
        return BraveTemplateUrlServiceJni.get()
                .update(mNativeTemplateUrlServiceAndroid, existingKeyword, title, keyword, url);
    }

    public boolean remove(String keyword) {
        return BraveTemplateUrlServiceJni.get().remove(mNativeTemplateUrlServiceAndroid, keyword);
    }

    @NativeMethods
    public interface Natives {
        boolean add(long nativeTemplateUrlServiceAndroid, String title, String keyword, String url);

        boolean update(
                long nativeTemplateUrlServiceAndroid,
                String existingKeyword,
                String title,
                String keyword,
                String url);

        boolean remove(long nativeTemplateUrlServiceAndroid, String keyword);
    }
}
