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
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;

import androidx.annotation.VisibleForTesting;

import org.chromium.base.ApplicationStatus;
import org.chromium.base.BraveFeatureList;
import org.chromium.base.BravePreferenceKeys;
import org.chromium.chrome.browser.brave_shields.FirstPartyStorageCleanerInterface;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.incognito.reauth.IncognitoReauthManager;
import org.chromium.chrome.browser.preferences.ChromeSharedPreferences;
import org.chromium.chrome.browser.toolbar.settings.AddressBarPreference;

/**
 * Brave's extension for {@link HubToolbarView}. Here we control what elements should be visible in
 * tab switcher mode when bottom toolbar is visible.
 */
public class BraveHubToolbarView extends HubToolbarView
        implements ApplicationStatus.TaskVisibilityListener,
                IncognitoReauthManager.IncognitoReauthCallback {
    private Button mActionButton;
    private Button mShredButton;
    private FrameLayout mMenuButton;
    private boolean mIsIncognitoSelected = true;
    private FirstPartyStorageCleanerInterface mFpCleaner;

    public BraveHubToolbarView(Context context, AttributeSet attributeSet) {
        super(context, attributeSet);
        if (context instanceof FirstPartyStorageCleanerInterface) {
            mFpCleaner = (FirstPartyStorageCleanerInterface) context;
            mFpCleaner.setShredButtonVisibilityObserver(this);
        }
        ApplicationStatus.registerTaskVisibilityListener(this);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        mActionButton = findViewById(R.id.toolbar_action_button);
        mShredButton =
                findViewById(org.chromium.chrome.browser.brave_shields.R.id.shred_data_button);
        mMenuButton = findViewById(R.id.menu_button_wrapper);

        mShredButton.setOnClickListener(
                v -> {
                    if (mFpCleaner != null) {
                        mFpCleaner.shredSiteData();
                    }
                });
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        ApplicationStatus.unregisterTaskVisibilityListener(this);
        if (mFpCleaner != null) {
            mFpCleaner.removeShredButtonVisibilityObserver(this);
        }
    }

    @Override
    void setPaneSwitcherIndex(int index) {
        super.setPaneSwitcherIndex(index);

        // Update visibility of action and menu buttons based on the switching panes.
        updateButtonsVisibility();
    }

    @Override
    void updateIncognitoElements(boolean isIncognito) {
        super.updateIncognitoElements(isIncognito);
        mIsIncognitoSelected = isIncognito;
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

    @Override
    public void onIncognitoReauthSuccess() {
        updateButtonsVisibility();
    }

    @Override
    public void onIncognitoReauthNotPossible() {}

    @Override
    public void onIncognitoReauthFailure() {}

    @Override
    public void onTaskVisibilityChanged(int taskId, boolean isVisible) {
        updateButtonsVisibility();
    }

    @VisibleForTesting(otherwise = VisibleForTesting.PRIVATE)
    public void setFirstPartyStorageCleanerForTesting(
            FirstPartyStorageCleanerInterface firstPartyStorageCleaner) {
        mFpCleaner = firstPartyStorageCleaner;
    }

    private void updateButtonsVisibility() {
        boolean shouldHideButtons =
                AddressBarPreference.isToolbarConfiguredToShowOnTop()
                        && ChromeSharedPreferences.getInstance()
                                .readBoolean(BravePreferenceKeys.BRAVE_IS_MENU_FROM_BOTTOM, true);

        mActionButton.setVisibility(shouldHideButtons ? View.GONE : View.VISIBLE);
        mMenuButton.setVisibility(shouldHideButtons ? View.GONE : View.VISIBLE);

        // When the menu button is GONE the shred button becomes the rightmost child of
        // menu_button_container; give it an end margin matching the toolbar's start
        // inset so the shred icon doesn't sit flush against the screen edge.
        if (shouldHideButtons) {
            ViewGroup.MarginLayoutParams shredLp =
                    (ViewGroup.MarginLayoutParams) mShredButton.getLayoutParams();
            int endMargin =
                    getResources()
                            .getDimensionPixelSize(R.dimen.hub_toolbar_action_button_start_margin);
            if (shredLp.getMarginEnd() != endMargin) {
                shredLp.setMarginEnd(endMargin);
                mShredButton.setLayoutParams(shredLp);
            }
        }

        final boolean isShredButtonVisible =
                (!mIsIncognitoSelected
                        || (mFpCleaner != null ? mFpCleaner.isShredButtonVisible() : true));
        final boolean shouldShowShredButton =
                ChromeFeatureList.isEnabled(BraveFeatureList.BRAVE_SHRED) && isShredButtonVisible;
        mShredButton.setVisibility(shouldShowShredButton ? View.VISIBLE : View.INVISIBLE);
    }
}
