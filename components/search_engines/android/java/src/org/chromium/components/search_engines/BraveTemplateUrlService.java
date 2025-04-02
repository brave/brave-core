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

    public boolean addSearchEngine(String title, String keyword, String url) {
        return BraveTemplateUrlServiceJni.get()
                .addSearchEngine(mNativeTemplateUrlServiceAndroid, title, keyword, url);
    }

    public boolean updateSearchEngine(
            String existingKeyword, String title, String keyword, String url) {
        return BraveTemplateUrlServiceJni.get()
                .updateSearchEngine(
                        mNativeTemplateUrlServiceAndroid, existingKeyword, title, keyword, url);
    }

    public void removeSearchEngine(String keyword) {
        BraveTemplateUrlServiceJni.get()
                .removeSearchEngine(mNativeTemplateUrlServiceAndroid, keyword);
    }

    @NativeMethods
    public interface Natives {
        boolean addSearchEngine(
                long nativeTemplateUrlServiceAndroid, String title, String keyword, String url);

        boolean updateSearchEngine(
                long nativeTemplateUrlServiceAndroid,
                String existingKeyword,
                String title,
                String keyword,
                String url);

        void removeSearchEngine(long nativeTemplateUrlServiceAndroid, String keyword);
    }
}
