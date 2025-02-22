/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.search_engines;

public class BraveTemplateUrlService extends TemplateUrlService {

    // Overridden Chromium's TemplateUrlService.mNativeTemplateUrlServiceAndroid
    // private long mNativeTemplateUrlServiceAndroid;

    public BraveTemplateUrlService(long nativeTemplateUrlServiceAndroid) {
        super(nativeTemplateUrlServiceAndroid);
    }

    // public boolean addSearchEngine(String title, String keyword, String url) {
    //     Log.e("brave_search", "addSearchEngine");
    //     return BraveTemplateUrlServiceJni.get().addSearchEngine(mNativeTemplateUrlServiceAndroid,
    // title, keyword, url);
    // }

    // public void removeSearchEngine(String keyword) {
    //     Log.e("brave_search", "removeSearchEngine");
    //     BraveTemplateUrlServiceJni.get().removeSearchEngine(mNativeTemplateUrlServiceAndroid,
    // keyword);
    // }

    // @NativeMethods
    // public interface Natives {
    //     boolean addSearchEngine(long nativeTemplateUrlServiceAndroid, String title, String
    // keyword, String url);
    //     void removeSearchEngine(long nativeTemplateUrlServiceAndroid, String keyword);
    // }
}
