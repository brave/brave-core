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
        if (isWalletProviderOverride(params)) {
            String originalUrl = params.getUrl().getSpec();
            String url = originalUrl.replaceFirst("^rewards://", "brave://rewards/");
            GURL browserFallbackGURL = new GURL(url);
            if (params.getRedirectHandler() != null) {
                params.getRedirectHandler().setShouldNotOverrideUrlLoadingOnCurrentRedirectChain();
            }
            return OverrideUrlLoadingResult.forNavigateTab(browserFallbackGURL, params);
        }
        return super.shouldOverrideUrlLoading(params);
    }

    private boolean isWalletProviderOverride(ExternalNavigationParams params) {
        if (params.getUrl().getSpec().startsWith(BraveWalletProvider.UPHOLD_REDIRECT_URL)) {
            return true;
        }

        if (params.getUrl().getSpec().startsWith(BraveWalletProvider.BITFLYER_REDIRECT_URL)) {
            return true;
        }

        if (params.getUrl().getSpec().startsWith(BraveWalletProvider.GEMINI_REDIRECT_URL)) {
            return true;
        }

        return false;
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
