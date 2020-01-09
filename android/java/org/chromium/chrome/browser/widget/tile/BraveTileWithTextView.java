// Copyright 2019 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.widget.tile;

import android.content.Context;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import android.widget.TextView;
import android.content.SharedPreferences;

import org.chromium.chrome.R;
import org.chromium.base.ContextUtils;
import org.chromium.chrome.browser.preferences.BackgroundImagesPreferences;

public class BraveTileWithTextView extends TileWithTextView {
	public BraveTileWithTextView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public void setTitle(String title, int titleLines) {

    	super.setTitle(title, titleLines);

    	SharedPreferences mSharedPreferences = ContextUtils.getAppSharedPreferences();
        if(mSharedPreferences.getBoolean(BackgroundImagesPreferences.PREF_SHOW_BACKGROUND_IMAGES, true)) {
            mTitleView.setTextColor(getResources().getColor(android.R.color.white));
        }
    }
}