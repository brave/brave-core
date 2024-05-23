/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.NTPUtil;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;

public class SponsoredTab {
    private NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private NTPImage ntpImage;
    private int tabIndex;
    private boolean mShouldShowBanner;

    public SponsoredTab(NTPBackgroundImagesBridge mNTPBackgroundImagesBridge) {
        this.mNTPBackgroundImagesBridge = mNTPBackgroundImagesBridge;
        if (NTPUtil.shouldEnableNTPFeature()) {
            ntpImage = NTPUtil.getNTPImage(mNTPBackgroundImagesBridge);
            tabIndex = SponsoredImageUtil.getTabIndex();
        }
    }

    public NTPImage getTabNTPImage(boolean isReset) {
        if (ntpImage == null || isReset) {
            ntpImage = NTPUtil.getNTPImage(mNTPBackgroundImagesBridge);
        }
        return ntpImage;
    }

    public void setNTPImage(NTPImage ntpImage) {
        this.ntpImage = ntpImage;
    }

    public int getTabIndex() {
        return tabIndex;
    }

    public void setTanIndex(int tabIndex) {
        this.tabIndex = tabIndex;
    }
}
