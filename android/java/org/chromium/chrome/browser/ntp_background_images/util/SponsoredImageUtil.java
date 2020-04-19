/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.chromium.chrome.browser.ntp_background_images.util;

import org.chromium.chrome.R;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import org.chromium.chrome.browser.ntp_background_images.model.BackgroundImage;
import org.chromium.chrome.browser.ntp_background_images.model.ImageCredit;

public class SponsoredImageUtil {

    public static final String NTP_TYPE = "ntp_type";

    public static final int BR_INVALID_OPTION = -1;
    public static final int BR_OFF = 1;
    public static final int BR_ON_ADS_OFF = 2;
    public static final int BR_ON_ADS_OFF_BG_IMAGE = 3;
    public static final int BR_ON_ADS_ON = 4;

    public static final int MAX_TABS = 10;

	private static List<BackgroundImage> backgroundImages = new ArrayList<BackgroundImage>(Arrays.asList(
            new BackgroundImage(R.drawable.anders_jilden, 1200, new ImageCredit("Anders Jildén", "https://unsplash.com/@andersjilden?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.andreas_gucklhorn, 1160, new ImageCredit("Andreas Gücklhorn", "https://unsplash.com/@draufsicht?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.andy_mai, 988, new ImageCredit("Andy Mai", "https://unsplash.com/@veroz?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.annie_spratt, 682, new ImageCredit("Annie Spratt", "https://unsplash.com/@anniespratt?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.anton_repponen, 2185, new ImageCredit("Anton Repponen", "https://unsplash.com/@repponen?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.ben_karpinski, 815, new ImageCredit("Ben Karpinski", "http://bklandscapes.com/")),
            new BackgroundImage(R.drawable.dc_cavalleri, 480, new ImageCredit("D.C. Cavalleri", "https://gradivis.com")),
            new BackgroundImage(R.drawable.joe_gardner, 1277, new ImageCredit("Joe Gardner", "https://unsplash.com/@josephgardnerphotography?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.louis_kim, 0, new ImageCredit("Louis Kim", "http://louiskimphotography.com/")),
            new BackgroundImage(R.drawable.matt_palmer, 1205, new ImageCredit("Matt Palmer", "https://unsplash.com/@mattpalmer?utm_source=unsplash&utm_medium=referral&utm_content=credit")),
            new BackgroundImage(R.drawable.oliwier_gesla, 780, new ImageCredit("Oliwier Gesla", "https://www.instagram.com/oliwiergesla/")),
            new BackgroundImage(R.drawable.svalbard_jerol_soibam, 1280, new ImageCredit("Jerol Soibam", "https://www.instagram.com/jerol_soibam/")),
            new BackgroundImage(R.drawable.will_christiansen_glacier_peak, 1630, new ImageCredit("Will Christiansen", "http://www.theskyfolk.com/")),
            new BackgroundImage(R.drawable.will_christiansen_ice, 1330, new ImageCredit("Will Christiansen", "http://www.theskyfolk.com/")),
            new BackgroundImage(R.drawable.xavier_balderas_cejudo, 1975, new ImageCredit("Xavier Balderas Cejudo", "https://unsplash.com/@xavibalderas?utm_source=unsplash&utm_medium=referral&utm_content=credit"))
    ));

	private static int backgroundImageIndex = getRandomIndex(backgroundImages.size());

	private static int tabIndex = 1;

    public static List<BackgroundImage> getBackgroundImages() {
        return backgroundImages;
    }

    public static int getTabIndex() {
        return tabIndex;
    }

    public static void incrementTabIndex(int count) {
        tabIndex = tabIndex + count;
    }

    private static int getRandomIndex(int count) {
    	Random rand = new Random();
    	return rand.nextInt(count);
    }

    public static BackgroundImage getBackgroundImage() {
    	if (backgroundImageIndex >= backgroundImages.size()) {
    		backgroundImageIndex = 0;
    	}

    	BackgroundImage backgroundImage = backgroundImages.get(backgroundImageIndex);
    	backgroundImageIndex++;
    	return backgroundImage;
    }
}
