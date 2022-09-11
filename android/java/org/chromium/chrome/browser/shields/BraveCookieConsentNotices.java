/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.shields;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.ArrayList;
import java.util.List;

@JNINamespace("chrome::android")
public class BraveCookieConsentNotices {
    private long mNativeBraveCookieConsentNotices;
    private static BraveCookieConsentNotices sInstance;
    private static final Object lock = new Object();

    public static BraveCookieConsentNotices getInstance() {
        synchronized(lock) {
          if(sInstance == null) {
              sInstance = new BraveCookieConsentNotices();
          }
        }
        return sInstance;
    }

    private BraveCookieConsentNotices() {
        mNativeBraveCookieConsentNotices = 0;
        init();
    }

    private void init() {
        if (mNativeBraveCookieConsentNotices == 0) {
            BraveCookieConsentNoticesJni.get().init(this);
        }
    }

    public void enableFilter() {
        synchronized (lock) {
            BraveCookieConsentNoticesJni.get().enableFilter(mNativeBraveCookieConsentNotices);
        }
    }

    public boolean isFilterListAvailable() {
        synchronized (lock) {
            return BraveCookieConsentNoticesJni.get().isFilterListAvailable(
                    mNativeBraveCookieConsentNotices);
        }
    }

    @Override
    protected void finalize() {
        destroy();
    }

    private void destroy() {
        if (mNativeBraveCookieConsentNotices == 0) {
            return;
        }
        BraveCookieConsentNoticesJni.get().destroy(mNativeBraveCookieConsentNotices);
    }

    @CalledByNative
    private void setNativePtr(long nativePtr) {
        assert mNativeBraveCookieConsentNotices == 0;
        mNativeBraveCookieConsentNotices = nativePtr;
    }

    @NativeMethods
    interface Natives {
        void init(BraveCookieConsentNotices self);
        void enableFilter(long nativeBraveCookieConsentNotices);
        boolean isFilterListAvailable(long nativeBraveCookieConsentNotices);
        void destroy(long nativeBraveCookieConsentNotices);
    }
}
