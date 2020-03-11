/**
 * Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.notifications.channels;

import android.app.NotificationManager;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.notifications.channels.ChannelDefinitions;

import java.util.Map;
import java.util.Set;

public class BraveChannelDefinitions {
    public class ChannelId {
        public static final String BRAVE_ADS = "com.brave.browser.ads";
        public static final String BRAVE_ADS_BACKGROUND = "com.brave.browser.ads.background";
	}

    public class ChannelGroupId { public static final String BRAVE_ADS = "com.brave.browser.ads"; }

    static protected void addBraveChannels(
            Map<String, ChannelDefinitions.PredefinedChannel> map, Set<String> startup) {
        map.put(ChannelId.BRAVE_ADS,
                new ChannelDefinitions.PredefinedChannel(ChannelId.BRAVE_ADS,
                        R.string.notification_category_brave_ads,
                        NotificationManager.IMPORTANCE_HIGH, ChannelGroupId.BRAVE_ADS));
        startup.add(ChannelId.BRAVE_ADS);

        map.put(ChannelId.BRAVE_ADS_BACKGROUND,
                new ChannelDefinitions.PredefinedChannel(ChannelId.BRAVE_ADS_BACKGROUND,
                       R.string.notification_category_brave_ads_background,
                       NotificationManager.IMPORTANCE_LOW, ChannelGroupId.BRAVE_ADS));
        startup.add(ChannelId.BRAVE_ADS_BACKGROUND);
    }

    static protected void addBraveChannelGroups(
            Map<String, ChannelDefinitions.PredefinedChannelGroup> map) {
        map.put(ChannelGroupId.BRAVE_ADS,
                new ChannelDefinitions.PredefinedChannelGroup(
                        ChannelGroupId.BRAVE_ADS, R.string.notification_category_group_brave_ads));
    }
}
