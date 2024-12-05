/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

public final class BravePreferenceKeys {
    public static final String BRAVE_BOTTOM_TOOLBAR_ENABLED_KEY =
            "brave_bottom_toolbar_enabled_key";
    public static final String BRAVE_BOTTOM_TOOLBAR_SET_KEY = "brave_bottom_toolbar_enabled";
    public static final String BRAVE_IS_MENU_FROM_BOTTOM = "brave_is_menu_from_bottom";
    public static final String BRAVE_USE_CUSTOM_TABS = "use_custom_tabs";
    public static final String BRAVE_APP_OPEN_COUNT = "brave_app_open_count";
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
    public static final String BRAVE_TAB_GROUPS_ENABLED_DEFAULT_VALUE =
            "org.chromium.chrome.browser.Brave_Tab_Groups_Enabled_Default_Value";
    public static final String BRAVE_DISABLE_SHARING_HUB =
            "org.chromium.chrome.browser.Brave_Disable_Sharing_Hub";
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
    public static final String BRAVE_BACKGROUND_VIDEO_PLAYBACK_CONVERTED_TO_FEATURE =
            "brave_background_video_playback_converted_to_feature";
    public static final String BRAVE_DEFERRED_DEEPLINK_PLAYLIST =
            "brave_deferred_deeplink_playlist";
    public static final String BRAVE_DEFERRED_DEEPLINK_VPN = "brave_deferred_deeplink_vpn";
    public static final String BRAVE_CLOSE_TABS_ON_EXIT = "close_tabs_on_exit";
    public static final String BRAVE_CLEAR_ON_EXIT = "clear_on_exit";
    public static final String BRAVE_QUICK_ACTION_SEARCH_AND_BOOKMARK_WIDGET_TILES =
            "org.chromium.chrome.browser.widget.quickactionsearchandbookmark.QuickActionSearchAndBookmarkWidgetProvider.TILES";
    public static final String ENABLE_MULTI_WINDOWS = "enable_multi_windows";
    public static final String ENABLE_MULTI_WINDOWS_UPGRADE = "enable_multi_windows_upgrade";

    public static final String BRAVE_LEO_AUTOCOMPLETE =
            "org.chromium.chrome.browser.Brave_Leo_AutoComplete";

    // Playlist preference keys
    public static final String PREF_ENABLE_PLAYLIST = "enable_playlist";
    public static final String PREF_ADD_TO_PLAYLIST_BUTTON = "add_to_playlist_button";
    public static final String PREF_REMEMBER_FILE_PLAYBACK_POSITION =
            "remember_file_playback_position";
    public static final String PREF_REMEMBER_LIST_PLAYBACK_POSITION =
            "remember_list_playback_position";
    public static final String PREF_CONTINUOUS_LISTENING = "continuous_listening";
    public static final String PREF_RESET_PLAYLIST = "reset_playlist";

    // These are dynamic keys
    public static final String BRAVE_RECYCLERVIEW_POSITION = "recyclerview_visible_position_";
    public static final String BRAVE_RECYCLERVIEW_OFFSET_POSITION = "recyclerview_offset_position_";

    public static final String BRAVE_IN_APP_UPDATE_TIMING = "in_app_update_timing";

    public static final String DAY_ZERO_EXPT_FLAG = "day_zero_expt_flag";

    public static final String SHOW_UNDO_WHEN_TABS_CLOSED = "show_undo_when_tabs_closed";

    public static final String OPEN_YT_IN_BRAVE_DIALOG = "open_yt_in_brave_dialog";

    public static final String BRAVE_QUICK_SEARCH_ENGINES = "quick_search_engines";
    public static final String BRAVE_QUICK_SEARCH_ENGINES_FEATURE = "quick_search_engines_feature";
    public static final String BRAVE_QUICK_SEARCH_ENGINES_PREVIOUS_DSE =
            "quick_search_engines_previous_dse";

    /*
     * Checks if preference key is used in Brave.
     * It's no op currently. We might reconsider
     * using it in the future for keys sanitation
     */
    public static boolean isBraveKeyInUse(String key) {
        return true;
    }
}
