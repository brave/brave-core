// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.search_engines;

import org.jni_zero.NativeMethods;

public class BraveTemplateUrlService extends TemplateUrlService {

    // Overridden Chromium's TemplateUrlService.mNativeTemplateUrlServiceAndroid
    private long mNativeTemplateUrlServiceAndroid;

    public BraveTemplateUrlService(long nativeTemplateUrlServiceAndroid) {
        super(nativeTemplateUrlServiceAndroid);
    }

    public boolean addSearchEngine() {
        return BraveTemplateUrlServiceJni.get().addSearchEngine(mNativeTemplateUrlServiceAndroid);
    }

    @NativeMethods
    public interface Natives {
        boolean addSearchEngine(long nativeTemplateUrlServiceAndroid);
    }
}
