/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;

import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.ContextCompat;
import androidx.core.widget.ImageViewCompat;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.omnibox.status.StatusView;
import org.chromium.chrome.browser.preferences.website.BraveShieldsContentSettings;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.tab.Tab;

public class BraveLocationBarLayout extends LocationBarLayout {
    private final ImageButton mQRButton;
    private StatusView mStatusView;

    public BraveLocationBarLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mQRButton = findViewById(R.id.qr_button);
    }

    public BraveLocationBarLayout(Context context, AttributeSet attrs, int layoutId) {
        super(context, attrs, layoutId);
        mQRButton = findViewById(R.id.qr_button);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();

        mStatusView = findViewById(R.id.location_bar_status);
        mStatusView.setBackgroundDrawable(null);

        // Ensure these location bar buttons are aligned to parent by top/bottom. Otherwise they
        // look as misaligned.
        tieTopAndBottomToParent(R.id.location_bar_status);
        tieTopAndBottomToParent(R.id.bookmark_button);
        tieTopAndBottomToParent(R.id.mic_button);
        tieTopAndBottomToParent(R.id.delete_button);
        tieTopAndBottomToParent(R.id.install_button);
        // TODO: This should almost certainly be a callback, polling is hacky.
	postDelayed(mShieldIconUpdater, 50);
    }

    private final Runnable mShieldIconUpdater =
            new Runnable() {
                @Override
                public void run() {
                    boolean shouldContinue = updateShieldsIcon();
                    // Only continue updating if we're on an http(s) page
                    if (shouldContinue) {
                        postDelayed(this, 50);
                    } else {
                        // Stop the updater for non-http(s) pages and try again later
                        postDelayed(this, 500);
                    }
                }
            };

    public StatusView getStatusView() {
        return mStatusView;
    }

    void tieTopAndBottomToParent(int id) {
        View view = findViewById(id);
        ConstraintLayout.LayoutParams params =
                (ConstraintLayout.LayoutParams) view.getLayoutParams();
        params.bottomToBottom = ConstraintLayout.LayoutParams.PARENT_ID;
        params.topToTop = ConstraintLayout.LayoutParams.PARENT_ID;
        view.setLayoutParams(params);
    }

    // public void setCurrentTab(Tab tab) {
    // No longer needed - using mLocationBarDataProvider instead
    // Don't call updateShieldsIcon here as it might set icon too early
    // }

    private boolean updateShieldsIcon() {
        // Use the LocationBarDataProvider to get the current tab and URL
        if (mLocationBarDataProvider == null || mStatusView == null) {
            return false;
        }

        Tab currentTab = mLocationBarDataProvider.getTab();
        if (currentTab == null || currentTab.getUrl() == null) {
            return false;
        }

        String urlSpec = currentTab.getUrl().getSpec();

        // Early check: don't touch the icon for non-http(s) URLs
        // This includes chrome://, chrome-native://, about:, data:, etc.
        if (!urlSpec.startsWith("http://") && !urlSpec.startsWith("https://")) {
            return false;
        }

        try {
            // Get the profile from the current tab to ensure we're checking the right one
            // (important for incognito vs regular profiles)
            if (currentTab.getWebContents() == null) {
                return false;
            }
            Profile profile = Profile.fromWebContents(currentTab.getWebContents());

            // Use the full URL spec (not just host) as that's what the shields API expects
            boolean isShieldsEnabled =
                    BraveShieldsContentSettings.getShields(
                            profile,
                            urlSpec,
                            BraveShieldsContentSettings.RESOURCE_IDENTIFIER_BRAVE_SHIELDS);

            int iconRes =
                    isShieldsEnabled
                            ? R.drawable.ic_shield_done
                            : R.drawable.ic_shield_done_outlined;

            // Find the ImageView inside StatusView and update its drawable
            ImageView iconView = mStatusView.findViewById(R.id.location_bar_status_icon);
            if (iconView != null) {
                Drawable drawable = ContextCompat.getDrawable(getContext(), iconRes);
                iconView.setImageDrawable(drawable);
            }
            return true;
        } catch (Exception e) {
            // Ignore invalid URLs
            return false;
        }
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        // Stop the icon updater
        removeCallbacks(mShieldIconUpdater);
    }

    void setQRButtonTint(ColorStateList colorStateList) {
        ImageViewCompat.setImageTintList(mQRButton, colorStateList);
    }

    void setQRButtonVisibility(boolean shouldShow) {
        mQRButton.setVisibility(shouldShow ? VISIBLE : GONE);
    }
}
