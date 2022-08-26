/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox;

import android.content.Context;
import android.content.res.ColorStateList;
import android.util.AttributeSet;
import android.widget.ImageButton;

import org.chromium.base.ApiCompatibilityUtils;

public class BraveLocationBarLayout extends LocationBarLayout {
    private ImageButton mQRButton;

    public BraveLocationBarLayout(Context context, AttributeSet attrs) {
        super(context, attrs);
        mQRButton = findViewById(R.id.qr_button);
    }

    public BraveLocationBarLayout(Context context, AttributeSet attrs, int layoutId) {
        super(context, attrs, layoutId);
        mQRButton = findViewById(R.id.qr_button);
    }

    void setQRButtonTint(ColorStateList colorStateList) {
        ApiCompatibilityUtils.setImageTintList(mQRButton, colorStateList);
    }

    void setQRButtonVisibility(boolean shouldShow) {
        mQRButton.setVisibility(shouldShow ? VISIBLE : GONE);
    }
}
