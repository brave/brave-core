/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveLeoPreferences;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveLeoPrefUtils {
    private static final String TAG = "BraveLeoPrefUtils";

    /**
     * Returns true if Leo is disabled by policy for the given profile.
     *
     * <p>Note: The pref is "enabled_by_policy", so we check if it's managed AND false (disabled).
     */
    public static boolean isLeoDisabledByPolicy(@Nullable Profile profile) {
        if (profile == null) {
            return false;
        }
        PrefService prefService = UserPrefs.get(profile);
        // kEnabledByPolicy: true = enabled, false = disabled
        // We return true (disabled) if it's managed AND the value is false
        return prefService.isManagedPreference(BravePref.ENABLED_BY_POLICY)
                && !prefService.getBoolean(BravePref.ENABLED_BY_POLICY);
    }

    private static Profile getProfile() {
        Profile profile = null;
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            profile = activity.getCurrentProfile();
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "get BraveActivity exception", e);
        }
        if (profile == null) {
            Log.e(TAG, "BraveLeoPrefUtils.getProfile profile is null");
        }

        return profile;
    }

    public static void setIsSubscriptionActive(boolean value) {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.setIsSubscriptionActive profile is null");
            return;
        }
        UserPrefs.get(profileToUse)
                .setBoolean(BravePref.BRAVE_CHAT_SUBSCRIPTION_ACTIVE_ANDROID, value);
    }

    public static boolean getIsSubscriptionActive(Profile profile) {
        Profile profileToUse = profile == null ? BraveLeoPrefUtils.getProfile() : profile;
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.getIsSubscriptionActive profile is null");
            return false;
        }
        return UserPrefs.get(profileToUse)
                .getBoolean(BravePref.BRAVE_CHAT_SUBSCRIPTION_ACTIVE_ANDROID);
    }

    public static void setChatPurchaseToken(String token) {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.setChatPurchaseToken profile is null");
            return;
        }
        PrefService prefService = UserPrefs.get(profileToUse);
        if (prefService.getString(BravePref.BRAVE_CHAT_PURCHASE_TOKEN_ANDROID).equals(token)
                && !prefService.getString(BravePref.BRAVE_CHAT_ORDER_ID_ANDROID).isEmpty()) {
            return;
        }
        // It means we don't have a Play Store subscription anymore or
        // we have a new one.
        BraveLeoPrefUtils.resetSubscriptionLinkedStatus(profileToUse);
        prefService.setString(BravePref.BRAVE_CHAT_ORDER_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_CHAT_PURCHASE_TOKEN_ANDROID, token);
        if (!token.isEmpty()) {
            createFetchOrder(profileToUse);
        }
    }

    public static boolean getIsHistoryEnabled() {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.getIsHistoryEnabled profile is null");
            return false;
        }
        return UserPrefs.get(profileToUse).getBoolean(BravePref.BRAVE_CHAT_STORAGE_ENABLED);
    }

    public static void setIsHistoryEnabled(boolean isEnabled) {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.getIsHistoryEnabled profile is null");
            return;
        }
        UserPrefs.get(profileToUse).setBoolean(BravePref.BRAVE_CHAT_STORAGE_ENABLED, isEnabled);
    }

    private static void createFetchOrder(Profile profileToUse) {
        BraveLeoMojomHelper.getInstance(profileToUse)
                .createOrderId(
                        orderId -> {
                            fetchOrder(profileToUse, orderId);
                        });
    }

    private static void fetchOrder(Profile profileToUse, String orderId) {
        BraveLeoMojomHelper.getInstance(profileToUse)
                .fetchOrderCredentials(
                        orderId,
                        response -> {
                            if (!response.isEmpty()) {
                                return;
                            }
                            UserPrefs.get(profileToUse)
                                    .setString(BravePref.BRAVE_CHAT_ORDER_ID_ANDROID, orderId);
                        });
    }

    public static void setChatPackageName() {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.setChatPackageName profile is null");
            return;
        }
        UserPrefs.get(profileToUse)
                .setString(
                        BravePref.BRAVE_CHAT_PACKAGE_NAME_ANDROID,
                        ContextUtils.getApplicationContext().getPackageName());
    }

    public static void setChatProductId(String productId) {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.setChatProductId profile is null");
            return;
        }
        UserPrefs.get(profileToUse).setString(BravePref.BRAVE_CHAT_PRODUCT_ID_ANDROID, productId);
    }

    public static boolean isLeoEnabled() {
        if (!ChromeFeatureList.isEnabled(BraveFeatureList.AI_CHAT)) {
            return false;
        }

        return true;
    }

    public static boolean isSubscriptionLinked() {
        Profile profileToUse = BraveLeoPrefUtils.getProfile();
        if (profileToUse == null) {
            Log.e(TAG, "BraveLeoPrefUtils.isSubscriptionLinked profile is null");
            return false;
        }

        return UserPrefs.get(profileToUse)
                        .getInteger(BravePref.BRAVE_CHAT_SUBSCRIPTION_LINK_STATUS_ANDROID)
                != 0;
    }

    private static void resetSubscriptionLinkedStatus(Profile profile) {
        UserPrefs.get(profile).setInteger(BravePref.BRAVE_CHAT_SUBSCRIPTION_LINK_STATUS_ANDROID, 0);
    }

    public static boolean shouldShowLeoQuickSearchEngine() {
        return ChromeSharedPreferences.getInstance()
                .readBoolean(BraveLeoPreferences.PREF_LEO_QUICK_SEARCH_ENGINE, true);
    }
}
