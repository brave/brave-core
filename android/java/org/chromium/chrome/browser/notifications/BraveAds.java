/** Copyright (c) 2019 The Brave Authors. All rights reserved.
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this file,
  * You can obtain one at http://mozilla.org/MPL/2.0/.
  */

package org.chromium.chrome.browser.notifications;

import org.chromium.base.annotations.CalledByNative;

/**
 * This class provides the Brave Ads related methods for the native library
 * (brave/components/brave_ads/browser/notification_helper_android)
 */
public abstract class BraveAds {
    // TODO(jocelyn): Move this into a separate class for handling Brave's
    // notification channels.
    private static final String BRAVE_ADS_CHANNEL_ID = "com.brave.browser.ads";

    @CalledByNative
    public static String getBraveAdsChannelId() {
        return BRAVE_ADS_CHANNEL_ID;
    }
}
