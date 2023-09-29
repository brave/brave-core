/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import java.util.Arrays;
import java.util.List;

public final class BravePreferenceKeys {
    public static final String BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY =
            "brave_bottom_toolbar_enabled_key";
    public static final String BRAVE_BOTTOM_TOOLBAR_SET_KEY = "brave_bottom_toolbar_enabled";
    public static final String BRAVE_USE_CUSTOM_TABS = "use_custom_tabs";
    public static final String BRAVE_APP_OPEN_COUNT = "brave_app_open_count";
    public static final String BRAVE_ROLE_MANAGER_DIALOG_COUNT = "brave_role_manager_dialog_count";
    public static final String BRAVE_IS_DEFAULT = "brave_is_default";
    public static final String BRAVE_WAS_DEFAULT_ASK_COUNT = "brave_was_default_ask_count";
    public static final String BRAVE_SET_DEFAULT_BOTTOM_SHEET_COUNT =
            "brave_set_default_bottom_sheet_count";
    public static final String BRAVE_DEFAULT_DONT_ASK = "brave_default_dont_ask";
    public static final String BRAVE_UPDATE_EXTRA_PARAM =
            "org.chromium.chrome.browser.upgrade.UPDATE_NOTIFICATION_NEW";
    public static final String BRAVE_NOTIFICATION_PREF_NAME =
            "org.chromium.chrome.browser.upgrade.NotificationUpdateTimeStampPreferences_New";
    public static final String BRAVE_MILLISECONDS_NAME =
            "org.chromium.chrome.browser.upgrade.Milliseconds_New";
    public static final String BRAVE_DOWNLOADS_AUTOMATICALLY_OPEN_WHEN_POSSIBLE =
            "org.chromium.chrome.browser.downloads.Automatically_Open_When_Possible";
    public static final String BRAVE_DOWNLOADS_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE =
            "org.chromium.chrome.browser.downloads.Download_Progress_Notification_Bubble";
    public static final String BRAVE_DOUBLE_RESTART =
            "org.chromium.chrome.browser.Brave_Double_Restart";
    public static final String BRAVE_TAB_GROUPS_ENABLED =
            "org.chromium.chrome.browser.Brave_Tab_Groups_Enabled";
    public static final String BRAVE_DISABLE_SHARING_HUB =
            "org.chromium.chrome.browser.Brave_Disable_Sharing_Hub";
    public static final String BRAVE_NEWS_CARDS_VISITED = "brave_news_cards_visited";
    public static final String BRAVE_NEWS_CHANGE_SOURCE = "brave_news_change_source";
    public static final String BRAVE_NEWS_FEED_HASH = "brave_news_feed_hash";
    public static final String BRAVE_NEWS_PREF_SHOW_NEWS = "kNewTabPageShowToday";
    public static final String BRAVE_NEWS_PREF_TURN_ON_NEWS = "kBraveNewsOptedIn";
    public static final String BRAVE_USE_BIOMETRICS_FOR_WALLET =
            "org.chromium.chrome.browser.Brave_Use_Biometrics_For_Wallet";
    public static final String BRAVE_BIOMETRICS_FOR_WALLET_IV =
            "org.chromium.chrome.browser.Brave_Biometrics_For_Wallet_Iv";
    public static final String BRAVE_BIOMETRICS_FOR_WALLET_ENCRYPTED =
            "org.chromium.chrome.browser.Brave_Biometrics_For_Wallet_Encrypted";
    public static final String BRAVE_AD_FREE_CALLOUT_DIALOG = "brave_ad_free_callout_dialog";
    public static final String BRAVE_OPENED_YOUTUBE = "brave_opened_youtube";
    public static final String SHOULD_SHOW_COOKIE_CONSENT_NOTICE =
            "should_show_cookie_consent_notice";
    public static final String LOADED_SITE_COUNT = "loaded_site_count";
    public static final String BRAVE_BACKGROUND_VIDEO_PLAYBACK_CONVERTED_TO_FEATURE =
            "brave_background_video_playback_converted_to_feature";
    public static final String BRAVE_APP_OPEN_COUNT_FOR_WIDGET_PROMO =
            "brave_app_open_count_for_widget_promo";
    public static final String BRAVE_DEFERRED_DEEPLINK_PLAYLIST =
            "brave_deferred_deeplink_playlist";
    public static final String BRAVE_DEFERRED_DEEPLINK_VPN = "brave_deferred_deeplink_vpn";
    public static final String BRAVE_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    public static final String BRAVE_CLEAR_ON_EXIT = "clear_on_exit";
    public static final String BRAVE_QUICK_ACTION_SEARCH_AND_BOOKMARK_WIDGET_TILES =
            "org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider.TILES";

