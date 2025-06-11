/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.content.res.ColorStateList;
import android.util.AttributeSet;
import android.widget.ImageButton;

import androidx.core.widget.ImageViewCompat;

import org.chromium.chrome.browser.omnibox.status.StatusView;

public class BraveLocationBarLayout extends LocationBarLayout {
    private final ImageButton mQRButton;

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

        StatusView statusView = findViewById(R.id.location_bar_status);
        statusView.setBackgroundDrawable(null);
    }

    void setQRButtonTint(ColorStateList colorStateList) {
        ImageViewCompat.setImageTintList(mQRButton, colorStateList);
    }

    void setQRButtonVisibility(boolean shouldShow) {
        mQRButton.setVisibility(shouldShow ? VISIBLE : GONE);
    }
}
