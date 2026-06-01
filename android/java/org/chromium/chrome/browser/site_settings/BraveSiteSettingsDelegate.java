/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.site_settings;

import android.app.Activity;
import android.content.Context;
import android.graphics.drawable.Drawable;

import org.chromium.base.Callback;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.browsing_data.BraveClearBrowsingDataFragment;
import org.chromium.chrome.browser.browsing_data.ClearBrowsingDataFragment;
import org.chromium.chrome.browser.crypto_wallet.BraveWalletPolicy;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.settings.SettingsNavigationFactory;
import org.chromium.components.browser_ui.site_settings.BraveWalletSiteSettingsDelegate;
import org.chromium.components.browser_ui.site_settings.SiteSettingsCategory;
import org.chromium.url.GURL;

@NullMarked
public class BraveSiteSettingsDelegate extends ChromeSiteSettingsDelegate
        implements BraveWalletSiteSettingsDelegate {
    // Will be deleted in bytecode, value from the parent class will be used instead.
    @Nullable private Profile mProfile;

    public BraveSiteSettingsDelegate(Context context, Profile profile) {
        super(context, profile);
    }

    /**
     * Launches the brave specific clear browsing data dialog from site settings.
     *
     * @param currentActivity The activity on which the fragment will be attached.
     */
    @Override
    public void launchClearBrowsingDataDialog(Activity currentActivity) {
        SettingsNavigationFactory.createSettingsNavigation()
                .startSettings(
                        currentActivity,
                        BraveClearBrowsingDataFragment.class,
                        ClearBrowsingDataFragment.createFragmentArgs(
                                BraveSiteSettingsDelegate.class.getName()));
    }

    @Override
    public boolean isWalletDisabledByPolicy() {
        return BraveWalletPolicy.isDisabledByPolicy(mProfile);
    }

    @SuppressWarnings("NullAway")
    @Override
    public void getFaviconImageForURL(GURL faviconUrl, Callback<Drawable> callback) {
        if (!faviconUrl.isValid()) {
            callback.onResult(null);
            return;
        }

        super.getFaviconImageForURL(faviconUrl, callback);
    }

    @Override
    public boolean isCategoryVisible(@SiteSettingsCategory.Type int type) {
        switch (type) {
            case SiteSettingsCategory.Type.STORAGE_ACCESS:
                return false;
            case SiteSettingsCategory.Type.ANTI_ABUSE:
                return false;
            default:
                return super.isCategoryVisible(type);
        }
    }
}
