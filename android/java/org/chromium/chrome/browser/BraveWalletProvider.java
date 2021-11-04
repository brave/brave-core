/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */
package org.chromium.chrome.browser;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.net.Uri;
import android.text.TextUtils;

import org.json.JSONException;
import org.json.JSONObject;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsBalance;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.BraveRewardsNativeWorker;
import org.chromium.chrome.browser.BraveRewardsObserver;
import org.chromium.chrome.browser.app.BraveActivity;
import org.chromium.chrome.browser.externalnav.BraveExternalNavigationHandler;
import org.chromium.components.external_intents.ExternalNavigationParams;

import java.util.Locale;

// Used from org.chromium.chrome.browser.externalnav
public class BraveWalletProvider implements BraveRewardsObserver {
    public static final String ACTION_VALUE = "authorization";

    public static final String REDIRECT_URL_KEY = "redirect_url";

    public static final String UPHOLD_REDIRECT_URL = "rewards://uphold";
    public static final String UPHOLD_SUPPORT_URL = "http://uphold.com/en/brave/support";
    public static final String UPHOLD_ORIGIN_URL = "http://uphold.com";

    public static final String BITFLYER_REDIRECT_URL = "rewards://bitflyer";

    // Wallet types
    public static final String UPHOLD = "uphold";
    public static final String BITFLYER = "bitflyer";

    private static int UNKNOWN_ERROR_CODE = -1;

    private ExternalNavigationParams mExternalNavigationParams;
    private BraveExternalNavigationHandler mBraveExternalNavigationHandler;
    private BraveRewardsNativeWorker rewardsNativeProxy;

    public void completeWalletProviderVerification(
            ExternalNavigationParams params, BraveExternalNavigationHandler handler) {
        mExternalNavigationParams = params;
        mBraveExternalNavigationHandler = handler;
        rewardsNativeProxy = BraveRewardsNativeWorker.getInstance();

        Uri uri = Uri.parse(params.getUrl().getSpec());
        rewardsNativeProxy.AddObserver(this);
        String path = uri.getPath();
        String query = uri.getQuery();

        if (TextUtils.isEmpty(path) || TextUtils.isEmpty(query)) {
            rewardsNativeProxy.RemoveObserver(this);
            releaseDependencies();
            showErrorMessageBox(UNKNOWN_ERROR_CODE);
            return;
        }

        // Ledger expects:
        // path: "/provider/path"
        // query: "?query"
        path = String.format(Locale.US, "/%s/%s", rewardsNativeProxy.getExternalWalletType(), path);
        query = String.format(Locale.US, "?%s", query);
        rewardsNativeProxy.ProcessRewardsPageUrl(path, query);
    }

    @Override
    public void OnProcessRewardsPageUrl(
            int errorCode, String walletType, String action, String jsonArgs) {
        // remove observer
        if (rewardsNativeProxy != null) {
            rewardsNativeProxy.RemoveObserver(this);
        }

        String redirectUrl = parseJsonArgs(jsonArgs);
        if (BraveRewardsNativeWorker.LEDGER_OK == errorCode
                && TextUtils.equals(action, ACTION_VALUE)) {
            // wallet is verified: redirect to chrome://rewards for now
            if (TextUtils.isEmpty(redirectUrl)) {
                redirectUrl = BraveActivity.REWARDS_SETTINGS_URL;
            }
            mBraveExternalNavigationHandler.clobberCurrentTabWithFallbackUrl(
                    redirectUrl, mExternalNavigationParams);

            // temporary: open Rewards Panel to show wallet transition state.
            // remove this step once settings activity has this info
            BraveActivity.getBraveActivity().openRewardsPanel();
            releaseDependencies();
        } else {
            showErrorMessageBox(errorCode);
        }
    }

    private String parseJsonArgs(String jsonArgs) {
        String redirect_url = "";
        try {
            JSONObject jsonObj = new JSONObject(jsonArgs);
            if (jsonObj.has(REDIRECT_URL_KEY)) {
                redirect_url = jsonObj.getString(REDIRECT_URL_KEY);
            }
        } catch (JSONException e) {
        }
        return redirect_url;
    }

    private void releaseDependencies() {
        mExternalNavigationParams = null;
        mBraveExternalNavigationHandler = null;
    }

    private void showErrorMessageBox(int errorCode) {
        String message = "";
        String messageTitle = "";
        Context context = ContextUtils.getApplicationContext();
        AlertDialog.Builder builder = new AlertDialog.Builder(
                BraveRewardsHelper.getChromeTabbedActivity(), R.style.Theme_Chromium_AlertDialog);

        if (BraveRewardsNativeWorker.BAT_NOT_ALLOWED == errorCode) {
            message = context.getResources().getString(R.string.bat_not_allowed_in_region);
            messageTitle =
                    context.getResources().getString(R.string.bat_not_allowed_in_region_title);
        } else {
            message = context.getResources().getString(R.string.wallet_verification_generic_error);
            messageTitle = context.getResources().getString(
                    R.string.wallet_verification_generic_error_title);
        }
        builder.setMessage(message)
                .setTitle(messageTitle)
                .setPositiveButton(R.string.ok,
                        (DialogInterface dialog, int which) -> {
                            mBraveExternalNavigationHandler.clobberCurrentTabWithFallbackUrl(
                                    UPHOLD_SUPPORT_URL, mExternalNavigationParams);
                        })
                .setOnDismissListener((DialogInterface dialog) -> { releaseDependencies(); });

        AlertDialog dialog = builder.create();
        dialog.show();
    }
}
