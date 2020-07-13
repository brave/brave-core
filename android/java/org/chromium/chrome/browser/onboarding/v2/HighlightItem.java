/**
 * Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.onboarding.v2;

public class HighlightItem {
    private int screenLeft;
    private int screenTop;
    private int screenRight;
    private int screenBottom;

    public void setScreenPosition(int screenLeft, int screenTop, int screenRight, int screenBottom) {
        this.screenLeft = screenLeft;
        this.screenTop = screenTop;
        this.screenRight = screenRight;
        this.screenBottom = screenBottom;
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