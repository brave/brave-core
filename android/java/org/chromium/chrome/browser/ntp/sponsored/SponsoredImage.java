/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp.sponsored;

public class SponsoredImage extends BackgroundImage {

    private long startDate;
    private long endDate;

    public SponsoredImage(int imageDrawable, int centerPoint, ImageCredit imageCredit, long startDate, long endDate) {
        super(imageDrawable, centerPoint, imageCredit);
        this.startDate = startDate;
        this.endDate = endDate;
    }

    public long getStartDate() {
        return startDate;
    }

    public long getEndDate() {
        return endDate;
    }
}