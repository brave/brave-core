/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

import android.view.View;

public class HighlightItem {
    private int screenLeft;
    private int screenTop;
    private int screenRight;
    private int screenBottom;

    public HighlightItem(View highlightView) {
        int[] location = new int[2];
        highlightView.getLocationOnScreen(location);
        screenLeft = location[0];
        screenTop = location[1];
        screenRight = location[0] + highlightView.getMeasuredWidth();
        screenBottom = location[1] + highlightView.getMeasuredHeight();
    }

    public int getScreenLeft() {
        return screenLeft;
    }

    public int getScreenTop() {
        return screenTop;
    }

    public int getScreenRight() {
        return screenRight;
    }

    public int getScreenBottom() {
        return screenBottom;
    }
}