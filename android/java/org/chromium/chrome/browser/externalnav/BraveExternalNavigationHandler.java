/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.externalnav;

import android.annotation.SuppressLint;

import org.chromium.chrome.browser.BraveWalletProvider;
import org.chromium.components.external_intents.ExternalNavigationDelegate;
import org.chromium.components.external_intents.ExternalNavigationHandler;
import org.chromium.components.external_intents.ExternalNavigationHandler.OverrideUrlLoadingResult;
import org.chromium.components.external_intents.ExternalNavigationParams;
import org.chromium.url.GURL;

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
}
