/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.homepage.settings;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.RadioGroup;

import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;
import androidx.preference.PreferenceViewHolder;

import org.chromium.chrome.R;
import org.chromium.components.browser_ui.widget.RadioButtonWithDescription;
import org.chromium.components.browser_ui.widget.RadioButtonWithEditText;

/**
 * Brave's version of a radio button group Preference used for Homepage Preference.
 */
public final class BraveRadioButtonGroupHomepagePreference extends RadioButtonGroupHomepagePreference {
    static final String MOBILE_BOOKMARKS_PATH = "chrome-native://bookmarks/folder/1";

    private RadioButtonWithDescription mMobileBookmarks;
    private RadioButtonWithEditText mCustomUri;
    private boolean mSelectMobileBookmarks;

    public BraveRadioButtonGroupHomepagePreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * Called when the checked radio button has changed. When the selection is cleared, checkedId is
     * -1.
     *
     * @param group The group in which the checked radio button has changed
     * @param checkedId The unique identifier of the newly checked radio button
     */
    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId) {
        PreferenceValues preferenceValues = getPreferenceValue();
        if (preferenceValues == null) return;

        if (checkedId == R.id.brave_homepage_mobile_bookmarks) {
            maybeSelectMobileBookmarks(true);
        } else {
            super.onCheckedChanged(group, checkedId);
        }
        maybeClearCustomUri();
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);

        mMobileBookmarks = (RadioButtonWithDescription) holder.findViewById(R.id.brave_homepage_mobile_bookmarks);
        assert mMobileBookmarks != null : "Mobile bookmarks button is missing in the layout";

        mCustomUri = (RadioButtonWithEditText) holder.findViewById(R.id.radio_button_uri_edit);
        assert mCustomUri != null : "Custom URI button missing in the layout";

        maybeSelectMobileBookmarks(false);
        maybeClearCustomUri();
    }

    @Override
    void setupPreferenceValues(@NonNull PreferenceValues value) {
        super.setupPreferenceValues(value);

        // If it is custom uri with mobile bookmarks path, set the mobile bookmarks radio button
        // selected.
        mSelectMobileBookmarks =
                value.getCheckedOption() == HomepageOption.ENTRY_CUSTOM_URI
                        && value.getCustomURI().equals(MOBILE_BOOKMARKS_PATH);

        maybeSelectMobileBookmarks(false);
        maybeClearCustomUri();
    }

    void maybeSelectMobileBookmarks(boolean forceSelect) {
        if (mSelectMobileBookmarks || forceSelect) {
            if (mMobileBookmarks != null) {
                // Make sure it is selected.
                mMobileBookmarks.setChecked(true);
                // Set proper URL (mobile bookmarks is the same as custom uri just with mobile
                // bookmarks path).
                onTextChanged(MOBILE_BOOKMARKS_PATH);
            }
        }
    }

    private void maybeClearCustomUri() {
        // If the custom uri is not selected and the text is the mobile bookmarks path then clear
        // the text.
        if (mCustomUri != null
                && !mCustomUri.isChecked()
                && mCustomUri.getPrimaryText().toString().equals(MOBILE_BOOKMARKS_PATH)) {
            mCustomUri.setPrimaryText("");
        }
    }

    @VisibleForTesting
    RadioButtonWithDescription getMobileBookmarksRadioButton() {
        return mMobileBookmarks;
    }
}
