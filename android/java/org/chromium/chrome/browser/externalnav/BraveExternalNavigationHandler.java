/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.externalnav;

import android.content.Intent;

import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.chrome.browser.privacy.settings.BravePrivacySettings;
import org.chromium.components.external_intents.ExternalNavigationDelegate;
import org.chromium.components.external_intents.ExternalNavigationHandler;
import org.chromium.components.external_intents.ExternalNavigationHandler.OverrideUrlLoadingResult;
import org.chromium.components.external_intents.ExternalNavigationParams;
import org.chromium.url.GURL;

/**
 * Extends Chromium's ExternalNavigationHandler
 */
public class BraveExternalNavigationHandler extends ExternalNavigationHandler {
    private BraveWalletProvider mBraveWalletProvider;

    public BraveExternalNavigationHandler(ExternalNavigationDelegate delegate) {
        super(delegate);
    }

    @Override
    public OverrideUrlLoadingResult shouldOverrideUrlLoading(ExternalNavigationParams params) {
        // TODO: Once we have a ready for https://github.com/brave/brave-browser/issues/33015, We'll
        // use this code
        /*if (originalUrl.equalsIgnoreCase("chrome://adblock/")) {
            try {
                BraveActivity.getBraveActivity().openBraveContentFilteringSettings();
            } catch (BraveActivity.BraveActivityNotFoundException e) {
                Log.e(TAG, "adblock url " + e);
            }
            return OverrideUrlLoadingResult.forExternalIntent();
        }*/
        return super.shouldOverrideUrlLoading(params);
    }

    @Override
    protected OverrideUrlLoadingResult startActivity(Intent intent, boolean requiresIntentChooser,
            QueryIntentActivitiesSupplier resolvingInfos, ResolveActivitySupplier resolveActivity,
            GURL browserFallbackUrl, GURL intentDataUrl, ExternalNavigationParams params) {
        if (ContextUtils.getAppSharedPreferences().getBoolean(
                    BravePrivacySettings.PREF_APP_LINKS, true)) {
            return super.startActivity(intent, requiresIntentChooser, resolvingInfos,
                    resolveActivity, browserFallbackUrl, intentDataUrl, params);
        } else {
            return OverrideUrlLoadingResult.forNoOverride();
        }
    }
}
