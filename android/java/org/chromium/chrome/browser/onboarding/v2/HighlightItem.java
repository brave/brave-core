/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.view.View;

public class HighlightItem {
    private final int mScreenLeft;
    private final int mScreenTop;
    private final int mScreenRight;
    private final int mScreenBottom;

    public HighlightItem(View highlightView) {
        int[] location = new int[2];
        highlightView.getLocationOnScreen(location);
        mScreenLeft = location[0];
        mScreenTop = location[1];
        mScreenRight = location[0] + highlightView.getMeasuredWidth();
        mScreenBottom = location[1] + highlightView.getMeasuredHeight();
    }

    public int getScreenLeft() {
        return mScreenLeft;
    }

    public int getScreenTop() {
        return mScreenTop;
    }

    public int getScreenRight() {
        return mScreenRight;
    }

    public int getScreenBottom() {
        return mScreenBottom;
    }
}
