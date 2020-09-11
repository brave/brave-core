/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.widget.tile;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.TextView;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.BraveRewardsHelper;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.preferences.BravePref;
import org.chromium.components.user_prefs.UserPrefs;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.suggestions.tile.TileWithTextView;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;

public class BraveTileWithTextView extends TileWithTextView {
    public BraveTileWithTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setTitle(String title, int titleLines) {
        super.setTitle(title, titleLines);
        TextView mTitleView = findViewById(R.id.tile_view_title);
        if (UserPrefs.get(Profile.getLastUsedRegularProfile()).getBoolean(BravePref.NEW_TAB_PAGE_SHOW_BACKGROUND_IMAGE)
                && NTPUtil.shouldEnableNTPFeature()) {
            mTitleView.setTextColor(getResources().getColor(android.R.color.white));
        }
    }
}
