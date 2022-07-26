/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.infobar;

import android.graphics.Bitmap;

import androidx.annotation.ColorRes;

import org.chromium.chrome.R;
import org.chromium.components.infobars.InfoBar;

public class BraveTranslateCompactInfoBarBase extends InfoBar {
    BraveTranslateCompactInfoBarBase(
            int iconDrawableId, @ColorRes int iconTintId, CharSequence message, Bitmap iconBitmap) {
        super(iconDrawableId == R.drawable.infobar_translate_compact ? R.drawable.ic_translate
                                                                     : iconDrawableId,
                0, null, null);
    }
}
