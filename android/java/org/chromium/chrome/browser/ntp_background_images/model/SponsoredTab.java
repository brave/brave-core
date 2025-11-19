/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.model;

import org.chromium.chrome.browser.ntp_background_images.NTPBackgroundImagesBridge;
import org.chromium.chrome.browser.ntp_background_images.util.NTPImageUtil;
import org.chromium.chrome.browser.ntp_background_images.util.SponsoredImageUtil;

public class SponsoredTab {
    private final NTPBackgroundImagesBridge mNTPBackgroundImagesBridge;
    private NTPImage mNtpImage;
    private int mTabIndex;

    public SponsoredTab(
            NTPBackgroundImagesBridge mNTPBackgroundImagesBridge, boolean allowSponsoredImage) {
        this.mNTPBackgroundImagesBridge = mNTPBackgroundImagesBridge;
        if (NTPImageUtil.shouldEnableNTPFeature()) {
            mNtpImage = NTPImageUtil.getNTPImage(mNTPBackgroundImagesBridge, allowSponsoredImage);
            mTabIndex = SponsoredImageUtil.getTabIndex();
        }
    }

    public NTPImage getTabNTPImage(boolean isReset, boolean allowSponsoredImage) {
        if (mNtpImage == null || isReset) {
            mNtpImage = NTPImageUtil.getNTPImage(mNTPBackgroundImagesBridge, allowSponsoredImage);
        }
        return mNtpImage;
    }

    public void setNTPImage(NTPImage ntpImage) {
        this.mNtpImage = ntpImage;
    }

    public int getTabIndex() {
        return mTabIndex;
    }

    public void setTanIndex(int tabIndex) {
        this.mTabIndex = tabIndex;
    }
}
