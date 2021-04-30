/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser;

import org.chromium.base.annotations.JNINamespace;
import org.chromium.base.annotations.NativeMethods;
import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("brave_ads")
public class BraveAdsNativeHelper {
    private BraveAdsNativeHelper() {}

    public static boolean nativeIsBraveAdsEnabled(Profile profile) {
        return BraveAdsNativeHelperJni.get().isBraveAdsEnabled(profile);
    };

    public static void nativeSetAdsEnabled(Profile profile) {
        BraveAdsNativeHelperJni.get().setAdsEnabled(profile);
    };

    public static boolean nativeIsNewlySupportedLocale(Profile profile) {
        return BraveAdsNativeHelperJni.get().isNewlySupportedLocale(profile);
    };

    public static boolean nativeIsSupportedLocale(Profile profile) {
        return BraveAdsNativeHelperJni.get().isSupportedLocale(profile);
    };

    public static void nativeOnShowAdNotification(Profile profile, String j_notification_id) {
        BraveAdsNativeHelperJni.get().onShowAdNotification(profile, j_notification_id);
    };

    public static void nativeOnCloseAdNotification(
            Profile profile, String j_notification_id, boolean j_by_user) {
        BraveAdsNativeHelperJni.get().onCloseAdNotification(profile, j_notification_id, j_by_user);
    };

    public static void nativeOnClickAdNotification(Profile profile, String j_notification_id) {
        BraveAdsNativeHelperJni.get().onClickAdNotification(profile, j_notification_id);
    };

    @NativeMethods
    interface Natives {
        boolean isBraveAdsEnabled(Profile profile);
        void setAdsEnabled(Profile profile);
        boolean isSupportedLocale(Profile profile);
        boolean isNewlySupportedLocale(Profile profile);
        void onShowAdNotification(Profile profile, String j_notification_id);
        void onCloseAdNotification(Profile profile, String j_notification_id, boolean j_by_user);
        void onClickAdNotification(Profile profile, String j_notification_id);
    }
}
