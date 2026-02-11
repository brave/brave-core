// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

package org.chromium.chrome.browser.password_manager.settings;

import android.content.Context;
import android.graphics.drawable.BitmapDrawable;

import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.R;
import org.chromium.chrome.browser.profiles.Profile;
import org.chromium.chrome.browser.ui.favicon.FaviconHelper;
import org.chromium.url.GURL;

/**
 * A Preference that lazily loads a favicon when it becomes visible on screen. The favicon is
 * fetched only once when the view is first bound, and cached via {@link Preference#setIcon} for
 * subsequent rebinds.
 */
@NullMarked
public class PasswordEntryPreference extends Preference {
    private final GURL mOriginUrl;
    private final FaviconHelper mFaviconHelper;
    private final FaviconHelper.DefaultFaviconHelper mDefaultFaviconHelper;
    private final Profile mProfile;

    public PasswordEntryPreference(
            Context context,
            String originUrl,
            FaviconHelper faviconHelper,
            FaviconHelper.DefaultFaviconHelper defaultFaviconHelper,
            Profile profile) {
        super(context);
        mOriginUrl = new GURL(originUrl);
        mFaviconHelper = faviconHelper;
        mDefaultFaviconHelper = defaultFaviconHelper;
        mProfile = profile;
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        if (getIcon() == null) {
            int faviconSizePixels =
                    getContext()
                            .getResources()
                            .getDimensionPixelSize(R.dimen.omnibox_suggestion_favicon_size);
            mFaviconHelper.getForeignFaviconImageForURL(
                    mProfile,
                    mOriginUrl,
                    faviconSizePixels,
                    (bitmap, iconUrl) -> {
                        if (bitmap == null) {
                            bitmap =
                                    mDefaultFaviconHelper.getDefaultFaviconBitmap(
                                            getContext(), iconUrl, true, false);
                        }
                        setIcon(new BitmapDrawable(getContext().getResources(), bitmap));
                    });
        }
    }
}
