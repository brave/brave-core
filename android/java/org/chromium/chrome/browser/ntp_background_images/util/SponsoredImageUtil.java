/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.util;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.ImageCredit;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

public class SponsoredImageUtil {

    public static final String NTP_TYPE = "ntp_type";

    public static final int BR_INVALID_OPTION = -1;
    public static final int BR_OFF = 1;
    public static final int BR_ON_ADS_OFF = 2;
    public static final int BR_ON_ADS_OFF_BG_IMAGE = 3;
    public static final int BR_ON_ADS_ON = 4;

    public static final int MAX_TABS = 10;

    private static final List<BackgroundImage> sBackgroundImages =
            new ArrayList<BackgroundImage>(
                    Arrays.asList(
                            new BackgroundImage(
                                    R.drawable.dylan_malval_sea_min,
                                    1300,
                                    720,
                                    new ImageCredit(
                                            "Dylan Malval",
                                            "https://www.instagram.com/vass_captures/"))));

    private static int mBackgroundImageIndex = getRandomIndex(sBackgroundImages.size());

    private static int mTabIndex = 1;

    public static List<BackgroundImage> getBackgroundImages() {
        return sBackgroundImages;
    }

    public static int getTabIndex() {
        return mTabIndex;
    }

    public static void incrementTabIndex(int count) {
        mTabIndex = mTabIndex + count;
    }

    private static int getRandomIndex(int count) {
    	Random rand = new Random();
    	return rand.nextInt(count);
    }

    public static BackgroundImage getBackgroundImage() {
        if (mBackgroundImageIndex >= sBackgroundImages.size()) {
            mBackgroundImageIndex = 0;
        }

        BackgroundImage backgroundImage = sBackgroundImages.get(mBackgroundImageIndex);
        mBackgroundImageIndex++;
        return backgroundImage;
    }
}
