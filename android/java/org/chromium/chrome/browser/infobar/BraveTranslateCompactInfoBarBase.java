// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.infobar;

import android.graphics.Bitmap;

import androidx.annotation.ColorRes;

import org.chromium.chrome.R;
import org.chromium.components.infobars.InfoBar;

public class BraveTranslateCompactInfoBarBase extends InfoBar {
    BraveTranslateCompactInfoBarBase(
            int iconDrawableId, @ColorRes int iconTintId, CharSequence message, Bitmap iconBitmap) {
        super(R.drawable.ic_translate, 0, null, null);
    }
}