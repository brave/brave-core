/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.omnibox.status;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.ImageView;

import org.chromium.build.annotations.NullMarked;
import org.chromium.chrome.browser.omnibox.R;
import org.chromium.components.browser_ui.widget.RoundedCornerOutlineProvider;

@NullMarked
public class BraveStatusView extends StatusView {
    public BraveStatusView(Context context, AttributeSet attributes) {
        super(context, attributes);
    }

    @Override
    protected void onFinishInflate() {
        super.onFinishInflate();
        // Upstream creates an outline provider but never attaches it to mIconView
        // (see https://crrev.com/c/7738777), so setClipToOutline(true) clips against
        // the default rectangular outline and the icon renders as a square.
        ImageView iconView = findViewById(R.id.location_bar_status_icon);
        int radius =
                getResources()
                        .getDimensionPixelSize(
                                R.dimen.omnibox_search_engine_logo_composed_half_size);
        iconView.setOutlineProvider(new RoundedCornerOutlineProvider(radius));
    }
}
