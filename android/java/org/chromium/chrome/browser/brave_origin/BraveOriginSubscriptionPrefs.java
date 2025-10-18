/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_origin;

import android.app.Activity;
import android.util.Base64;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.Callback;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.base.task.PostTask;
import org.chromium.base.task.TaskTraits;
import org.chromium.brave.browser.skus.SkusServiceFactory;
import org.chromium.brave.browser.util.BraveDomainsUtils;
import org.chromium.brave.browser.util.ServicesEnvironment;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.BraveOriginPreferences;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.prefs.PrefService;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.skus.mojom.SkusResult;
import org.chromium.skus.mojom.SkusResultCode;
import org.chromium.skus.mojom.SkusService;

import java.nio.charset.StandardCharsets;

/** Utility class for managing Brave Origin subscription preferences. */
@NullMarked
public class BraveOriginSubscriptionPrefs {
    private static final String TAG = "BraveOriginSubsPrefs";
    private static final String ORIGIN_SKU_HOSTNAME_PART = "origin";

    // JSON field names for credential summary response
    private static final String JSON_FIELD_ACTIVE = "active";
    private static final String JSON_FIELD_REMAINING_CREDENTIAL_COUNT =
            "remaining_credential_count";

    // JSON field names for order creation request
    private static final String JSON_FIELD_TYPE = "type";
    private static final String JSON_FIELD_RAW_RECEIPT = "raw_receipt";
    private static final String JSON_FIELD_PACKAGE = "package";
    private static final String JSON_FIELD_SUBSCRIPTION_ID = "subscription_id";
    private static final String JSON_VALUE_ANDROID = "android";

