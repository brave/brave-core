/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

package org.chromium.base;

import org.chromium.build.annotations.NullMarked;

/** A list of feature flags exposed to Java. */
@NullMarked
public abstract class BraveFeatureList {
    public static final String NATIVE_BRAVE_WALLET = "NativeBraveWallet";
    public static final String USE_DEV_UPDATER_URL = "UseDevUpdaterUrl";
    public static final String FORCE_WEB_CONTENTS_DARK_MODE = "WebContentsForceDark";
    public static final String ENABLE_FORCE_DARK = "enable-force-dark";
    public static final String ENABLE_PARALLEL_DOWNLOADING = "enable-parallel-downloading";
    public static final String BRAVE_SEARCH_OMNIBOX_BANNER = "BraveSearchOmniboxBanner";
    public static final String BRAVE_BACKGROUND_VIDEO_PLAYBACK = "BraveBackgroundVideoPlayback";
    public static final String BRAVE_PICTURE_IN_PICTURE_FOR_YOUTUBE_VIDEOS =
            "BravePictureInPictureForYouTubeVideos";
    public static final String BRAVE_ANDROID_SAFE_BROWSING = "BraveAndroidSafeBrowsing";
    public static final String BRAVE_VPN_LINK_SUBSCRIPTION_ANDROID_UI =
            "BraveVPNLinkSubscriptionAndroidUI";
    public static final String DEBOUNCE = "BraveDebounce";
    public static final String BRAVE_GOOGLE_SIGN_IN_PERMISSION = "BraveGoogleSignInPermission";
    public static final String BRAVE_LOCALHOST_PERMISSION = "BraveLocalhostPermission";
    public static final String BRAVE_PLAYLIST = "Playlist";
    public static final String HTTPS_BY_DEFAULT = "HttpsByDefault";
    public static final String BRAVE_FORGET_FIRST_PARTY_STORAGE = "BraveForgetFirstPartyStorage";
    public static final String BRAVE_REQUEST_OTR_TAB = "BraveRequestOTRTab";
    public static final String AI_CHAT = "AIChat";
    public static final String AI_CHAT_HISTORY = "AIChatHistory";
    public static final String BRAVE_SHOW_STRICT_FINGERPRINTING_MODE =
            "BraveShowStrictFingerprintingMode";
    public static final String BRAVE_DAY_ZERO_EXPERIMENT = "BraveDayZeroExperiment";
    public static final String BRAVE_NEW_ANDROID_ONBOARDING = "NewAndroidOnboarding";
    public static final String BRAVE_FRESH_NTP_AFTER_IDLE_EXPIREMENT =
            "BraveFreshNtpAfterIdleExpirement";
    public static final String BRAVE_FALLBACK_DOH_PROVIDER = "BraveFallbackDoHProvider";
    public static final String BRAVE_BLOCK_ALL_COOKIES_TOGGLE = "BlockAllCookiesToggle";
    public static final String BRAVE_SHIELDS_ELEMENT_PICKER = "BraveShieldsElementPicker";
    public static final String BRAVE_WEB_DISCOVERY_NATIVE = "BraveWebDiscoveryNative";
    public static final String BRAVE_INCOGNITO_SCREENSHOT = "incognito-screenshot";
    public static final String BRAVE_NTP_BRANDED_WALLPAPER_SURVEY_PANELIST =
            "BraveNTPBrandedWallpaperSurveyPanelist";
    public static final String BRAVE_ACCOUNT = "BraveAccount";
    public static final String BRAVE_ANDROID_DYNAMIC_COLORS = "BraveAndroidDynamicColors";
    public static final String BRAVE_ORIGIN = "BraveOrigin";
}
