/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.brave_leo;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;

import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;

import org.jni_zero.JNINamespace;
import org.jni_zero.NativeMethods;

import org.chromium.ai_chat.mojom.ModelWithSubtitle;
import org.chromium.base.Callback;
import org.chromium.base.Log;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.billing.InAppPurchaseWrapper;
import org.chromium.chrome.browser.billing.PurchaseModel;
import org.chromium.chrome.browser.util.LiveDataUtil;
import org.chromium.chrome.browser.util.TabUtils;
import org.chromium.content_public.browser.WebContents;

@JNINamespace("ai_chat")
public class BraveLeoUtils {
    private static final String TAG = "BraveLeoUtils";

    public static void verifySubscription(Callback callback) {
        MutableLiveData<PurchaseModel> _activePurchases = new MutableLiveData();
        LiveData<PurchaseModel> activePurchases = _activePurchases;
        InAppPurchaseWrapper.getInstance()
                .queryPurchases(_activePurchases, InAppPurchaseWrapper.SubscriptionProduct.LEO);
        LiveDataUtil.observeOnce(
                activePurchases,
                activePurchaseModel -> {
                    BraveLeoPrefUtils.setIsSubscriptionActive(activePurchaseModel != null);
                    if (activePurchaseModel != null) {
                        BraveLeoPrefUtils.setChatPackageName();
                        BraveLeoPrefUtils.setChatProductId(activePurchaseModel.getProductId());
                        BraveLeoPrefUtils.setChatPurchaseToken(
                                activePurchaseModel.getPurchaseToken());
                    } else {
                        BraveLeoPrefUtils.setChatProductId("");
                        BraveLeoPrefUtils.setChatPurchaseToken("");
                    }
                    if (callback != null) {
                        callback.onResult(null);
                    }
                });
    }

    public static void openLeoQuery(
            WebContents webContents,
            String conversationUuid,
            String query,
            boolean openLeoChatWindow) {
        BraveLeoUtilsJni.get().openLeoQuery(webContents, conversationUuid, query);
    }

    public static String getDefaultModelName(ModelWithSubtitle[] models, String defaultModelKey) {
        for (ModelWithSubtitle model : models) {
            if (model.model.key.equals(defaultModelKey)) {
                return model.model.displayName;
            }
        }

        return "";
    }

    public static void openManageSubscription() {
        try {
            BraveActivity activity = BraveActivity.getBraveActivity();
            Intent browserIntent = new Intent(
                    Intent.ACTION_VIEW,
                    Uri.parse(InAppPurchaseWrapper.MANAGE_SUBSCRIPTION_PAGE));
            activity.startActivity(browserIntent);
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "openManageSubscription get BraveActivity exception", e);
        }
    }

    public static void goPremium(Activity activity) {
        Intent braveLeoPlansIntent = new Intent(activity, BraveLeoPlansActivity.class);
        braveLeoPlansIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        braveLeoPlansIntent.setAction(Intent.ACTION_VIEW);
        activity.startActivity(braveLeoPlansIntent);
    }

    public static void bringMainActivityOnTop() {
        try {
            TabUtils.bringChromeTabbedActivityToTheTop(BraveActivity.getBraveActivity());
        } catch (BraveActivity.BraveActivityNotFoundException e) {
            Log.e(TAG, "bringMainActivityOnTop error", e);
        }
    }

    @NativeMethods
    public interface Natives {
        void openLeoQuery(WebContents webContents, String conversationUuid, String query);
    }
}
