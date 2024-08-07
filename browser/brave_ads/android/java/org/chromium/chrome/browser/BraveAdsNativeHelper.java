/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.chrome.browser.profiles.Profile;

@JNINamespace("brave_ads")
public class BraveAdsNativeHelper {
    private BraveAdsNativeHelper() {}

    public static boolean nativeIsOptedInToNotificationAds(Profile profile) {
        return BraveAdsNativeHelperJni.get().isOptedInToNotificationAds(profile);
    };

    public static void nativeSetOptedInToNotificationAds(Profile profile, boolean opted_in) {
        BraveAdsNativeHelperJni.get().setOptedInToNotificationAds(profile, opted_in);
    }

    public static boolean nativeIsSupportedRegion(Profile profile) {
        return BraveAdsNativeHelperJni.get().isSupportedRegion(profile);
    }

    public static void nativeClearData(Profile profile) {
        BraveAdsNativeHelperJni.get().clearData(profile);
    }

    public static void nativeOnNotificationAdShown(Profile profile, String j_notification_id) {
        BraveAdsNativeHelperJni.get().onNotificationAdShown(profile, j_notification_id);
    }

    public static void nativeOnNotificationAdClosed(
            Profile profile, String j_notification_id, boolean j_by_user) {
        BraveAdsNativeHelperJni.get().onNotificationAdClosed(profile, j_notification_id, j_by_user);
    }

    public static void nativeOnNotificationAdClicked(Profile profile, String j_notification_id) {
        BraveAdsNativeHelperJni.get().onNotificationAdClicked(profile, j_notification_id);
    }

    @NativeMethods
    interface Natives {
        boolean isOptedInToNotificationAds(Profile profile);

        void setOptedInToNotificationAds(Profile profile, boolean opted_in);

        boolean isSupportedRegion(Profile profile);

        void clearData(Profile profile);

        void onNotificationAdShown(Profile profile, String j_notification_id);

        void onNotificationAdClosed(Profile profile, String j_notification_id, boolean j_by_user);

        void onNotificationAdClicked(Profile profile, String j_notification_id);
    }
}
