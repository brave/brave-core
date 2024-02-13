/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.suggestions.tile;

import android.content.Context;
import android.util.AttributeSet;
import android.view.ViewGroup;
import android.widget.TextView;

import org.chromium.base.Log;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.profiles.ProfileManager;
import org.chromium.components.browser_ui.widget.tile.TileView;
import org.chromium.components.user_prefs.UserPrefs;

public class BraveTileView extends TileView {
    private static final String TAG = "BraveTileView";

    public BraveTileView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setTitle(String title, int titleLines) {
        super.setTitle(title, titleLines);
        if (ProfileManager.isInitialized()) {
            TextView mTitleView = findViewById(R.id.tile_view_title);
            if (UserPrefs.get(Profile.getLastUsedRegularProfile())
                    .getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)) {
                mTitleView.setTextColor(
                        getContext().getColor(R.color.brave_state_time_count_color));
                mTitleView.setShadowLayer(
                        18, 0, 0, getContext().getColor(R.color.onboarding_black));
                if (mTitleView.getLayoutParams() instanceof ViewGroup.MarginLayoutParams) {
                    ViewGroup.MarginLayoutParams params =
                            (ViewGroup.MarginLayoutParams) mTitleView.getLayoutParams();
                    params.bottomMargin = getResources().getDimensionPixelSize(
                            R.dimen.tile_view_icon_background_margin_top_modern);
                    mTitleView.setLayoutParams(params);
                }
            }
        } else {
            Log.w(TAG, "Attempt to access profile before native initialization");
        }
    }
}
