/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.components.infobars;

import android.graphics.Bitmap;
import android.widget.TextView;

import androidx.annotation.ColorRes;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

@NullMarked
public class BraveConfirmInfoBar extends ConfirmInfoBar {
    protected BraveConfirmInfoBar(
            int iconDrawableId,
            @ColorRes int iconTintId,
            @Nullable Bitmap iconBitmap,
            String message,
            @Nullable String linkText,
            String primaryButtonText,
            @Nullable String secondaryButtonText) {
        super(
                iconDrawableId,
                iconTintId,
                iconBitmap,
                message,
                linkText,
                primaryButtonText,
                secondaryButtonText);
    }

    @Override
    public void createContent(InfoBarLayout layout) {
        super.createContent(layout);
        if ((int) getInfoBarIdentifier()
                == BraveInfoBarIdentifier.NEW_TAB_TAKEOVER_INFOBAR_DELEGATE_EDGE_TO_EDGE) {
            // Adding extra padding at the bottom when edge to edge
            // is active.
            // See https://github.com/brave/brave-browser/issues/46513
            // for more info.
            TextView messageTexView = layout.getMessageTextView();
            messageTexView.setPadding(
                    messageTexView.getPaddingLeft(),
                    messageTexView.getPaddingTop(),
                    messageTexView.getPaddingRight(),
                    messageTexView.getPaddingBottom()
                            + messageTexView
                                    .getResources()
                                    .getDimensionPixelOffset(R.dimen.infobar_padding));
        }
    }
}
