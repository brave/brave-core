/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.hub;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.util.AttributeSet;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;

import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.first_party_storage_cleaner.FirstPartyStorageCleanerInterface;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.toolbar.settings.AddressBarPreference;

/**
 * Brave's extension for {@link HubToolbarView}. Here we control what elements should be visible in
 * tab switcher mode when bottom toolbar is visible.
 */
public class BraveHubToolbarView extends HubToolbarView {
    private Button mActionButton;
    private Button mShredButton;
    private FrameLayout mMenuButton;

    public BraveHubToolbarView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mActionButton = findViewById(R.id.toolbar_action_button);
        mShredButton =
                findViewById(org.chromium.chrome.browser.first_party_storage_cleaner.R.id.shred_data_button);
        mMenuButton = findViewById(R.id.menu_button_wrapper);

        mShredButton.setOnClickListener(
                v -> {
                    Context context = getContext();
                    if (context instanceof FirstPartyStorageCleanerInterface) {
                        FirstPartyStorageCleanerInterface fpCleaner =
                                (FirstPartyStorageCleanerInterface) context;
                        fpCleaner.shredSiteData();
                    }
                });
    }

    @Override
    void setPaneSwitcherIndex(int index) {
        super.setPaneSwitcherIndex(index);

        // Update visibility of action and menu buttons based on the switching panes.
        updateButtonsVisibility();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        Context context = getContext();
        if (context instanceof Activity
                && (((Activity) context).isFinishing() || ((Activity) context).isDestroyed())) {
            return;
        }
        updateButtonsVisibility();
    }

    @Override
    void setMenuButtonVisible(boolean visible) {
        super.setMenuButtonVisible(visible);

        updateButtonsVisibility();
    }

    private void updateButtonsVisibility() {
        boolean shouldHideButtons =
                AddressBarPreference.isToolbarConfiguredToShowOnTop()
                        && ContextUtils.getAppSharedPreferences()
                                .getBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true);

        mActionButton.setVisibility(shouldHideButtons ? View.INVISIBLE : View.VISIBLE);
        mMenuButton.setVisibility(shouldHideButtons ? View.INVISIBLE : View.VISIBLE);

        final boolean shouldShowShredButton =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SHRED);
        mShredButton.setVisibility(shouldShowShredButton ? View.VISIBLE : View.INVISIBLE);
    }
}
