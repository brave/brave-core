/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.toolbar.adaptive.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.widget.RadioGroup;

import androidx.annotation.VisibleForTesting;
import androidx.preference.PreferenceViewHolder;

import org.chromium.base.BraveFeatureList;
import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;
import org.chromium.chrome.browser.flags.ChromeFeatureList;
import org.chromium.chrome.browser.toolbar.R;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarButtonVariant;
import org.chromium.chrome.browser.toolbar.adaptive.AdaptiveToolbarStatePredictor.UiState;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.ui.base.DeviceFormFactor;

import java.util.ArrayList;

/** Brave's fragment that allows the user to configure toolbar shortcut preferences. */
@NullMarked
public class BraveRadioButtonGroupAdaptiveToolbarPreference
        extends RadioButtonGroupAdaptiveToolbarPreference {
    private static boolean sIsJunitTesting;

    // Variables below are to be removed in the bytecode, variables from the parent class will be
    // used instead.
    private @AdaptiveToolbarButtonVariant int mSelected;
    private boolean mIsBound;
    private @Nullable RadioButtonWithDescription mAutoButton;
    private @Nullable RadioButtonWithDescription mNewTabButton;
    private @Nullable RadioButtonWithDescription mShareButton;

    // Own members.
    private final Context mContext;
    private @Nullable RadioButtonWithDescription mBookmarksButton;
    private @Nullable RadioButtonWithDescription mHistoryButton;
    private @Nullable RadioButtonWithDescription mDownloadsButton;
    private @Nullable RadioButtonWithDescription mBraveLeoButton;
    private @Nullable RadioButtonWithDescription mBraveWalletButton;

    public BraveRadioButtonGroupAdaptiveToolbarPreference(Context context, AttributeSet attrs) {
        super(context, attrs);

        mContext = context;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        // Needs to be done before super is called, otherwise the button will be null in getButton
        // method.
        mBookmarksButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_bookmarks);
        mHistoryButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_history);
        mDownloadsButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_downloads);
        mBraveLeoButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_brave_leo);
        if (!sIsJunitTesting
                && mBraveLeoButton != null
                && !ChromeFeatureList.isEnabled(BraveFeatureList.AI_CHAT)) {
            mBraveLeoButton.setVisibility(View.GONE);
        }
        mBraveWalletButton =
                (RadioButtonWithDescription) holder.findViewById(R.id.adaptive_option_brave_wallet);
        if (!sIsJunitTesting
                && mBraveWalletButton != null
                && !ChromeFeatureList.isEnabled(BraveFeatureList.NATIVE_BRAVE_WALLET)) {
            mBraveWalletButton.setVisibility(View.GONE);
        }

        super.onBindViewHolder(holder);

        // We don't have Auto option in Brave, so we hide it.
        assert mAutoButton != null : "mAutoButton should not be null at this point";
        if (mAutoButton != null) {
            mAutoButton.setVisibility(View.GONE);
        }
    }

    @Override
    public void onCheckedChanged(@Nullable RadioGroup group, int checkedId) {
        if (!mIsBound) return;

        RadioButtonWithDescription defaultButton =
                DeviceFormFactor.isNonMultiDisplayContextOnTablet(mContext)
                        ? mShareButton
                        : mNewTabButton;
        if (mAutoButton != null && defaultButton != null && checkedId == mAutoButton.getId()) {
            // Redirect the state of the Auto button to the default button.
            checkedId = defaultButton.getId();
            defaultButton.setChecked(mAutoButton.isChecked());
        }

        boolean isOnCheckedChangedHandled = false;
        if (mBookmarksButton != null && mBookmarksButton.isChecked()) {
            mSelected = AdaptiveToolbarButtonVariant.BOOKMARKS;
            isOnCheckedChangedHandled = true;
        } else if (mHistoryButton != null && mHistoryButton.isChecked()) {
            mSelected = AdaptiveToolbarButtonVariant.HISTORY;
            isOnCheckedChangedHandled = true;
        } else if (mDownloadsButton != null && mDownloadsButton.isChecked()) {
            mSelected = AdaptiveToolbarButtonVariant.DOWNLOADS;
            isOnCheckedChangedHandled = true;
        } else if (mBraveLeoButton != null && mBraveLeoButton.isChecked()) {
            mSelected = AdaptiveToolbarButtonVariant.LEO;
            isOnCheckedChangedHandled = true;
        } else if (mBraveWalletButton != null && mBraveWalletButton.isChecked()) {
            mSelected = AdaptiveToolbarButtonVariant.WALLET;
            isOnCheckedChangedHandled = true;
        }
        if (isOnCheckedChangedHandled) {
            callChangeListener(mSelected);
            return;
        }

        super.onCheckedChanged(group, checkedId);
    }

    @Override
    @Nullable RadioButtonWithDescription getButton(@AdaptiveToolbarButtonVariant int variant) {
        switch (variant) {
            case AdaptiveToolbarButtonVariant.BOOKMARKS:
                return mBookmarksButton;
            case AdaptiveToolbarButtonVariant.HISTORY:
                return mHistoryButton;
            case AdaptiveToolbarButtonVariant.DOWNLOADS:
                return mDownloadsButton;
            case AdaptiveToolbarButtonVariant.LEO:
                return mBraveLeoButton;
            case AdaptiveToolbarButtonVariant.WALLET:
                return mBraveWalletButton;
        }

        return super.getButton(variant);
    }

    public UiState buildUiStateForStats() {
        // For stats always report New Tab button just to avoid assert on Brave variants.
        ArrayList<Integer> buttonList = new ArrayList<>();
        buttonList.add(AdaptiveToolbarButtonVariant.UNKNOWN);
        return new UiState(
                /* canShowUi= */ true,
                buttonList,
                AdaptiveToolbarButtonVariant.NEW_TAB,
                AdaptiveToolbarButtonVariant.NEW_TAB);
    }

    @VisibleForTesting
    public static void setIsJunitTesting(boolean isJunitTesting) {
        sIsJunitTesting = isJunitTesting;
    }
}
