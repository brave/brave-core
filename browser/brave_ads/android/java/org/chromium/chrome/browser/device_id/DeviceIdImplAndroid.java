/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.device_id;

import android.annotation.SuppressLint;
import android.content.Context;
import android.provider.Settings;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;

import org.chromium.base.ContextUtils;

@JNINamespace("brave_ads")
public class DeviceIdImplAndroid {
    @SuppressLint("HardwareIds")
    @CalledByNative
    public static String getAndroidId() {
        Context context = ContextUtils.getApplicationContext();
        return Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
    }
}
