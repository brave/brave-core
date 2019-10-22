/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import org.chromium.base.Log;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("chrome::android")
public class BraveAdsNativeHelper {
    private BraveAdsNativeHelper() {}

    public static native boolean nativeIsBraveAdsEnabled(Profile profile);
    public static native boolean nativeIsLocaleValid(Profile profile);
    public static native void nativeSetAdsEnabled(Profile profile);
    public static native String nativeGetCountryCode(String locale);
    public static native String nativeGetLocale();
}