    /**
     * Sets the Origin subscription active status for the given profile.
     *
     * @param profile The profile to use for preference storage
     * @param value The subscription active status
     */
    public static void setIsSubscriptionActive(@Nullable Profile profile, boolean value) {
        if (profile == null) {
            Log.e(TAG, "setIsSubscriptionActive profile is null");
            return;
        }
        UserPrefs.get(profile)
                .setBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID, value);
    }

    /**
     * Gets the Origin subscription active status for the given profile.
     *
     * @param profile The profile to use for preference retrieval
     * @return The subscription active status, or false if profile is null
     */
    public static boolean getIsSubscriptionActive(@Nullable Profile profile) {
        if (profile == null) {
            Log.e(TAG, "getIsSubscriptionActive profile is null");
            return false;
        }
        return UserPrefs.get(profile)
                .getBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID);
    }

    /**
     * Sets the Origin subscription purchase token for the given profile.
     *
     * @param profile The profile to use for preference storage
     * @param token The purchase token
     */
    public static void setOriginPurchaseToken(@Nullable Profile profile, String token) {
        if (profile == null) {
            Log.e(TAG, "setOriginPurchaseToken profile is null");
            return;
        }
        PrefService prefService = UserPrefs.get(profile);
        if (prefService.getString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID).equals(token)
                && !prefService.getString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID).isEmpty()) {
            return;
        }
        // It means we don't have a Play Store subscription anymore or
        // we have a new one.
        resetSubscriptionLinkedStatus(profile);
        prefService.setString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID, token);
        if (!token.isEmpty()) {
            createFetchOrder(profile, token);
        }
    }

    /**
     * Sets the Origin subscription package name for the given profile.
     *
     * @param profile The profile to use for preference storage
     */
    public static void setOriginPackageName(@Nullable Profile profile) {
        if (profile == null) {
            Log.e(TAG, "setOriginPackageName profile is null");
            return;
        }
        UserPrefs.get(profile)
                .setString(
                        BravePref.BRAVE_ORIGIN_PACKAGE_NAME_ANDROID,
                        ContextUtils.getApplicationContext().getPackageName());
    }

    /**
     * Sets the Origin subscription product ID for the given profile.
     *
     * @param profile The profile to use for preference storage
     * @param productId The product ID
     */
    public static void setOriginProductId(@Nullable Profile profile, String productId) {
        if (profile == null) {
            Log.e(TAG, "setOriginProductId profile is null");
            return;
        }
        UserPrefs.get(profile).setString(BravePref.BRAVE_ORIGIN_PRODUCT_ID_ANDROID, productId);
    }

    /**
     * Checks if the Origin subscription is linked for the given profile.
     *
     * @param profile The profile to use for preference retrieval
     * @return True if subscription is linked, false otherwise
     */
    public static boolean isSubscriptionLinked(@Nullable Profile profile) {
        if (profile == null) {
            Log.e(TAG, "isSubscriptionLinked profile is null");
            return false;
        }

        return UserPrefs.get(profile)
                        .getInteger(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_LINK_STATUS_ANDROID)
                != 0;
    }

    /**
     * Resets the subscription linked status for the given profile.
     *
     * @param profile The profile to use for preference storage
     */
    private static void resetSubscriptionLinkedStatus(@Nullable Profile profile) {
        if (profile == null) {
            Log.e(TAG, "resetSubscriptionLinkedStatus profile is null");
            return;
        }
        UserPrefs.get(profile)
                .setInteger(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_LINK_STATUS_ANDROID, 0);
    }

    /**
     * Creates an order for the Origin subscription.
     *
     * @param profile The profile to use for the operation
     * @param purchaseToken The purchase token to use for the operation
     */
    private static void createFetchOrder(Profile profile, String purchaseToken) {
        PrefService prefService = UserPrefs.get(profile);
        String packageName = prefService.getString(BravePref.BRAVE_ORIGIN_PACKAGE_NAME_ANDROID);
        String productId = prefService.getString(BravePref.BRAVE_ORIGIN_PRODUCT_ID_ANDROID);

        // Perform JSON generation and Base64 encoding on background thread
        PostTask.postTask(
                TaskTraits.BEST_EFFORT_MAY_BLOCK,
                () -> {
                    String encodedRequestJson;
                    try {
                        // Create JSON request
                        JSONObject request = new JSONObject();
                        request.put(JSON_FIELD_TYPE, JSON_VALUE_ANDROID);
                        request.put(JSON_FIELD_RAW_RECEIPT, purchaseToken);
                        request.put(JSON_FIELD_PACKAGE, packageName);
                        request.put(JSON_FIELD_SUBSCRIPTION_ID, productId);

                        String requestJson = request.toString();
                        encodedRequestJson =
                                Base64.encodeToString(
                                        requestJson.getBytes(StandardCharsets.UTF_8),
                                        Base64.NO_WRAP);
                    } catch (JSONException e) {
                        Log.e(TAG, "Failed to create JSON request", e);
                        return;
                    }

                    // Switch back to UI thread for the service call
                    PostTask.postTask(
                            TaskTraits.UI_DEFAULT,
                            () -> {
                                SkusService skusService =
                                        SkusServiceFactory.getInstance()
                                                .getSkusService(profile, null);
                                if (skusService == null) {
                                    Log.e(TAG, "SkusService is null, cannot create order");
                                    return;
                                }
                                String domain =
                                        BraveDomainsUtils.getServicesDomain(
                                                ORIGIN_SKU_HOSTNAME_PART,
                                                ServicesEnvironment.STAGING);
                                skusService.createOrderFromReceipt(
                                        domain,
                                        encodedRequestJson,
                                        (result) -> {
                                            if (result == null
                                                    || result.code != SkusResultCode.OK
                                                    || result.message == null
                                                    || result.message.isEmpty()) {
                                                Log.e(
                                                        TAG,
                                                        "Failed to create order: "
                                                                + (result != null
                                                                        ? result.message
                                                                        : "null result"));
                                                skusService.close();
                                                return;
                                            }
                                            // Fetch order credentials using the same service
                                            fetchOrderCredentials(
                                                    profile, result.message, skusService, domain);
                                        });
                            });
                });
    }

    /**
     * Fetches order credentials for the Origin subscription using the provided order ID and
     * SkusService.
     *
     * @param profile The profile to use for the operation
     * @param orderId The order ID to fetch credentials for
     * @param skusService The SkusService instance to use for the operation
     */
    private static void fetchOrderCredentials(
            Profile profile, @Nullable String orderId, SkusService skusService, String domain) {
        if (orderId == null || orderId.isEmpty()) {
            skusService.close();
            return;
        }

        skusService.fetchOrderCredentials(
                domain,
                orderId,
                (result) -> {
                    try {
                        if (result == null || result.code != SkusResultCode.OK) {
                            Log.e(
                                    TAG,
                                    "Failed to fetch order credentials for order ID: "
                                            + orderId
                                            + " "
                                            + (result != null ? result.message : "null result"));
                            return;
                        }
                        // Store the order ID
                        UserPrefs.get(profile)
                                .setString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID, orderId);
                    } finally {
                        skusService.close();
                    }
                });
    }

    /**
     * Requests credential summary for the Origin subscription.
     *
     * @param profile The profile to use for the operation
     * @param callback Callback to handle the credential summary result - true if active, false if
     *     not
     */
    public static void requestCredentialSummary(
            @Nullable Profile profile, @Nullable Callback<Boolean> callback) {
        if (profile == null) {
            Log.e(TAG, "requestCredentialSummary profile is null");
            if (callback != null) {
                callback.onResult(false);
            }
            return;
        }

        SkusService skusService = SkusServiceFactory.getInstance().getSkusService(profile, null);
        if (skusService == null) {
            Log.e(TAG, "SkusService is null, cannot request credential summary");
            if (callback != null) {
                callback.onResult(false);
            }
            return;
        }

        String domain =
                BraveDomainsUtils.getServicesDomain(
                        ORIGIN_SKU_HOSTNAME_PART, ServicesEnvironment.STAGING);
        skusService.credentialSummary(
                domain,
                (result) -> {
                    try {
                        // Move JSON parsing to background thread to avoid potential UI blocking
                        PostTask.postTask(
                                TaskTraits.BEST_EFFORT,
                                () -> {
                                    boolean isActive = parseCredentialSummary(result);
                                    // Switch back to UI thread for callback
                                    PostTask.postTask(
                                            TaskTraits.UI_DEFAULT,
                                            () -> {
                                                if (callback != null) {
                                                    callback.onResult(isActive);
                                                }
                                            });
                                });
                    } finally {
                        skusService.close();
                    }
                });
    }

    /**
     * Parses the credential summary result on a background thread. This avoids potential UI
     * blocking for JSON parsing operations.
     *
     * @param summary The credential summary result
     * @return true if subscription is active, false otherwise
     */
    private static boolean parseCredentialSummary(@Nullable SkusResult summary) {
        if (summary == null || summary.code != SkusResultCode.OK) {
            return false;
        }

        String summaryMessage = summary.message != null ? summary.message.trim() : "";
        if (summaryMessage.isEmpty()) {
            return false;
        }

        try {
            // Parse JSON response
            JSONObject records = new JSONObject(summaryMessage);
            // Empty dict - clean user
            if (records.length() == 0) {
                return false;
            }

            // Check if credential is valid (has active status and remaining credentials)
            boolean active = records.optBoolean(JSON_FIELD_ACTIVE, false);
            int remainingCredentialCount = records.optInt(JSON_FIELD_REMAINING_CREDENTIAL_COUNT, 0);

            return active && remainingCredentialCount > 0;
        } catch (JSONException e) {
            Log.e(TAG, "Failed to parse credential summary JSON", e);
            return false;
        }
    }

    /**
     * Clears all Origin subscription preferences for the given profile.
     *
     * @param profile The profile to use for preference clearing
     */
    public static void clearOriginSubscriptionPrefs(@Nullable Profile profile) {
        if (profile == null) {
            Log.e(TAG, "clearOriginSubscriptionPrefs profile is null");
            return;
        }

        PrefService prefService = UserPrefs.get(profile);
        prefService.setBoolean(BravePref.BRAVE_ORIGIN_SUBSCRIPTION_ACTIVE_ANDROID, false);
        prefService.setString(BravePref.BRAVE_ORIGIN_PURCHASE_TOKEN_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_PRODUCT_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_ORDER_ID_ANDROID, "");
        prefService.setString(BravePref.BRAVE_ORIGIN_PACKAGE_NAME_ANDROID, "");
        resetSubscriptionLinkedStatus(profile);
    }

    /**
     * Opens the Brave Origin preferences settings screen.
     *
     * @param activity The activity to use for launching the settings
     */
    public static void openOriginPreferences(@Nullable Activity activity) {
        if (activity == null || activity.isFinishing()) {
            Log.e(TAG, "openOriginPreferences activity is null or finishing");
            return;
        }
        SettingsNavigationFactory.createSettingsNavigation()
                .startSettings(activity, BraveOriginPreferences.class);
    }
}