    // These are dynamic keys
    public static final String BRAVE_RECYCLERVIEW_POSITION = "recyclerview_visible_position_";
    public static final String BRAVE_RECYCLERVIEW_OFFSET_POSITION = "recyclerview_offset_position_";

    /*
     * Returns the list of Brave's preference keys.
     */
    private static List<String> getKeysInUse() {
        return Arrays.asList(BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY, BRAVE_BOTTOM_TOOLBAR_SET_KEY,
                BRAVE_USE_CUSTOM_TABS, BRAVE_APP_OPEN_COUNT, BRAVE_ROLE_MANAGER_DIALOG_COUNT,
                BRAVE_IS_DEFAULT, BRAVE_WAS_DEFAULT_ASK_COUNT, BRAVE_SET_DEFAULT_BOTTOM_SHEET_COUNT,
                BRAVE_DEFAULT_DONT_ASK, BRAVE_UPDATE_EXTRA_PARAM, BRAVE_NOTIFICATION_PREF_NAME,
                BRAVE_MILLISECONDS_NAME, BRAVE_DOWNLOADS_AUTOMATICALLY_OPEN_WHEN_POSSIBLE,
                BRAVE_DOWNLOADS_DOWNLOAD_PROGRESS_NOTIFICATION_BUBBLE, BRAVE_DOUBLE_RESTART,
                BRAVE_TAB_GROUPS_ENABLED, BRAVE_DISABLE_SHARING_HUB, BRAVE_NEWS_CARDS_VISITED,
                BRAVE_NEWS_CHANGE_SOURCE, BRAVE_NEWS_FEED_HASH, BRAVE_NEWS_PREF_SHOW_NEWS,
                BRAVE_NEWS_PREF_TURN_ON_NEWS, BRAVE_USE_BIOMETRICS_FOR_WALLET,
                BRAVE_BIOMETRICS_FOR_WALLET_IV, BRAVE_BIOMETRICS_FOR_WALLET_ENCRYPTED,
                BRAVE_AD_FREE_CALLOUT_DIALOG, BRAVE_OPENED_YOUTUBE,
                SHOULD_SHOW_COOKIE_CONSENT_NOTICE, LOADED_SITE_COUNT,
                BRAVE_BACKGROUND_VIDEO_PLAYBACK_CONVERTED_TO_FEATURE,
                BRAVE_APP_OPEN_COUNT_FOR_WIDGET_PROMO, BRAVE_DEFERRED_DEEPLINK_PLAYLIST,
                BRAVE_DEFERRED_DEEPLINK_VPN, BRAVE_CLOSE_TABS_ON_EXIT, BRAVE_CLEAR_ON_EXIT,
                BRAVE_QUICK_ACTION_SEARCH_AND_BOOKMARK_WIDGET_TILES);
    }

    /*
     * Checks if preference key is among Brave's dynamic keys.
     */
    private static boolean isBraveDynamicKeyInUse(String key) {
        return key.startsWith(BRAVE_RECYCLERVIEW_POSITION)
                || key.startsWith(BRAVE_RECYCLERVIEW_OFFSET_POSITION);
    }

    /*
     * Checks if preference key is used in Brave.
     */
    public static boolean isBraveKeyInUse(String key) {
        return getKeysInUse().contains(key) || isBraveDynamicKeyInUse(key);
    }
}
