/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_webui_source.h"

#include <map>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "brave/common/url_constants.h"
#include "brave/components/crypto_dot_com/browser/buildflags/buildflags.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/grit/components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/resources/grit/webui_resources_map.h"

#if !defined(OS_ANDROID)
#include "brave/browser/ui/webui/navigation_bar_data_provider.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/grit/chromium_strings.h"
#endif

namespace {

struct WebUISimpleItem {
  const char* name;
  int id;
};

void AddLocalizedStringsBulk(content::WebUIDataSource* html_source,
                             const std::vector<WebUISimpleItem>& simple_items) {
  for (size_t i = 0; i < simple_items.size(); i++) {
    html_source->AddLocalizedString(simple_items[i].name,
                                    simple_items[i].id);
  }
}

void AddResourcePaths(content::WebUIDataSource* html_source,
                      const std::vector<WebUISimpleItem>& simple_items) {
  for (size_t i = 0; i < simple_items.size(); i++) {
    html_source->AddResourcePath(simple_items[i].name,
                                 simple_items[i].id);
  }
}

void CustomizeWebUIHTMLSource(const std::string &name,
    content::WebUIDataSource* source) {
#if !defined(OS_ANDROID)
  if (name == "rewards" || name == "wallet") {
    NavigationBarDataProvider::Initialize(source);
  }
#endif

  // clang-format off
  static std::map<std::string, std::vector<WebUISimpleItem> > resources = {
    {
      std::string("newtab"), {
        { "img/toolbar/menu_btn.svg", IDR_BRAVE_COMMON_TOOLBAR_IMG },
        // Hash path is the MD5 of the file contents,
        // webpack image loader does this
        { "fd85070af5114d6ac462c466e78448e4.svg", IDR_BRAVE_NEW_TAB_IMG1 },
        { "314e7529efec41c8867019815f4d8dad.svg", IDR_BRAVE_NEW_TAB_IMG4 },
        { "6c337c63662ee0ba4e57f6f8156d69ce.svg", IDR_BRAVE_NEW_TAB_IMG2 },
        // New tab Backgrounds
#if !defined(OS_ANDROID)
        { "alex-plesovskich.avif", IDR_BRAVE_NEW_TAB_BACKGROUND1 },
        { "andre-benz.avif", IDR_BRAVE_NEW_TAB_BACKGROUND2 },
        { "corwin-prescott_olympic.avif", IDR_BRAVE_NEW_TAB_BACKGROUND3 },
        { "dylan-malval_alps.avif", IDR_BRAVE_NEW_TAB_BACKGROUND4 },
        { "sora-sagano.avif", IDR_BRAVE_NEW_TAB_BACKGROUND5 },
        { "spencer-moore_lake.avif", IDR_BRAVE_NEW_TAB_BACKGROUND6 },
        { "su-san-lee.avif", IDR_BRAVE_NEW_TAB_BACKGROUND7 },
        { "zane-lee.avif", IDR_BRAVE_NEW_TAB_BACKGROUND8 },
#endif
        // private tab
        { "c168145d6bf1abf2c0322636366f7dbe.svg", IDR_BRAVE_PRIVATE_TAB_TOR_IMG },               // NOLINT
        { "dbdc336ccc651b8a7c925b3482d6e65a.svg", IDR_BRAVE_PRIVATE_TAB_IMG }
    }
#if !defined(OS_ANDROID)
    }, {
      std::string("rewards"), {
        { "favicon.ico", IDR_BRAVE_REWARDS_FAVICON },
        { "6dd79d472f9c73429b26dae4ef14575e.svg", IDR_BRAVE_REWARDS_IMG_WALLET_BG },             // NOLINT
        { "c9255cc2aa3d81ca6328e82d25a95766.png", IDR_BRAVE_REWARDS_IMG_CAPTCHA_BAT },           // NOLINT
        { "1bb9aa85741c6d1c077f043324aae835.svg", IDR_BRAVE_REWARDS_IMG_WELCOME_BG },            // NOLINT
        { "dcaf489409ca7908aef96547c9aad274.svg", IDR_BRAVE_REWARDS_IMG_TAP },                   // NOLINT
      }
#endif
    }, {
      std::string("welcome"), {
        { "favicon.ico", IDR_BRAVE_WELCOME_PAGE_FAVICON }
      }
    }, {
      std::string("adblock"), {}
    }
  };
  AddResourcePaths(source, resources[name]);

  // clang-format off
  static std::map<std::string, std::vector<WebUISimpleItem> >
                                                           localized_strings = {
    {
      std::string("newtab"), {
        { "adsTrackersBlocked", IDS_BRAVE_NEW_TAB_TOTAL_ADS_TRACKERS_BLOCKED },
        { "httpsUpgraded", IDS_BRAVE_NEW_TAB_TOTAL_HTTPS_UPGRADES },
        { "estimatedTimeSaved", IDS_BRAVE_NEW_TAB_TOTAL_TIME_SAVED },
        { "estimatedBandwidthSaved",
            IDS_BRAVE_NEW_TAB_ESTIMATED_BANDWIDTH_SAVED },
        { "thumbRemoved", IDS_BRAVE_NEW_TAB_THUMB_REMOVED },
        { "undoRemoved", IDS_BRAVE_NEW_TAB_UNDO_REMOVED },
        { "close", IDS_BRAVE_NEW_TAB_CLOSE },
        { "restoreAll", IDS_BRAVE_NEW_TAB_RESTORE_ALL },
        { "second", IDS_BRAVE_NEW_TAB_SECOND },
        { "seconds", IDS_BRAVE_NEW_TAB_SECONDS },
        { "minute", IDS_BRAVE_NEW_TAB_MINUTE },
        { "minutes", IDS_BRAVE_NEW_TAB_MINUTES },
        { "hour", IDS_BRAVE_NEW_TAB_HOUR },
        { "hours", IDS_BRAVE_NEW_TAB_HOURS },
        { "day", IDS_BRAVE_NEW_TAB_DAY },
        { "days", IDS_BRAVE_NEW_TAB_DAYS },
        { "B", IDS_BRAVE_NEW_TAB_BYTES },
        { "KB", IDS_BRAVE_NEW_TAB_KILOBYTES },
        { "MB", IDS_BRAVE_NEW_TAB_MEGABYTES },
        { "GB", IDS_BRAVE_NEW_TAB_GIGABYTES },
        { "photoBy", IDS_BRAVE_NEW_TAB_PHOTO_BY },
        { "hide", IDS_BRAVE_NEW_TAB_HIDE },
        { "preferencesPageTitle", IDS_BRAVE_NEW_TAB_PREFERENCES_PAGE_TITLE },
        { "bookmarksPageTitle", IDS_BRAVE_NEW_TAB_BOOKMARKS_PAGE_TITLE },
        { "historyPageTitle", IDS_BRAVE_NEW_TAB_HISTORY_PAGE_TITLE },
        { "dashboardSettingsTitle",
            IDS_BRAVE_NEW_TAB_DASHBOARD_SETTINGS_TITLE },
        { "customize", IDS_BRAVE_NEW_TAB_CUSTOMIZE },
        { "showBackgroundImage", IDS_BRAVE_NEW_TAB_SHOW_BACKGROUND_IMAGE },
        { "showBraveStats", IDS_BRAVE_NEW_TAB_SHOW_BRAVE_STATS },
        { "showClock", IDS_BRAVE_NEW_TAB_SHOW_CLOCK },
        { "clockFormat", IDS_BRAVE_NEW_TAB_CLOCK_FORMAT },
        { "clockFormatDefault", IDS_BRAVE_NEW_TAB_CLOCK_FORMAT_DEFAULT },
        { "clockFormat12", IDS_BRAVE_NEW_TAB_CLOCK_FORMAT_12 },
        { "clockFormat24", IDS_BRAVE_NEW_TAB_CLOCK_FORMAT_24 },
        { "addTopSiteDialogTitle", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_TITLE },  // NOLINT
        { "editTopSiteDialogTitle", IDS_BRAVE_NEW_TAB_EDIT_TOP_SITE_DIALOG_TITLE },  // NOLINT
        { "editSiteTileMenuItem", IDS_BRAVE_NEW_TAB_EDIT_SITE_TILE_MENU_ITEM },
        { "removeTileMenuItem", IDS_BRAVE_NEW_TAB_REMOVE_TILE_MENU_ITEM },
        { "addTopSiteDialogURLLabel", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_URL_LABEL },  // NOLINT
        { "addTopSiteDialogURLInputPlaceHolder", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_URL_INPUT_PLACEHOLDER },  // NOLINT
        { "addTopSiteDialogNameLabel", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_NAME_LABEL },  // NOLINT
        { "addTopSiteDialogNameInputPlaceHolder", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_NAME_INPUT_PLACEHOLDER },  // NOLINT
        { "addTopSiteDialogSaveButtonLabel", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_SAVE_BUTTON_LABEL },  // NOLINT
        { "addTopSiteDialogCancelButtonLabel", IDS_BRAVE_NEW_TAB_ADD_TOP_SITE_DIALOG_CANCEL_BUTTON_LABEL },  // NOLINT
        { "showTopSites", IDS_BRAVE_NEW_TAB_SHOW_TOP_SITES },
        { "showFavoritesLabel", IDS_BRAVE_NEW_TAB_SHOW_FAVORITES_LABEL },
        { "showFavoritesDesc", IDS_BRAVE_NEW_TAB_SHOW_FAVORITES_DESC },
        { "showFrecencyLabel", IDS_BRAVE_NEW_TAB_SHOW_FRECENCY_LABEL },
        { "showFrecencyDesc", IDS_BRAVE_NEW_TAB_SHOW_FRECENCY_DESC },
        { "addSiteMenuLabel", IDS_BRAVE_NEW_TAB_ADD_SITE_MENU_LABEL },
        { "showFrecencyMenuLabel", IDS_BRAVE_NEW_TAB_SHOW_FRECENCY_MENU_LABEL },
        { "showFavoritesMenuLabel", IDS_BRAVE_NEW_TAB_SHOW_FAVORITES_MENU_LABEL },  // NOLINT
        { "showRewards", IDS_BRAVE_NEW_TAB_SHOW_REWARDS },
        { "showBinance", IDS_BRAVE_NEW_TAB_SHOW_BINANCE },
        { "showTogether", IDS_BRAVE_NEW_TAB_SHOW_TOGETHER },
        { "cards", IDS_BRAVE_NEW_TAB_SHOW_CARDS },
        { "brandedWallpaperOptIn", IDS_BRAVE_NEW_TAB_BRANDED_WALLPAPER_OPT_IN },
        { "topSitesTitle", IDS_BRAVE_NEW_TAB_TOP_SITES },
        { "statsTitle", IDS_BRAVE_NEW_TAB_STATS },
        { "clockTitle", IDS_BRAVE_NEW_TAB_CLOCK },
        { "backgroundImageTitle", IDS_BRAVE_NEW_TAB_BACKGROUND_IMAGE },
        { "settingsNavigateBack", IDS_BRAVE_NEW_TAB_SETTINGS_BACK },

        { "braveTodayTitle", IDS_BRAVE_TODAY_TITLE },
        { "braveTodayIntroTitle", IDS_BRAVE_TODAY_INTRO_TITLE },
        { "braveTodayIntroDescription", IDS_BRAVE_TODAY_INTRO_DESCRIPTION },
        { "braveTodayStatusFetching", IDS_BRAVE_TODAY_STATUS_FETCHING},
        { "braveTodayActionRefresh", IDS_BRAVE_TODAY_ACTION_REFRESH},
        { "braveTodayScrollHint", IDS_BRAVE_TODAY_SCROLL_HINT},
        { "braveTodayResetAction", IDS_BRAVE_TODAY_RESET_ACTION},
        { "braveTodayResetConfirm", IDS_BRAVE_TODAY_RESET_CONFIRM},
        { "braveTodayCategoryNameAll", IDS_BRAVE_TODAY_CATEGORY_NAME_ALL},
        { "braveTodaySourcesTitle", IDS_BRAVE_TODAY_SOURCES_TITLE},
        { "braveTodayDisableSourceCommand",
            IDS_BRAVE_TODAY_DISABLE_SOURCE_COMMAND},
        { "promoted", IDS_BRAVE_TODAY_PROMOTED },

        { "addWidget", IDS_BRAVE_NEW_TAB_WIDGET_ADD },
        { "hideWidget", IDS_BRAVE_NEW_TAB_WIDGET_HIDE },
        { "rewardsWidgetDesc", IDS_BRAVE_NEW_TAB_REWARDS_WIDGET_DESC },
        { "binanceWidgetDesc", IDS_BRAVE_NEW_TAB_BINANCE_WIDGET_DESC },
        { "geminiWidgetDesc", IDS_BRAVE_NEW_TAB_GEMINI_WIDGET_DESC },
        { "cardsToggleTitle", IDS_BRAVE_NEW_TAB_CARDS_TITLE },
        { "cardsToggleDesc", IDS_BRAVE_NEW_TAB_CARDS_DESC },
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
        { "cryptoDotComWidgetDesc", IDS_BRAVE_NEW_TAB_CRYPTO_DOT_COM_WIDGET_DESC },              // NOLINT
#endif
        { "braveRewardsTitle", IDS_BRAVE_NEW_TAB_BRAVE_REWARDS_TITLE },
        // Private Tab - General
        { "learnMore", IDS_BRAVE_PRIVATE_NEW_TAB_LEARN_MORE },
        { "done", IDS_BRAVE_PRIVATE_NEW_TAB_DONE },
        { "searchSettings", IDS_BRAVE_PRIVATE_NEW_TAB_SEARCH_SETTINGS },
        { "headerLabel", IDS_BRAVE_PRIVATE_NEW_TAB_THIS_IS_A },

        // Private Tab - Header Private Window
        { "headerTitle", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW },
        { "headerText", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_DESC },
        { "headerButton", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_BUTTON },

        // Private Tab - Header Private Window with Tor
        { "headerTorTitle", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_TOR },
        { "headerTorText", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WINDOW_TOR_DESC },
        { "headerTorButton", IDS_BRAVE_PRIVATE_NEW_TAB_PRIVATE_WIONDOW_TOR_BUTTON },             // NOLINT

        // Private Tab - Box for DDG
        { "boxDdgLabel", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_DDG_LABEL },
        { "boxDdgTitle", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_DDG_TITLE },
        { "boxDdgText", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_DDG_TEXT_1 },
        { "boxDdgText2", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_DDG_TEXT_2 },
        { "boxDdgButton", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_DDG_BUTTON },

        // Private Tab - Box for Tor
        { "boxTorLabel", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_LABEL },
        { "boxTorLabel2", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_LABEL_2 },
        { "boxTorTitle", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_TITLE },

        // Private Tab - Private Window with Tor - Tor Box
        { "boxTorText", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_TEXT_1 },

        // Private Tab - Private Window - Tor Box
        { "boxTorText2", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_TEXT_2 },
        { "boxTorButton", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_BUTTON },

        // Private Tab - Private Window - Tor Status
        { "torStatus", IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS },
        { "torStatusConnected", IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_CONNECTED },         // NOLINT
        { "torStatusDisconnected", IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_DISCONNECTED },   // NOLINT
        { "torStatusInitializing", IDS_BRAVE_PRIVATE_NEW_TAB_TOR_STATUS_INITIALIZING },   // NOLINT

        // Together prompt
        { "togetherPromptTitle", IDS_BRAVE_TOGETHER_PROMPT_TITLE },
        { "togetherPromptDescription", IDS_BRAVE_TOGETHER_PROMPT_DESCRIPTION },
        { "togetherPromptAction", IDS_BRAVE_TOGETHER_PROMPT_ACTION },

        // Rewards widget
        { "rewardsWidgetBap", IDS_BRAVE_UI_BAP_REWARDS_TEXT },
        { "rewardsWidgetBat", IDS_BRAVE_UI_BAT_REWARDS_TEXT },
        { "rewardsWidgetBraveRewards", IDS_BRAVE_UI_BRAVE_REWARDS },
        { "rewardsWidgetTurnOnAds", IDS_BRAVE_UI_TURN_ON_ADS },
        { "rewardsWidgetTurnOnText", IDS_BRAVE_UI_TURN_ON_TEXT },
        { "rewardsWidgetTurnOnTitle", IDS_BRAVE_UI_TURN_ON_TITLE },
        { "rewardsWidgetClaimMyRewards", IDS_REWARDS_WIDGET_CLAIM_MY_REWARDS },
        { "rewardsWidgetAboutRewards", IDS_REWARDS_WIDGET_ABOUT_REWARDS },
        { "rewardsWidgetServiceText", IDS_REWARDS_WIDGET_SERVICE_TEXT },
        { "rewardsWidgetEstimatedEarnings", IDS_REWARDS_WIDGET_ESTIMATED_EARNINGS },      // NOLINT
        { "rewardsWidgetMonthlyTips", IDS_REWARDS_WIDGET_MONTHLY_TIPS },
        { "rewardsWidgetEnableBrandedWallpaperTitle", IDS_REWARDS_WIDGET_ENABLE_BRANDED_WALLPAPER_TITLE },  // NOLINT
        { "rewardsWidgetEnableBrandedWallpaperSubTitle", IDS_REWARDS_WIDGET_ENABLE_BRANDED_WALLPAPER_SUBTITLE },            // NOLINT
        { "rewardsWidgetAdsNotSupported", IDS_BRAVE_REWARDS_LOCAL_ADS_NOT_SUPPORTED_REGION },    // NOLINT
        { "rewardsWidgetNotificationTitle", IDS_REWARDS_WIDGET_NOTIFICATION_TITLE },      // NOLINT
        { "rewardsWidgetNotificationTextAds", IDS_REWARDS_WIDGET_NOTIFICATION_TEXT_ADS }, // NOLINT
        { "rewardsWidgetNotificationTextUGP", IDS_REWARDS_WIDGET_NOTIFICATION_TEXT_UGP },  // NOLINT
        { "rewardsWidgetBrandedNotificationTitle", IDS_REWARDS_WIDGET_BRANDED_NOTIFICATION_TITLE },      // NOLINT
        { "rewardsWidgetBrandedNotificationDescription", IDS_REWARDS_WIDGET_BRANDED_NOTIFICATION_DESCRIPTION }, // NOLINT
        { "rewardsWidgetBrandedNotificationHideAction", IDS_REWARDS_WIDGET_BRANDED_NOTIFICATION_HIDE_ACTION }, // NOLINT
        { "editCardsTitle", IDS_EDIT_CARDS_TITLE },
        { "tosAndPp", IDS_REWARDS_WIDGET_TOS_AND_PP},     // NOLINT
        { "rewardsWidgetStartUsing", IDS_REWARDS_WIDGET_START_USING},     // NOLINT
        // Rewards BAP Deprecation Alert
        { "bapDeprecationAlertText", IDS_REWARDS_BAP_DEPRECATION_ALERT_TEXT },
        { "bapDeprecationHeader", IDS_REWARDS_BAP_DEPRECATION_HEADER },
        { "bapDeprecationOK", IDS_REWARDS_BAP_DEPRECATION_OK },
        // Together Widget
        { "togetherWidgetTitle", IDS_TOGETHER_WIDGET_TITLE },
        { "togetherWidgetWelcomeTitle", IDS_TOGETHER_WIDGET_WELCOME_TITLE },
        { "togetherWidgetStartButton", IDS_TOGETHER_WIDGET_START_BUTTON },
        { "togetherWidgetAboutData", IDS_TOGETHER_WIDGET_ABOUT_DATA },
        // Binance Widget
        { "binanceWidgetBuy", IDS_BINANCE_WIDGET_BUY },
        { "binanceWidgetBuyCrypto", IDS_BINANCE_WIDGET_BUY_CRYPTO },
        { "binanceWidgetBuyDefault", IDS_BINANCE_WIDGET_BUY_DEFAULT },
        { "binanceWidgetWelcomeTitle", IDS_BINANCE_WIDGET_WELCOME_TITLE },
        { "binanceWidgetSubText", IDS_BINANCE_WIDGET_SUB_TEXT },
        { "binanceWidgetConnectText", IDS_BINANCE_WIDGET_CONNECT_TEXT },
        { "binanceWidgetDismissText", IDS_BINANCE_WIDGET_DISMISS_TEXT },
        { "binanceWidgetValueText", IDS_BINANCE_WIDGET_VALUE_TEXT },
        { "binanceWidgetBTCTickerText" , IDS_BINANCE_BTC_TICKER_TEXT },
        { "binanceWidgetViewDetails", IDS_BRAVE_UI_VIEW_DETAILS },
        { "binanceWidgetDepositLabel", IDS_BINANCE_WIDGET_DEPOSIT_LABEL },
        { "binanceWidgetTradeLabel", IDS_BINANCE_WIDGET_TRADE_LABEL },
        { "binanceWidgetInvalidEntry", IDS_BINANCE_WIDGET_INVALID_ENTRY },
        { "binanceWidgetValidatingCreds", IDS_BINANCE_WIDGET_VALIDATING_CREDS },    // NOLINT
        { "binanceWidgetDisconnectTitle", IDS_BINANCE_WIDGET_DISCONNECT_TITLE },    // NOLINT
        { "binanceWidgetDisconnectText" , IDS_BINANCE_WIDGET_DISCONNECT_TEXT },     // NOLINT
        { "binanceWidgetDisconnectButton" , IDS_BINANCE_WIDGET_DISCONNECT_BUTTON }, // NOLINT
        { "binanceWidgetCancelText" , IDS_BRAVE_UI_CANCEL },
        { "binanceWidgetAccountDisconnected" , IDS_BINANCE_WIDGET_ACCOUNT_DISCONNECTED }, // NOLINT
        { "binanceWidgetConfigureButton" , IDS_BINANCE_WIDGET_CONFIGURE_BUTTON },         // NOLINT
        { "binanceWidgetConnect", IDS_BINANCE_WIDGET_CONNECT },
        { "binanceWidgetConverted", IDS_BINANCE_WIDGET_CONVERTED },
        { "binanceWidgetContinue", IDS_BINANCE_WIDGET_CONTINUE },
        { "binanceWidgetUnableToConvert", IDS_BINANCE_WIDGET_UNABLE_TO_CONVERT },         // NOLINT
        { "binanceWidgetRetry", IDS_BINANCE_WIDGET_RETRY },
        { "binanceWidgetInsufficientFunds", IDS_BINANCE_WIDGET_INSUFFICIENT_FUNDS },      // NOLINT
        { "binanceWidgetConversionFailed", IDS_BINANCE_WIDGET_CONVERSION_FAILED },        // NOLINT
        { "binanceWidgetDone", IDS_BINANCE_WIDGET_DONE },
        { "binanceWidgetCopy", IDS_BINANCE_WIDGET_COPY },
        { "binanceWidgetSearch", IDS_BINANCE_WIDGET_SEARCH },
        { "binanceWidgetAddressUnavailable", IDS_BINANCE_WIDGET_ADDRESS_UNAVAILABLE },    // NOLINT
        { "binanceWidgetDepositAddress", IDS_BINANCE_WIDGET_DEPOSIT_ADDRESS },
        { "binanceWidgetDepositMemo", IDS_BINANCE_WIDGET_DEPOSIT_MEMO },
        { "binanceWidgetConfirmConversion", IDS_BINANCE_WIDGET_CONFIRM_CONVERSION },      // NOLINT
        { "binanceWidgetConvert", IDS_BINANCE_WIDGET_CONVERT },
        { "binanceWidgetRate", IDS_BINANCE_WIDGET_RATE },
        { "binanceWidgetFee", IDS_BINANCE_WIDGET_FEE },
        { "binanceWidgetWillReceive", IDS_BINANCE_WIDGET_WILL_RECEIVE },
        { "binanceWidgetConfirm", IDS_BINANCE_WIDGET_CONFIRM },
        { "binanceWidgetCancel", IDS_BINANCE_WIDGET_CANCEL },
        { "binanceWidgetAvailable" , IDS_BINANCE_WIDGET_AVAILABLE },
        { "binanceWidgetConvertIntent", IDS_BINANCE_WIDGET_CONVERT_INTENT },
        { "binanceWidgetPreviewConvert", IDS_BINANCE_WIDGET_PREVIEW_CONVERT },
        { "binanceWidgetSummary", IDS_BINANCE_WIDGET_SUMMARY },
        { "binanceWidgetAuthInvalid", IDS_BINANCE_WIDGET_AUTH_INVALID },
        { "binanceWidgetAuthInvalidCopy", IDS_BINANCE_WIDGET_AUTH_INVALID_COPY },         // NOLINT
        { "binanceWidgetRefreshData", IDS_BINANCE_WIDGET_REFRESH_DATA },
        { "binanceWidgetUnderMinimum", IDS_BINANCE_WIDGET_UNDER_MINIMUM },
        // Gemini Widget
        { "geminiWidgetAuthInvalid", IDS_BINANCE_WIDGET_AUTH_INVALID },
        { "geminiWidgetAuthInvalidCopy", IDS_GEMINI_WIDGET_AUTH_INVALID_COPY },
        { "geminiWidgetDone", IDS_BINANCE_WIDGET_DONE },
        { "geminiWidgetCopy", IDS_BINANCE_WIDGET_COPY },
        { "geminiWidgetRetry", IDS_BINANCE_WIDGET_RETRY },
        { "geminiWidgetCancel", IDS_BINANCE_WIDGET_CANCEL },
        { "geminiWidgetConfirm", IDS_BINANCE_WIDGET_CONFIRM },
        { "geminiWidgetDisconnectTitle", IDS_BINANCE_WIDGET_DISCONNECT_TITLE },
        { "geminiWidgetDisconnectText", IDS_BINANCE_WIDGET_DISCONNECT_TEXT },
        { "geminiWidgetDisconnectButton", IDS_BINANCE_WIDGET_DISCONNECT_BUTTON },         // NOLINT
        { "geminiWidgetCancelText", IDS_BINANCE_WIDGET_CANCEL },
        { "geminiWidgetDismissText", IDS_BINANCE_WIDGET_DISMISS_TEXT },
        { "geminiWidgetConnectTitle", IDS_GEMINI_WIDGET_CONNECT_TITLE },
        { "geminiWidgetConnectCopy", IDS_GEMINI_WIDGET_CONNECT_COPY },
        { "geminiWidgetConnectButton", IDS_GEMINI_WIDGET_CONNECT_BUTTON },
        { "geminiWidgetFailedTrade", IDS_GEMINI_WIDGET_FAILED_TRADE },
        { "geminiWidgetInsufficientFunds", IDS_BINANCE_WIDGET_INSUFFICIENT_FUNDS },       // NOLINT
        { "geminiWidgetError", IDS_GEMINI_WIDGET_ERROR },
        { "geminiWidgetConfirmTrade", IDS_GEMINI_WIDGET_CONFIRM_TRADE },
        { "geminiWidgetBuy", IDS_BINANCE_WIDGET_BUY },
        { "geminiWidgetSell", IDS_GEMINI_WIDGET_SELL },
        { "geminiWidgetAvailable", IDS_BINANCE_WIDGET_AVAILABLE },
        { "geminiWidgetGetQuote", IDS_GEMINI_WIDGET_GET_QUOTE },
        { "geminiWidgetUnavailable", IDS_BINANCE_WIDGET_ADDRESS_UNAVAILABLE },
        { "geminiWidgetDepositAddress", IDS_BINANCE_WIDGET_DEPOSIT_ADDRESS },
        { "geminiWidgetSearch", IDS_BINANCE_WIDGET_SEARCH },
        { "geminiWidgetDepositLabel", IDS_BINANCE_WIDGET_DEPOSIT_LABEL },
        { "geminiWidgetTradeLabel", IDS_GEMINI_WIDGET_TRADE_LABEL },
        { "geminiWidgetBalanceLabel", IDS_GEMINI_WIDGET_BALANCE_LABEL },
        { "geminiWidgetBuying", IDS_GEMINI_WIDGET_BUYING },
        { "geminiWidgetSelling", IDS_GEMINI_WIDGET_SELLING },
        { "geminiWidgetContinue", IDS_BINANCE_WIDGET_CONTINUE },
        { "geminiWidgetBought", IDS_GEMINI_WIDGET_BOUGHT },
        { "geminiWidgetSold", IDS_GEMINI_WIDGET_SOLD },
        { "geminiWidgetFee", IDS_BINANCE_WIDGET_FEE },
        { "geminiWidgetUnitPrice", IDS_GEMINI_WIDGET_UNIT_PRICE },
        { "geminiWidgetTotalPrice", IDS_GEMINI_WIDGET_TOTAL_PRICE },
        { "geminiWidgetTotalAmount", IDS_GEMINI_WIDGET_TOTAL_AMOUNT },
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
        { "cryptoDotComWidgetShowPrice", IDS_CRYPTO_DOT_COM_WIDGET_SHOW_PRICE },
        { "cryptoDotComWidgetBuy", IDS_BINANCE_WIDGET_BUY },
        { "cryptoDotComWidgetCopyOne", IDS_CRYPTO_DOT_COM_WIDGET_COPY_ONE },
        { "cryptoDotComWidgetCopyTwo", IDS_CRYPTO_DOT_COM_WIDGET_COPY_TWO },
        { "cryptoDotComWidgetBuyBtc", IDS_CRYPTO_DOT_COM_WIDGET_BUY_BTC },
        { "cryptoDotComWidgetViewMarkets", IDS_CRYPTO_DOT_COM_WIDGET_VIEW_MARKETS },      // NOLINT
        { "cryptoDotComWidgetGraph", IDS_CRYPTO_DOT_COM_WIDGET_GRAPH },
        { "cryptoDotComWidgetPairs", IDS_CRYPTO_DOT_COM_WIDGET_PAIRS },
        { "cryptoDotComWidgetVolume", IDS_CRYPTO_DOT_COM_WIDGET_VOLUME },
#endif
      }
    }, {
      std::string("wallet"), {
        { "cryptoWalletsWelcome", IDS_BRAVE_WALLET_WELCOME },
        { "cryptoWalletsDisclosureOne", IDS_BRAVE_WALLET_DISCLOSURE_ONE },
        { "cryptoWalletsDisclosureTwo", IDS_BRAVE_WALLET_DISCLOSURE_TWO },
        { "cryptoWalletsDisclosureThree", IDS_BRAVE_WALLET_DISCLOSURE_THREE },
        { "cryptoWalletsDisclosureFour", IDS_BRAVE_WALLET_DISCLOSURE_FOUR },
        { "cryptoWalletsBraveRewards", IDS_BRAVE_WALLET_BRAVE_REWARDS },
        { "cryptoWalletsDownloading", IDS_BRAVE_WALLET_DOWNLOADING },
        { "cryptoWalletsDisclosureConfirm", IDS_BRAVE_WALLET_DISCLOSURE_CONFIRM }         // NOLINT
      }
    }, {
      std::string("welcome"), {
#if !defined(OS_ANDROID)
        { "headerText", IDS_WELCOME_HEADER },
#endif
        { "welcome", IDS_BRAVE_WELCOME_PAGE_MAIN_TITLE },
        { "whatIsBrave", IDS_BRAVE_WELCOME_PAGE_MAIN_DESC },
        { "letsGo", IDS_BRAVE_WELCOME_PAGE_MAIN_BUTTON },
        { "braveRewardsTitle", IDS_BRAVE_WELCOME_PAGE_REWARDS_TITLE },
        { "setupBraveRewards", IDS_BRAVE_WELCOME_PAGE_REWARDS_DESC },
        { "braveRewardsTerms", IDS_BRAVE_WELCOME_PAGE_REWARDS_TERMS },
        { "braveRewardsNote", IDS_BRAVE_WELCOME_PAGE_REWARDS_NOTE },
        { "enableRewards", IDS_BRAVE_WELCOME_PAGE_REWARDS_BUTTON },
        { "importFromAnotherBrowser", IDS_BRAVE_WELCOME_PAGE_IMPORT_TITLE },
        { "setupImport", IDS_BRAVE_WELCOME_PAGE_IMPORT_DESC },
        { "import", IDS_BRAVE_WELCOME_PAGE_IMPORT_BUTTON },
        { "importFrom", IDS_BRAVE_WELCOME_PAGE_IMPORT_FROM_DESC },
        { "default", IDS_BRAVE_WELCOME_PAGE_DEFAULT_TEXT },
        { "manageShields", IDS_BRAVE_WELCOME_PAGE_SHIELDS_TITLE },
        { "adjustProtectionLevel", IDS_BRAVE_WELCOME_PAGE_SHIELDS_DESC },
        { "shieldSettings", IDS_BRAVE_WELCOME_PAGE_SHIELDS_BUTTON },
        { "setDefault", IDS_BRAVE_WELCOME_PAGE_SET_DEFAULT_SEARCH_BUTTON },
        { "setDefaultSearchEngine", IDS_BRAVE_WELCOME_PAGE_SEARCH_TITLE },
        { "chooseSearchEngine", IDS_BRAVE_WELCOME_PAGE_SEARCH_DESC },
        { "selectSearchEngine", IDS_BRAVE_WELCOME_PAGE_SEARCH_SELECT },
        { "privateExperience", IDS_BRAVE_WELCOME_PAGE_PRIVATE_EXPERIENCE_DESC },
        { "skipWelcomeTour", IDS_BRAVE_WELCOME_PAGE_SKIP_BUTTON },
        { "next", IDS_BRAVE_WELCOME_PAGE_NEXT_BUTTON },
        { "done", IDS_BRAVE_WELCOME_PAGE_DONE_BUTTON },
        { "privacyTitle", IDS_BRAVE_WELCOME_PAGE_PRIVACY_TITLE },
        { "privacyDesc", IDS_BRAVE_WELCOME_PAGE_PRIVACY_DESC }
      }
    }, {
      std::string("rewards"), {
        { "adsCurrentEarnings",  IDS_BRAVE_REWARDS_LOCAL_ADS_CURRENT_EARNINGS },
        { "adsDesc",  IDS_BRAVE_REWARDS_LOCAL_ADS_DESC },
        { "adsDisabledTextOne",  IDS_BRAVE_REWARDS_LOCAL_ADS_DISABLED_TEXT_ONE },                // NOLINT
        { "adsDisabledTextTwo",  IDS_BRAVE_REWARDS_LOCAL_ADS_DISABLED_TEXT_TWO },                // NOLINT
        { "adsNotificationsReceived",  IDS_BRAVE_REWARDS_LOCAL_ADS_NOTIFICATIONS_RECEIVED },     // NOLINT
        { "adsNotSupportedRegion", IDS_BRAVE_REWARDS_LOCAL_ADS_NOT_SUPPORTED_REGION },           // NOLINT
        { "adsNotSupportedDevice", IDS_BRAVE_REWARDS_LOCAL_ADS_NOT_SUPPORTED_DEVICE },           // NOLINT
        { "adsPaymentDate",  IDS_BRAVE_REWARDS_LOCAL_ADS_PAYMENT_DATE },
        { "adsPagesViewed",  IDS_BRAVE_REWARDS_LOCAL_ADS_PAGES_VIEWED },
        { "adsOtherSettings",  IDS_BRAVE_REWARDS_LOCAL_ADS_OTHER_SETTINGS },
        { "adsPerHour",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR },
        { "adsPerHour1",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_1 },
        { "adsPerHour2",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_2 },
        { "adsPerHour3",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_3 },
        { "adsPerHour4",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_4 },
        { "adsPerHour5",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_5 },
        { "adsSubdivisionTargetingTitle",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_TITLE },  // NOLINT
        { "adsSubdivisionTargetingDescription",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_DESCRIPTION },  // NOLINT
        { "adsSubdivisionTargetingLearn",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_LEARN },  // NOLINT
        { "adsSubdivisionTargetingAutoDetectedAs",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_AUTO_DETECTED_AS },  // NOLINT
        { "adsSubdivisionTargetingAutoDetect",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_AUTO_DETECT },  // NOLINT
        { "adsSubdivisionTargetingDisable",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_DISABLE },  // NOLINT
        { "adsSubdivisionTargetingDisabled",  IDS_BRAVE_REWARDS_LOCAL_ADS_SUBDIVISION_TARGETING_DISABLED },  // NOLINT
        { "adsTitle",  IDS_BRAVE_REWARDS_LOCAL_ADS_TITLE },

        { "qrBoxText",  IDS_BRAVE_REWARDS_LOCAL_QR_BOX_TEXT },
        { "qrBoxButton",  IDS_BRAVE_REWARDS_LOCAL_QR_BOX_BUTTON },

        { "bat", IDS_BRAVE_UI_BAT_REWARDS_TEXT },
        { "bap", IDS_BRAVE_UI_BAP_REWARDS_TEXT },
        { "batPoints", IDS_BRAVE_UI_BAT_POINTS_TEXT },
        { "batPointsMessage", IDS_BRAVE_UI_POINTS_MESSAGE },
        { "contributionTitle",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TITLE },
        { "contributionDesc",  IDS_BRAVE_REWARDS_LOCAL_CONTR_DESC },
        { "contributionMonthly",  IDS_BRAVE_REWARDS_LOCAL_CONTR_MONTHLY },
        { "contributionNextDate",  IDS_BRAVE_REWARDS_LOCAL_CONTR_NEXT_DATE },
        { "contributionSites",  IDS_BRAVE_REWARDS_LOCAL_CONTR_SITES },
        { "contributionDisabledText1",  IDS_BRAVE_REWARDS_LOCAL_CONTR_DISABLED_TEXT1 },          // NOLINT
        { "contributionDisabledText2",  IDS_BRAVE_REWARDS_LOCAL_CONTR_DISABLED_TEXT2 },          // NOLINT
        { "contributionVisitSome",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_SOME },
        { "contributionMinTime",  IDS_BRAVE_REWARDS_LOCAL_CONTR_MIN_TIME },
        { "contributionMinVisits",  IDS_BRAVE_REWARDS_LOCAL_CONTR_MIN_VISITS },
        { "contributionOther",  IDS_BRAVE_REWARDS_LOCAL_CONTR_OTHER },
        { "contributionShowNonVerified",  IDS_BRAVE_REWARDS_LOCAL_CONTR_SHOW_NON_VERIFIED },        // NOLINT
        { "contributionVideos",  IDS_BRAVE_REWARDS_LOCAL_CONTR_ALLOW_VIDEOS },
        { "contributionVisit1",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_1 },
        { "contributionVisit5",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_5 },
        { "contributionVisit10",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_10 },
        { "contributionTime5",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_5 },
        { "contributionTime8",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_8 },
        { "contributionTime60",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_60 },
        { "contributionUpTo",  IDS_BRAVE_REWARDS_LOCAL_CONTR_UP_TO },

        { "deviceOffline", IDS_BRAVE_REWARDS_LOCAL_DEVICE_OFFLINE },
        { "donationTitle",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TITLE },
        { "donationDesc",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DESC },
        { "donationTotalDonations",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TOTAL_DONATIONS },            // NOLINT
        { "donationTotalMonthlyContribution",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TOTAL_MONTHLY_CONTRIBUTION },       // NOLINT
        { "donationVisitSome",  IDS_BRAVE_REWARDS_LOCAL_DONAT_VISIT_SOME },
        { "donationAbility",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY },
        { "donationAbilityYT",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY_YT },
        { "donationAbilityReddit", IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY_REDT},
        { "donationAbilityTwitter",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY_TW },
        { "donationAbilityGitHub",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY_GH },
        { "donationDisabledText1",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DISABLED_TEXT1 },              // NOLINT
        { "donationDisabledText2",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DISABLED_TEXT2 },              // NOLINT
        { "donationNextDate",  IDS_BRAVE_REWARDS_LOCAL_DONAT_NEXT_DATE },
        { "monthlyContributionTitle",  IDS_BRAVE_REWARDS_LOCAL_MONTHLY_CONTRIBUTION_TITLE },     // NOLINT
        { "monthlyContributionDesc",  IDS_BRAVE_REWARDS_LOCAL_MONTHLY_CONTRIBUTION_DESC },       // NOLINT
        { "monthlyContributionEmpty", IDS_BRAVE_REWARDS_LOCAL_MONTHLY_CONTRIBUTION_EMPTY },      // NOLINT
        { "monthlyContributionDisabledText", IDS_BRAVE_REWARDS_LOCAL_MONTHLY_CONTRIBUTION_DISABLED_TEXT },      // NOLINT

        { "panelAddFunds",  IDS_BRAVE_REWARDS_LOCAL_PANEL_ADD_FUNDS },
        { "panelWithdrawFunds",  IDS_BRAVE_REWARDS_LOCAL_PANEL_WITHDRAW_FUNDS },
        { "tokens",  IDS_BRAVE_REWARDS_LOCAL_TOKENS },
        { "walletRecoverySuccess",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RECOVERY_SUCCESS },           // NOLINT
        { "walletRestored",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RESTORED },
        { "walletRecoveryFail",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RECOVERY_FAIL },                 // NOLINT
        { "walletRecoveryOutdated",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RECOVERY_OUTDATED },                 // NOLINT
        { "almostThere",  IDS_BRAVE_REWARDS_LOCAL_ALMOST_THERE },
        { "notQuite",  IDS_BRAVE_REWARDS_LOCAL_NOT_QUITE },
        { "proveHuman",  IDS_BRAVE_REWARDS_LOCAL_PROVE_HUMAN },
        { "serverNotResponding",  IDS_BRAVE_REWARDS_LOCAL_SERVER_NOT_RESPONDING },               // NOLINT
        { "uhOh",  IDS_BRAVE_REWARDS_LOCAL_UH_OH },
        { "grantGeneralErrorTitle",  IDS_BRAVE_REWARDS_LOCAL_GENERAL_GRANT_ERROR_TITLE },        // NOLINT
        { "grantGeneralErrorButton",  IDS_BRAVE_REWARDS_LOCAL_GENERAL_GRANT_ERROR_BUTTON },      // NOLINT
        { "grantGeneralErrorText",  IDS_BRAVE_REWARDS_LOCAL_GENERAL_GRANT_ERROR_TEXT },          // NOLINT
        { "redirectModalError", IDS_BRAVE_REWARDS_LOCAL_REDIRECT_MODAL_ERROR },
        { "redirectModalClose", IDS_BRAVE_REWARDS_LOCAL_REDIRECT_MODAL_CLOSE },
        { "redirectModalErrorWallet", IDS_BRAVE_REWARDS_LOCAL_REDIRECT_MODAL_ERROR_WALLET },     // NOLINT
        { "redirectModalBatLimitTitle", IDS_BRAVE_REWARDS_LOCAL_REDIRECT_MODAL_BAT_LIMIT_TITLE },     // NOLINT
        { "redirectModalBatLimitText", IDS_BRAVE_REWARDS_LOCAL_REDIRECT_MODAL_BAT_LIMIT_TEXT },     // NOLINT
        { "redirectModalNotAllowed", IDS_BRAVE_REWARDS_LOCAL_REDIRECT_MODAL_NOT_ALLOWED},     // NOLINT
        { "tosAndPp", IDS_BRAVE_REWARDS_LOCAL_TOS_AND_PP},     // NOLINT

        { "click",  IDS_BRAVE_REWARDS_LOCAL_ADS_CONFIRMATION_TYPE_CLICK },
        { "dismiss",  IDS_BRAVE_REWARDS_LOCAL_ADS_CONFIRMATION_TYPE_DISMISS },
        { "landed",  IDS_BRAVE_REWARDS_LOCAL_ADS_CONFIRMATION_TYPE_LANDED },
        { "view",  IDS_BRAVE_REWARDS_LOCAL_ADS_CONFIRMATION_TYPE_VIEW },

        { "about", IDS_BRAVE_UI_ABOUT },
        { "accept", IDS_BRAVE_UI_ACCEPT },
        { "activityCopy", IDS_BRAVE_UI_ACTIVITY_COPY },
        { "activityNote", IDS_BRAVE_UI_ACTIVITY_NOTE },
        { "addFunds", IDS_BRAVE_UI_ADD_FUNDS },
        { "ads", IDS_BRAVE_UI_ADS},
        { "adsCurrentlyViewing", IDS_BRAVE_UI_ADS_CURRENTLY_VIEWING },
        { "adsEarnings", IDS_BRAVE_UI_ADS_EARNINGS },
        { "adsHistoryFilterAll", IDS_BRAVE_UI_ADS_HISTORY_FILTER_ALL },
        { "adsHistoryFilterSaved", IDS_BRAVE_UI_ADS_HISTORY_FILTER_SAVED },
        { "adsHistorySubTitle", IDS_BRAVE_UI_ADS_HISTORY_SUBTITLE },
        { "adsHistoryTitle", IDS_BRAVE_UI_ADS_HISTORY_TITLE },
        { "adsGrantReceived", IDS_BRAVE_UI_ADS_GRANT_RECEIVED },
        { "all", IDS_BRAVE_UI_ADS_ALL },
        { "allowTip", IDS_BRAVE_UI_ALLOW_TIP },
        { "amount", IDS_BRAVE_UI_AMOUNT },
        { "autoContribute", IDS_BRAVE_UI_BRAVE_CONTRIBUTE_TITLE },
        { "autoContributeTransaction", IDS_BRAVE_UI_BRAVE_CONTRIBUTE_TRANSACTION },              // NOLINT
        { "backup", IDS_BRAVE_UI_BACKUP },
        { "braveAdsDesc", IDS_BRAVE_UI_BRAVE_ADS_DESC },
        { "braveAdsDescPoints", IDS_BRAVE_UI_BRAVE_ADS_DESC_POINTS },
        { "braveAdsLaunchTitle", IDS_BRAVE_UI_BRAVE_ADS_LAUNCH_TITLE },
        { "braveAdsTitle", IDS_BRAVE_UI_BRAVE_ADS_TITLE },
        { "braveContributeDesc", IDS_BRAVE_UI_BRAVE_CONTRIBUTE_DESC },
        { "braveContributeTitle", IDS_BRAVE_UI_BRAVE_CONTRIBUTE_TITLE },
        { "braveRewards", IDS_BRAVE_UI_BRAVE_REWARDS },
        { "braveRewardsCreatingText", IDS_BRAVE_UI_BRAVE_REWARDS_CREATING_TEXT },                // NOLINT
        { "braveRewardsDesc", IDS_BRAVE_UI_BRAVE_REWARDS_DESC },
        { "braveRewardsOptInText", IDS_BRAVE_UI_BRAVE_REWARDS_OPT_IN_TEXT },
        { "braveRewardsSubTitle", IDS_BRAVE_UI_BRAVE_REWARDS_SUB_TITLE },
        { "braveRewardsTeaser", IDS_BRAVE_UI_BRAVE_REWARDS_TEASER },
        { "braveRewardsTitle", IDS_BRAVE_UI_BRAVE_REWARDS_TITLE },
        { "braveVerified", IDS_BRAVE_UI_BRAVE_VERIFIED },
        { "cancel", IDS_BRAVE_UI_CANCEL },
        { "captchaDrag", IDS_BRAVE_UI_CAPTCHA_DRAG },
        { "captchaProveHuman", IDS_BRAVE_UI_CAPTCHA_PROVE_HUMAN },
        { "captchaTarget", IDS_BRAVE_UI_CAPTCHA_TARGET },
        { "captchaMissedTarget", IDS_BRAVE_UI_CAPTCHA_MISSED_TARGET },
        { "category", IDS_BRAVE_UI_ADS_CATEGORY },
        { "claim", IDS_BRAVE_UI_CLAIM },
        { "closeBalance", IDS_BRAVE_UI_CLOSE_BALANCE },
        { "contribute", IDS_BRAVE_UI_CONTRIBUTE },
        { "contributeAllocation", IDS_BRAVE_UI_CONTRIBUTE_ALLOCATION },
        { "copy", IDS_BRAVE_UI_COPY },
        { "currentDonation", IDS_BRAVE_UI_CURRENT_DONATION },
        { "date", IDS_BRAVE_UI_DATE },
        { "deposit", IDS_BRAVE_UI_DEPOSIT },
        { "deposits", IDS_BRAVE_UI_DEPOSITS },
        { "description", IDS_BRAVE_UI_DESCRIPTION },
        { "details", IDS_BRAVE_UI_DETAILS },
        { "disabledPanelOff", IDS_BRAVE_UI_DISABLED_PANEL_OFF },
        { "disabledPanelSettings", IDS_BRAVE_UI_DISABLED_PANEL_SETTINGS },
        { "disabledPanelText", IDS_BRAVE_UI_DISABLED_PANEL_TEXT },
        { "disabledPanelTitle", IDS_BRAVE_UI_DISABLED_PANEL_TITLE },
        { "dndCaptchaText1", IDS_BRAVE_UI_DND_CAPTCHA_TEXT_1 },
        { "dndCaptchaText2", IDS_BRAVE_UI_DND_CAPTCHA_TEXT_2 },
        { "donation", IDS_BRAVE_UI_DONATION },
        { "donationAmount", IDS_BRAVE_UI_DONATION_AMOUNT },
        { "donationTips", IDS_BRAVE_REWARDS_LOCAL_DONAT_TITLE },
        { "donateMonthly", IDS_BRAVE_UI_DONATE_MONTHLY },
        { "donateNow", IDS_BRAVE_UI_DONATE_NOW },
        { "done", IDS_BRAVE_UI_DONE },
        { "downloadPDF", IDS_BRAVE_UI_DOWNLOAD_PDF },
        { "earningsAds", IDS_BRAVE_UI_EARNINGS_ADS },
        { "earningsClaimDefault", IDS_BRAVE_UI_EARNINGS_CLAIM_DEFAULT },
        { "enableTips", IDS_BRAVE_UI_ENABLE_TIPS },
        { "excludeSite", IDS_BRAVE_UI_EXCLUDE_SITE },
        { "excludedSitesText", IDS_BRAVE_UI_EXCLUDED_SITES },
        { "expiresOn", IDS_BRAVE_UI_EXPIRES_ON },
        { "for", IDS_BRAVE_UI_FOR },
        { "grantDisclaimer", IDS_BRAVE_UI_GRANT_DISCLAIMER },
        { "grantTitleUGP", IDS_BRAVE_UI_GRANT_TITLE_UGP },
        { "grantSubtitleUGP", IDS_BRAVE_UI_GRANT_SUBTITLE_UGP },
        { "grantAmountTitleUGP", IDS_BRAVE_UI_GRANT_AMOUNT_TITLE_UGP },
        { "grantDateTitleUGP", IDS_BRAVE_UI_GRANT_DATE_TITLE_UGP },
        { "grantTitleAds", IDS_BRAVE_UI_GRANT_TITLE_ADS },
        { "grantSubtitleAds", IDS_BRAVE_UI_GRANT_SUBTITLE_ADS },
        { "grantAmountTitleAds", IDS_BRAVE_UI_GRANT_AMOUNT_TITLE_ADS },
        { "grantDateTitleAds", IDS_BRAVE_UI_GRANT_DATE_TITLE_ADS },
        { "grantExpire", IDS_BRAVE_UI_GRANT_EXPIRE },
        { "grantFinishTextAds", IDS_BRAVE_UI_GRANT_FINISH_TEXT_ADS },
        { "grantFinishTextUGP", IDS_BRAVE_UI_GRANT_FINISH_TEXT_UGP },
        { "grantFinishTitleAds", IDS_BRAVE_UI_GRANT_FINISH_TITLE_ADS },
        { "grantFinishTitleUGP", IDS_BRAVE_UI_GRANT_FINISH_TITLE_UGP },
        { "grantFinishTokenAds", IDS_BRAVE_UI_GRANT_FINISH_TOKEN_ADS },
        { "grantFinishTokenUGP", IDS_BRAVE_UI_GRANT_FINISH_TOKEN_UGP },
        { "grantFinishPointUGP", IDS_BRAVE_UI_GRANT_FINISH_POINT_UGP },
        { "grants", IDS_BRAVE_UI_GRANTS },
        { "greetingsVerified", IDS_BRAVE_UI_GREETINGS_VERIFIED },
        { "import", IDS_BRAVE_UI_IMPORT },
        { "includeInAuto", IDS_BRAVE_UI_INCLUDE_IN_AUTO },
        { "learnMore", IDS_BRAVE_UI_LEARN_MORE },
        { "login", IDS_BRAVE_UI_LOGIN },
        { "loginMessageTitle", IDS_BRAVE_UI_LOGIN_MESSAGE_TITLE },
        { "loginMessageText", IDS_BRAVE_UI_LOGIN_MESSAGE_TEXT },
        { "makeMonthly", IDS_BRAVE_UI_MAKE_MONTHLY },
        { "manageWallet", IDS_BRAVE_UI_MANAGE_WALLET },
        { "markAsInappropriate", IDS_BRAVE_UI_ADS_MARK_AS_INAPPROPRIATE },
        { "markAsInappropriateChecked", IDS_BRAVE_UI_ADS_MARK_AS_INAPPROPRIATE_CHECKED },        // NOLINT
        { "monthApr", IDS_BRAVE_UI_MONTH_APR },
        { "monthAug", IDS_BRAVE_UI_MONTH_AUG },
        { "monthDec", IDS_BRAVE_UI_MONTH_DEC },
        { "monthFeb", IDS_BRAVE_UI_MONTH_FEB },
        { "monthJan", IDS_BRAVE_UI_MONTH_JAN },
        { "monthJul", IDS_BRAVE_UI_MONTH_JUL },
        { "monthJun", IDS_BRAVE_UI_MONTH_JUN },
        { "monthMar", IDS_BRAVE_UI_MONTH_MAR },
        { "monthMay", IDS_BRAVE_UI_MONTH_MAY },
        { "monthNov", IDS_BRAVE_UI_MONTH_NOV },
        { "monthOct", IDS_BRAVE_UI_MONTH_OCT },
        { "monthSep", IDS_BRAVE_UI_MONTH_SEP },
        { "monthlyTips", IDS_BRAVE_UI_MONTHLY_TIPS },
        { "monthlyContributions", IDS_BRAVE_UI_MONTHLY_CONTRIBUTIONS },
        { "newGrant", IDS_BRAVE_UI_NEW_GRANT },
        { "newTokenGrant", IDS_BRAVE_UI_NEW_TOKEN_GRANT },
        { "noActivity", IDS_BRAVE_UI_NO_ACTIVITY },
        { "noAdsHistory", IDS_BRAVE_UI_ADS_NO_ADS_HISTORY },
        { "noGrants", IDS_BRAVE_UI_NO_GRANTS },
        { "notEnoughTokens", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS },
        { "notEnoughTokensLink", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS_LINK },
        { "noThankYou", IDS_BRAVE_UI_NO_THANK_YOU },
        { "off", IDS_BRAVE_UI_OFF },
        { "ok", IDS_BRAVE_UI_OK },
        { "on", IDS_BRAVE_UI_ON },
        { "onboardingBraveRewards", IDS_BRAVE_REWARDS_ONBOARDING_BRAVE_REWARDS },  // NOLINT
        { "onboardingDetailLinks", IDS_BRAVE_REWARDS_ONBOARDING_DETAIL_LINKS },
        { "onboardingEarnHeader", IDS_BRAVE_REWARDS_ONBOARDING_EARN_HEADER },
        { "onboardingEarnText", IDS_BRAVE_REWARDS_ONBOARDING_EARN_TEXT },
        { "onboardingPromoHeader", IDS_BRAVE_REWARDS_ONBOARDING_PROMO_HEADER },
        { "onboardingPromoText", IDS_BRAVE_REWARDS_ONBOARDING_PROMO_TEXT },
        { "onboardingSetupAdsHeader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_HEADER },  // NOLINT
        { "onboardingSetupAdsSubheader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_SUBHEADER },  // NOLINT
        { "onboardingSetupContributeHeader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_CONTRIBUTE_HEADER },  // NOLINT
        { "onboardingSetupContributeSubheader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_CONTRIBUTE_SUBHEADER },  // NOLINT
        { "onboardingStartUsingRewards", IDS_BRAVE_REWARDS_ONBOARDING_START_USING_REWARDS },  // NOLINT
        { "onboardingTakeTour", IDS_BRAVE_REWARDS_ONBOARDING_TAKE_TOUR },
        { "onboardingTerms", IDS_BRAVE_REWARDS_ONBOARDING_TERMS },
        { "onboardingTourBack", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_BACK },
        { "onboardingTourBegin", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_BEGIN },
        { "onboardingTourContinue", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_CONTINUE },  // NOLINT
        { "onboardingTourDone", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_DONE },
        { "onboardingTourSkip", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_SKIP },
        { "onboardingTourSkipForNow", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_SKIP_FOR_NOW },  // NOLINT
        { "onboardingPanelWelcomeHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_WELCOME_HEADER },  // NOLINT
        { "onboardingPanelWelcomeText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_WELCOME_TEXT },  // NOLINT
        { "onboardingPanelAdsHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_ADS_HEADER },  // NOLINT
        { "onboardingPanelAdsText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_ADS_TEXT },  // NOLINT
        { "onboardingPanelScheduleHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SCHEDULE_HEADER },  // NOLINT
        { "onboardingPanelScheduleText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SCHEDULE_TEXT },  // NOLINT
        { "onboardingPanelAcHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_AC_HEADER },  // NOLINT
        { "onboardingPanelAcText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_AC_TEXT },
        { "onboardingPanelTippingHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_TIPPING_HEADER },  // NOLINT
        { "onboardingPanelTippingText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_TIPPING_TEXT },  // NOLINT
        { "onboardingPanelRedeemHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_REDEEM_HEADER },  // NOLINT
        { "onboardingPanelRedeemText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_REDEEM_TEXT },  // NOLINT
        { "onboardingPanelSetupHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SETUP_HEADER },  // NOLINT
        { "onboardingPanelSetupText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SETUP_TEXT },  // NOLINT
        { "onboardingPanelCompleteHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_COMPLETE_HEADER },  // NOLINT
        { "onboardingPanelCompleteText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_COMPLETE_TEXT },  // NOLINT
        { "oneTime", IDS_BRAVE_UI_ONE_TIME },
        { "oneTimeDonation", IDS_BRAVE_UI_ONE_TIME_DONATION },
        { "openBalance", IDS_BRAVE_UI_OPEN_BALANCE },
        { "openAdsHistory", IDS_BRAVE_UI_OPEN_ADS_HISTORY },
        { "optOutTooltip", IDS_BRAVE_UI_ADS_OPT_OUT_TOOLTIP },
        { "payment", IDS_BRAVE_UI_PAYMENT },
        { "paymentNotMade", IDS_BRAVE_UI_PAYMENT_NOT_MADE },
        { "pendingContributions", IDS_BRAVE_UI_PENDING_CONTRIBUTIONS },
        { "pendingContributionEmpty", IDS_BRAVE_UI_PENDING_CONTRIBUTION_EMPTY },
        { "pendingContributionRemoveAll", IDS_BRAVE_UI_PENDING_CONTRIBUTION_REMOVE_ALL },        // NOLINT
        { "pendingTyperecurring", IDS_BRAVE_UI_PENDING_TYPE_RECURRING },
        { "pendingTypetip", IDS_BRAVE_UI_PENDING_TYPE_TIP },
        { "pendingTypeac", IDS_BRAVE_UI_PENDING_TYPE_AC },
        { "pendingUntil", IDS_BRAVE_UI_PENDING_UNTIL },
        { "pinnedSitesHeader", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_HEADER },
        { "pinnedSitesMsg", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_MSG },
        { "pinnedSitesOne", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_ONE },
        { "pinnedSitesTwo", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_TWO },
        { "pinnedSitesThree", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_THREE },
        { "pinnedSitesFour", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_FOUR },
        { "pleaseNote", IDS_BRAVE_UI_PLEASE_NOTE },
        { "point", IDS_BRAVE_UI_POINT },
        { "pointGrantClaimed", IDS_BRAVE_UI_POINT_GRANT_CLAIMED },
        { "points", IDS_BRAVE_UI_POINTS },
        { "print", IDS_BRAVE_UI_PRINT },
        { "processingRequest", IDS_BRAVE_UI_PROCESSING_REQUEST },
        { "processingRequestButton", IDS_BRAVE_UI_PROCESSING_REQUEST_BUTTON },
        { "processorBraveTokens", IDS_BRAVE_UI_PROCESSOR_BRAVE_TOKENS },
        { "processorUphold", IDS_BRAVE_UI_PROCESSOR_UPHOLD },
        { "processorBitflyer", IDS_BRAVE_UI_PROCESSOR_BITFLYER },
        { "processorBraveUserFunds", IDS_BRAVE_UI_PROCESSOR_BRAVE_USER_FUNDS },
        { "readyToTakePart", IDS_BRAVE_UI_READY_TO_TAKE_PART },
        { "readyToTakePartOptInText", IDS_BRAVE_UI_READY_TO_TAKE_PART_OPT_IN_TEXT },             // NOLINT
        { "readyToTakePartStart", IDS_BRAVE_UI_READY_TO_TAKE_PART_START },
        { "recoveryKeys", IDS_BRAVE_UI_RECOVERY_KEYS },
        { "recurring", IDS_BRAVE_UI_RECURRING },
        { "recurringDonation", IDS_BRAVE_UI_RECURRING_DONATION },
        { "recurringDonations", IDS_BRAVE_UI_RECURRING_DONATIONS },
        { "remove", IDS_BRAVE_UI_REMOVE },
        { "removeAdFromSaved", IDS_BRAVE_UI_REMOVE_AD_FROM_SAVED },
        { "reservedAmountText", IDS_BRAVE_UI_RESERVED_AMOUNT_TEXT },
        { "reservedMoreLink", IDS_BRAVE_UI_RESERVED_MORE_LINK },
        { "reservedAllLink", IDS_BRAVE_UI_RESERVED_ALL_LINK },
        { "reset", IDS_BRAVE_UI_RESET },
        { "restore", IDS_BRAVE_UI_RESTORE },
        { "restoreAll", IDS_BRAVE_UI_RESTORE_ALL },
        { "reviewSitesMsg", IDS_BRAVE_UI_REVIEW_SITE_MSG },
        { "rewardsBackupNoticeText1", IDS_BRAVE_UI_REWARDS_BACKUP_NOTICE_TEXT1 },                // NOLINT
        { "rewardsBackupNoticeText2", IDS_BRAVE_UI_REWARDS_BACKUP_NOTICE_TEXT2 },                // NOLINT
        { "rewardsBackupNoticeText3", IDS_BRAVE_UI_REWARDS_BACKUP_NOTICE_TEXT3 },                // NOLINT
        { "rewardsBackupText1", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT1 },
        { "rewardsBackupText2", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT2 },
        { "rewardsBackupText3", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT3 },
        { "rewardsBackupText4", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT4 },
        { "rewardsBackupText5", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT5 },
        { "rewardsBackupText6", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT6 },
        { "rewardsBannerText1", IDS_BRAVE_UI_REWARDS_BANNER_TEXT1 },
        { "rewardsBannerText2", IDS_BRAVE_UI_REWARDS_BANNER_TEXT2 },
        { "rewardsContribute", IDS_BRAVE_UI_REWARDS_CONTRIBUTE },
        { "rewardsContributeAttention", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_ATTENTION },             // NOLINT
        { "rewardsContributeAttentionScore", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_ATTENTION_SCORE },  // NOLINT
        { "rewardsOffText2", IDS_BRAVE_UI_REWARDS_OFF_TEXT2 },
        { "rewardsOffText3", IDS_BRAVE_UI_REWARDS_OFF_TEXT3 },
        { "rewardsOffText4", IDS_BRAVE_UI_REWARDS_OFF_TEXT4 },
        { "rewardsPanelEmptyTextPoints", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT_POINTS },         // NOLINT
        { "rewardsPanelEmptyText1", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT1 },
        { "rewardsPanelEmptyText2", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT2 },
        { "rewardsPanelEmptyText3", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT3 },
        { "rewardsPanelEmptyText4", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT4 },
        { "rewardsPanelEmptyText5", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT5 },
        { "rewardsPanelEmptyText6", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT6 },
        { "rewardsPanelOffText1", IDS_BRAVE_UI_REWARDS_PANEL_OFF_TEXT1 },
        { "rewardsPanelOffText2", IDS_BRAVE_UI_REWARDS_PANEL_OFF_TEXT2 },
        { "rewardsPanelText1", IDS_BRAVE_UI_REWARDS_PANEL_TEXT1 },
        { "rewardsPanelText2", IDS_BRAVE_UI_REWARDS_PANEL_TEXT2 },
        { "rewardsPanelText3", IDS_BRAVE_UI_REWARDS_PANEL_TEXT3 },
        { "rewardsPanelText4", IDS_BRAVE_UI_REWARDS_PANEL_TEXT4 },
        { "rewardsPanelTextVerify", IDS_BRAVE_UI_REWARDS_PANEL_VERIFY },
        { "rewardsRestoreText1", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT1 },
        { "rewardsRestoreText2", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT2 },
        { "rewardsRestoreText3", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT3 },
        { "rewardsRestoreText4", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT4 },
        { "rewardsRestoreWarning", IDS_BRAVE_UI_REWARDS_RESTORE_WARNING },
        { "rewardsResetConfirmation", IDS_BRAVE_UI_REWARDS_RESET_CONFIRMATION },
        { "rewardsResetTextFunds", IDS_BRAVE_UI_REWARDS_RESET_TEXT_FUNDS },
        { "rewardsResetTextNoFunds", IDS_BRAVE_UI_REWARDS_RESET_TEXT_NO_FUNDS },
        { "rewardsSummary", IDS_BRAVE_UI_REWARDS_SUMMARY },
        { "saved", IDS_BRAVE_UI_ADS_SAVED },
        { "saveAd", IDS_BRAVE_UI_ADS_SAVE_AD },
        { "saveAsFile", IDS_BRAVE_UI_SAVE_AS_FILE },
        { "seeAllItems", IDS_BRAVE_UI_SEE_ALL_ITEMS },
        { "seeAllSites", IDS_BRAVE_UI_SEE_ALL_SITES },
        { "sendDonation", IDS_BRAVE_UI_SEND_DONATION },
        { "sendTip", IDS_BRAVE_UI_SEND_TIP },
        { "settings", IDS_BRAVE_UI_SETTINGS },
        { "site", IDS_BRAVE_UI_SITE },
        { "sites", IDS_BRAVE_UI_SITES },
        { "tipOnLike", IDS_BRAVE_UI_TIP_ON_LIKE },
        { "titleBAT", IDS_BRAVE_UI_TITLE_BAT},
        { "titleBTC", IDS_BRAVE_UI_TITLE_BTC},
        { "titleETH", IDS_BRAVE_UI_TITLE_ETH},
        { "titleLTC", IDS_BRAVE_UI_TITLE_LTC},
        { "tokenGrantClaimed", IDS_BRAVE_UI_TOKEN_GRANT_CLAIMED },
        { "tokenGrantReceived", IDS_BRAVE_UI_TOKEN_GRANT_RECEIVED },
        { "token", IDS_BRAVE_UI_TOKEN },
        { "tokens", IDS_BRAVE_UI_TOKENS },
        { "tokenGrants", IDS_BRAVE_UI_TOKEN_GRANTS },
        { "pointGrants", IDS_BRAVE_UI_POINT_GRANTS },
        { "qrCodeLoading", IDS_BRAVE_UI_QR_CODE_LOADING },
        { "total", IDS_BRAVE_UI_TOTAL },
        { "transactions", IDS_BRAVE_UI_TRANSACTIONS },
        { "turnOnAds", IDS_BRAVE_UI_TURN_ON_ADS },
        { "turnOnRewardsDesc", IDS_BRAVE_UI_TURN_ON_REWARDS_DESC },
        { "turnOnRewardsTitle", IDS_BRAVE_UI_TURN_ON_REWARDS_TITLE },
        { "type", IDS_BRAVE_UI_TYPE },
        { "verifiedPublisher", IDS_BRAVE_UI_VERIFIED_PUBLISHER },
        { "viewDetails" , IDS_BRAVE_UI_VIEW_DETAILS },
        { "viewMonthly", IDS_BRAVE_UI_VIEW_MONTHLY },
        { "walletActivity", IDS_BRAVE_UI_WALLET_ACTIVITY },
        { "walletAddress", IDS_BRAVE_UI_WALLET_ADDRESS },
        { "walletBalance", IDS_BRAVE_UI_WALLET_BALANCE },
        { "walletButtonDisconnected", IDS_BRAVE_UI_WALLET_BUTTON_DISCONNECTED },
        { "walletButtonUnverified", IDS_BRAVE_UI_WALLET_BUTTON_UNVERIFIED },
        { "walletButtonVerified", IDS_BRAVE_UI_WALLET_BUTTON_VERIFIED },
        { "walletGoToProvider", IDS_BRAVE_UI_WALLET_GO_TO_PROVIDER },
        { "walletGoToVerifyPage", IDS_BRAVE_UI_WALLET_GO_TO_VERIFY_PAGE },
        { "walletDisconnect", IDS_BRAVE_UI_WALLET_DISCONNECT },
        { "walletVerificationButton", IDS_BRAVE_UI_WALLET_VERIFICATION_BUTTON },
        { "walletVerificationFooter", IDS_BRAVE_UI_WALLET_VERIFICATION_FOOTER },
        { "walletVerificationID", IDS_BRAVE_UI_WALLET_VERIFICATION_ID },
        { "walletVerificationList1", IDS_BRAVE_UI_WALLET_VERIFICATION_LIST1 },
        { "walletVerificationList2", IDS_BRAVE_UI_WALLET_VERIFICATION_LIST2 },
        { "walletVerificationList3", IDS_BRAVE_UI_WALLET_VERIFICATION_LIST3 },
        { "walletVerificationListHeader", IDS_BRAVE_UI_WALLET_VERIFICATION_HEADER },  // NOLINT
        { "walletVerificationNote1", IDS_BRAVE_UI_WALLET_VERIFICATION_NOTE1 },
        { "walletVerificationNote2", IDS_BRAVE_UI_WALLET_VERIFICATION_NOTE2 },
        { "walletVerificationNote3", IDS_BRAVE_UI_WALLET_VERIFICATION_NOTE3 },
        { "walletVerificationTitle1", IDS_BRAVE_UI_WALLET_VERIFICATION_TITLE1 },
        { "walletConnected", IDS_BRAVE_UI_WALLET_CONNECTED },
        { "walletPending", IDS_BRAVE_UI_WALLET_PENDING },
        { "walletVerified", IDS_BRAVE_UI_WALLET_VERIFIED },
        { "walletFailedTitle", IDS_BRAVE_UI_WALLET_FAILED_TITLE },
        { "walletFailedText", IDS_BRAVE_UI_WALLET_FAILED_TEXT },
        { "welcome", IDS_BRAVE_UI_WELCOME },
        { "welcomeButtonTextOne", IDS_BRAVE_UI_WELCOME_BUTTON_TEXT_ONE},
        { "welcomeButtonTextTwo", IDS_BRAVE_UI_WELCOME_BUTTON_TEXT_TWO},
        { "welcomeDescOne", IDS_BRAVE_UI_WELCOME_DESC_ONE},
        { "welcomeDescTwo", IDS_BRAVE_UI_WELCOME_DESC_TWO},
        { "welcomeDescPoints", IDS_BRAVE_UI_WELCOME_DESC_POINTS },
        { "welcomeFooterTextOne", IDS_BRAVE_UI_WELCOME_FOOTER_TEXT_ONE},
        { "welcomeFooterTextTwo", IDS_BRAVE_UI_WELCOME_FOOTER_TEXT_TWO},
        { "welcomeHeaderOne", IDS_BRAVE_UI_WELCOME_HEADER_ONE},
        { "welcomeHeaderTwo", IDS_BRAVE_UI_WELCOME_HEADER_TWO},
        { "whyBraveRewards", IDS_BRAVE_UI_WHY_BRAVE_REWARDS },
        { "yourBalance", IDS_BRAVE_UI_YOUR_BALANCE },
        { "yourWallet", IDS_BRAVE_UI_YOUR_WALLET },

        { "and", IDS_BRAVE_UI_AND },
        { "excludedSites", IDS_BRAVE_UI_EXCLUDED_SITES_TEXT },
        { "privacyPolicy", IDS_BRAVE_UI_PRIVACY_POLICY },
        { "restoreSite", IDS_BRAVE_UI_RESTORE_SITE },
        { "rewardsExcludedText1", IDS_BRAVE_UI_REWARDS_EXCLUDED_TEXT_1 },
        { "rewardsExcludedText2", IDS_BRAVE_UI_REWARDS_EXCLUDED_TEXT_2 },
        { "rewardsOffText5", IDS_BRAVE_UI_REWARDS_OFF_TEXT5 },
        { "serviceTextWelcome", IDS_BRAVE_UI_SERVICE_TEXT_WELCOME },
        { "serviceTextReady", IDS_BRAVE_UI_SERVICE_TEXT_READY },
        { "showAll", IDS_BRAVE_UI_SHOW_ALL },
        { "viewedSites", IDS_BRAVE_UI_VIEWED_SITES },
        { "termsOfService", IDS_BRAVE_UI_TERMS_OF_SERVICE },

        { "tapNetworkTitle", IDS_BRAVE_UI_TAP_NETWORK_TITLE },
        { "tapNetworkInfo", IDS_BRAVE_UI_TAP_NETWORK_INFO },
        { "tapNetworkDisclaimer", IDS_BRAVE_UI_TAP_NETWORK_DISCLAIMER },

        { "upholdPromoTitle", IDS_BRAVE_UI_UPHOLD_PROMO_TITLE },
        { "upholdPromoInfo", IDS_BRAVE_UI_UPHOLD_PROMO_INFO },

        { "upholdPromoEquitiesTitle",IDS_BRAVE_UI_UPHOLD_PROMO_EQUITIES_TITLE },                 // NOLINT
        { "upholdPromoEquitiesInfo", IDS_BRAVE_UI_UPHOLD_PROMO_EQUITIES_INFO },
      }
    }, {
      std::string("adblock"), {
        { "additionalFiltersTitle", IDS_ADBLOCK_ADDITIONAL_FILTERS_TITLE },
        { "additionalFiltersWarning", IDS_ADBLOCK_ADDITIONAL_FILTERS_WARNING },                  // NOLINT
        { "adsBlocked", IDS_ADBLOCK_TOTAL_ADS_BLOCKED },
        { "customFiltersTitle", IDS_ADBLOCK_CUSTOM_FILTERS_TITLE },
        { "customFiltersInstructions", IDS_ADBLOCK_CUSTOM_FILTERS_INSTRUCTIONS },                // NOLINT
      }
    }, {
#if BUILDFLAG(IPFS_ENABLED)
      std::string("ipfs"), {
        { "connectedPeersTitle", IDS_IPFS_CONNECTED_PEERS_TITLE },
        { "addressesConfigTitle", IDS_IPFS_ADDRESSES_CONFIG_TITLE },
        { "repoStatsTitle", IDS_IPFS_REPO_STATS_TITLE },
        { "daemonStatusTitle", IDS_IPFS_DAEMON_STATUS_TITLE },
        { "api", IDS_IPFS_API },
        { "gateway", IDS_IPFS_GATEWAY },
        { "swarm", IDS_IPFS_SWARM },
        { "objects", IDS_IPFS_REPO_OBJECTS },
        { "size", IDS_IPFS_REPO_SIZE },
        { "storage", IDS_IPFS_REPO_STORAGE },
        { "path", IDS_IPFS_REPO_PATH },
        { "version", IDS_IPFS_REPO_VERSION },
        { "launched", IDS_IPFS_LAUNCHED },
        { "notLaunched", IDS_IPFS_NOT_LAUNCHED },
        { "launch", IDS_IPFS_LAUNCH },
        { "shutdown", IDS_IPFS_SHUTDOWN },
        { "notInstalled", IDS_IPFS_NOT_INSTALLED },
        { "nodeInfoTitle", IDS_IPFS_NODE_INFO_TITLE },
        { "id", IDS_IPFS_NODE_INFO_ID },
        { "agentVersion", IDS_IPFS_NODE_INFO_VERSION },
        { "restart", IDS_IPFS_RESTART },
        { "learnMore", IDS_IPFS_LEARN_MORE },
        { "installAndLaunch", IDS_IPFS_INSTALL_AND_LAUNCH },
        { "openWebUI", IDS_IPFS_OPEN_WEBUI },
        { "peerDetailsLink", IDS_IPFS_PEERS_DETAILS_LINK },
        { "installing", IDS_IPFS_INSTALLING },
        { "runGarbageCollectionTitle", IDS_IPFS_RUN_GC_BUTTON },
        { "gcError", IDS_IPFS_GC_ERROR }
      }
    }, {
#endif
      std::string("tip"), {
        { "addFunds", IDS_BRAVE_UI_ADD_FUNDS },
        { "bap", IDS_BRAVE_UI_BAP_REWARDS_TEXT },
        { "bapFunds", IDS_BRAVE_REWARDS_TIP_BAP_FUNDS },
        { "bat", IDS_BRAVE_UI_BAT_TEXT },
        { "batFunds", IDS_BRAVE_REWARDS_TIP_BAT_FUNDS },
        { "cancel", IDS_BRAVE_REWARDS_TIP_CANCEL },
        { "cancelConfirmationText", IDS_BRAVE_REWARDS_TIP_CANCEL_CONFIRMATION_TEXT },  // NOLINT
        { "cancelMonthlyContribution", IDS_BRAVE_REWARDS_TIP_CANCEL_MONTHLY_CONTRIBUTION },  // NOLINT
        { "changeAmount", IDS_BRAVE_REWARDS_TIP_CHANGE_AMOUNT },
        { "confirmCancel", IDS_BRAVE_REWARDS_TIP_CONFIRM_CANCEL },
        { "contributionAmount", IDS_BRAVE_REWARDS_TIP_CONTRIBUTION_AMOUNT },  // NOLINT
        { "contributionCanceled", IDS_BRAVE_REWARDS_TIP_CONTRIBUTION_CANCELED },  // NOLINT
        { "currentlySupporting", IDS_BRAVE_REWARDS_TIP_CURRENTLY_SUPPORTING },
        { "currentMonthlyContribution", IDS_BRAVE_REWARDS_TIP_CURRENT_MONTHLY_CONTRIBUTION },  // NOLINT
        { "doMonthly", IDS_BRAVE_UI_DO_MONTHLY },
        { "errorHasOccurred", IDS_BRAVE_REWARDS_TIP_ERROR_HAS_OCCURRED },
        { "errorServerConnection", IDS_BRAVE_REWARDS_TIP_ERROR_SERVER_CONNECTION },  // NOLINT
        { "errorTryAgain", IDS_BRAVE_REWARDS_TIP_ERROR_TRY_AGAIN },
        { "monthlyContribution", IDS_BRAVE_UI_MONTHLY_CONTRIBUTION },
        { "monthlyContributionSet", IDS_BRAVE_REWARDS_TIP_MONTHLY_CONTRIBUTION_SET },  // NOLINT
        { "monthlyText", IDS_BRAVE_UI_MONTHLY_TEXT },
        { "nextContributionDate", IDS_BRAVE_REWARDS_TIP_NEXT_CONTRIBUTION_DATE },  // NOLINT
        { "notEnoughTokens", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS },
        { "notEnoughTokensLink", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS_LINK },
        { "on", IDS_BRAVE_UI_ON },
        { "onboardingMaybeLater", IDS_BRAVE_REWARDS_ONBOARDING_MAYBE_LATER },
        { "onboardingSetupAdsHeader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_HEADER },  // NOLINT
        { "onboardingSetupAdsSubheader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_SUBHEADER },  // NOLINT
        { "onboardingSetupContributeHeader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_CONTRIBUTE_HEADER },  // NOLINT
        { "onboardingSetupContributeSubheader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_CONTRIBUTE_SUBHEADER },  // NOLINT
        { "onboardingStartUsingRewards", IDS_BRAVE_REWARDS_ONBOARDING_START_USING_REWARDS },  // NOLINT
        { "onboardingTakeTour", IDS_BRAVE_REWARDS_ONBOARDING_TAKE_TOUR },
        { "onboardingTerms", IDS_BRAVE_REWARDS_ONBOARDING_TERMS },
        { "onboardingTipHeader", IDS_BRAVE_REWARDS_ONBOARDING_TIP_HEADER },
        { "onboardingTipText", IDS_BRAVE_REWARDS_ONBOARDING_TIP_TEXT },
        { "onboardingTourBack", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_BACK },
        { "onboardingTourBegin", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_BEGIN },
        { "onboardingTourContinue", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_CONTINUE },  // NOLINT
        { "onboardingTourDone", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_DONE },
        { "onboardingTourSkip", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_SKIP },
        { "onboardingTourSkipForNow", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_SKIP_FOR_NOW },  // NOLINT
        { "onboardingPanelWelcomeHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_WELCOME_HEADER },  // NOLINT
        { "onboardingPanelWelcomeText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_WELCOME_TEXT },  // NOLINT
        { "onboardingPanelAdsHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_ADS_HEADER },  // NOLINT
        { "onboardingPanelAdsText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_ADS_TEXT },  // NOLINT
        { "onboardingPanelScheduleHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SCHEDULE_HEADER },  // NOLINT
        { "onboardingPanelScheduleText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SCHEDULE_TEXT },  // NOLINT
        { "onboardingPanelAcHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_AC_HEADER },  // NOLINT
        { "onboardingPanelAcText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_AC_TEXT },
        { "onboardingPanelTippingHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_TIPPING_HEADER },  // NOLINT
        { "onboardingPanelTippingText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_TIPPING_TEXT },  // NOLINT
        { "onboardingPanelRedeemHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_REDEEM_HEADER },  // NOLINT
        { "onboardingPanelRedeemText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_REDEEM_TEXT },  // NOLINT
        { "onboardingPanelSetupHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SETUP_HEADER },  // NOLINT
        { "onboardingPanelSetupText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SETUP_TEXT },  // NOLINT
        { "onboardingPanelCompleteHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_COMPLETE_HEADER },  // NOLINT
        { "onboardingPanelCompleteText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_COMPLETE_TEXT },  // NOLINT
        { "oneTimeTip", IDS_BRAVE_REWARDS_TIP_ONE_TIME_TIP },
        { "oneTimeTipAmount", IDS_BRAVE_REWARDS_TIP_ONE_TIME_TIP_AMOUNT },
        { "optInRequired", IDS_BRAVE_REWARDS_TIP_OPT_IN_REQUIRED },
        { "points", IDS_BRAVE_UI_POINTS },
        { "postHeader", IDS_BRAVE_REWARDS_TIP_POST_HEADER },
        { "postHeaderTwitter", IDS_BRAVE_REWARDS_TIP_POST_HEADER_TWITTER },
        { "rewardsBannerText1", IDS_BRAVE_UI_REWARDS_BANNER_TEXT1 },
        { "sendDonation", IDS_BRAVE_UI_SEND_DONATION },
        { "siteBannerConnectedText", IDS_BRAVE_UI_SITE_BANNER_CONNECTED_TEXT },
        { "siteBannerNoticeNote", IDS_BRAVE_UI_SITE_BANNER_NOTICE_NOTE },
        { "siteBannerNoticeText", IDS_BRAVE_UI_SITE_BANNER_NOTICE_TEXT },
        { "sorryToSeeYouGo", IDS_BRAVE_REWARDS_TIP_SORRY_TO_SEE_YOU_GO },
        { "supportThisCreator", IDS_BRAVE_REWARDS_TIP_SUPPORT_THIS_CREATOR },
        { "termsOfService", IDS_BRAVE_REWARDS_TIP_TERMS_OF_SERVICE },
        { "thanksForTheSupport", IDS_BRAVE_REWARDS_TIP_THANKS_FOR_THE_SUPPORT },  // NOLINT
        { "tipHasBeenSent", IDS_BRAVE_REWARDS_TIP_TIP_HAS_BEEN_SET },
        { "tipPostSubtitle", IDS_BRAVE_REWARDS_TIP_TIP_POST_SUBTITLE },
        { "tokens", IDS_BRAVE_UI_TOKENS },
        { "tweetAboutSupport", IDS_BRAVE_REWARDS_TIP_TWEET_ABOUT_SUPPORT },
        { "unverifiedTextMore", IDS_BRAVE_UI_SITE_UNVERIFIED_TEXT_MORE },
        { "welcome", IDS_BRAVE_UI_WELCOME },
      }
    }, {
      std::string("rewards-internals"), {
        { "amount", IDS_BRAVE_REWARDS_INTERNALS_AMOUNT },
        { "autoRefresh", IDS_BRAVE_REWARDS_INTERNALS_AUTO_REFRESH },
        { "balanceInfo", IDS_BRAVE_REWARDS_INTERNALS_BALANCE_INFO },
        { "bat", IDS_BRAVE_UI_BAT_TEXT },
        { "bootStamp", IDS_BRAVE_REWARDS_INTERNALS_BOOT_STAMP },
        { "clearButton", IDS_BRAVE_REWARDS_INTERNALS_CLEAR_BUTTON },
        { "contributedAmount", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTED_AMOUNT },
        { "contributionCreatedAt", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTED_CREATED_AT },         // NOLINT
        { "contribution", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION },
        { "contributionProcessor", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_PROCESSOR },         // NOLINT
        { "contributionStep", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP },
        { "contributionStepAutoContributeTableEmpty", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_AUTO_CONTRIBUTE_TABLE_EMPTY },  // NOLINT
        { "contributionStepNotEnoughFunds", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_NOT_ENOUGH_FUNDS },  // NOLINT
        { "contributionStepFailed", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_FAILED },      // NOLINT
        { "contributionStepCompleted", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_COMPLETED },// NOLINT
        { "contributionStepNo", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_NO },              // NOLINT
        { "contributionStepStart", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_START },        // NOLINT
        { "contributionStepPrepare", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_PREPARE },    // NOLINT
        { "contributionStepReserve", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_RESERVE },    // NOLINT
        { "contributionStepExternalTransaction", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_EXTERNAL_TRANSACTION },  // NOLINT
        { "contributionStepCreds", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_CREDS },        // NOLINT
        { "contributionStepRewardsOff", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_REWARDS_OFF },        // NOLINT
        { "contributionStepAutoContributeOff", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_AUTO_CONTRIBUTE_OFF },        // NOLINT
        { "contributionStepRetryCount", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_STEP_RETRY_COUNT },        // NOLINT
        { "eventLogKey", IDS_BRAVE_REWARDS_INTERNALS_EVENT_LOG_KEY },
        { "eventLogValue", IDS_BRAVE_REWARDS_INTERNALS_EVENT_LOG_VALUE },
        { "eventLogTime", IDS_BRAVE_REWARDS_INTERNALS_EVENT_LOG_TIME },
        { "mainDisclaimer", IDS_BRAVE_REWARDS_INTERNALS_MAIN_DISCLAIMER },             // NOLINT
        { "rewardsTypeAuto", IDS_BRAVE_REWARDS_INTERNALS_REWARDS_TYPE_AUTO },                    // NOLINT
        { "rewardsTypeOneTimeTip", IDS_BRAVE_REWARDS_INTERNALS_REWARDS_TYPE_ONE_TIME_TIP },      // NOLINT
        { "rewardsTypeRecurringTip", IDS_BRAVE_REWARDS_INTERNALS_REWARDS_TYPE_RECURRING_TIP },   // NOLINT
        { "contributionType", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTION_TYPE },
        { "contributions", IDS_BRAVE_REWARDS_INTERNALS_CONTRIBUTIONS },
        { "downloadButton", IDS_BRAVE_REWARDS_INTERNALS_DOWNLOAD_BUTTON },
        { "externalWallet", IDS_BRAVE_REWARDS_INTERNALS_EXTERNAL_WALLET },
        { "invalid", IDS_BRAVE_REWARDS_INTERNALS_INVALID },
        { "keyInfoSeed", IDS_BRAVE_REWARDS_INTERNALS_KEY_INFO_SEED },
        { "logNotice", IDS_BRAVE_REWARDS_INTERNALS_LOG_NOTICE },
        { "mainTitle", IDS_BRAVE_REWARDS_INTERNALS_MAIN_TITLE },
        { "personaId", IDS_BRAVE_REWARDS_INTERNALS_PERSONA_ID },
        { "processorBraveTokens", IDS_BRAVE_UI_PROCESSOR_BRAVE_TOKENS },
        { "processorUphold", IDS_BRAVE_UI_PROCESSOR_UPHOLD },
        { "processorBitflyer", IDS_BRAVE_UI_PROCESSOR_BITFLYER },
        { "processorBraveUserFunds", IDS_BRAVE_UI_PROCESSOR_BRAVE_USER_FUNDS },
        { "promotionAds", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_ADS },
        { "promotionAmount", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_AMOUNT },
        { "promotionClaimedAt", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_CLAIMED_AT },              // NOLINT
        { "promotionClaimId", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_CLAIM_ID },
        { "promotionExpiresAt", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_EXPIRES_AT },              // NOLINT
        { "promotionId", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_ID },
        { "promotionLegacyClaimed", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_LEGACY_CLAIMED },      // NOLINT
        { "promotionLegacyNo", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_LEGACY_NO },                // NOLINT
        { "promotionLegacyYes", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_LEGACY_YES },              // NOLINT
        { "promotions", IDS_BRAVE_REWARDS_INTERNALS_PROMOTIONS },
        { "promotionStatus", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_STATUS },
        { "promotionStatusActive", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_STATUS_ACTIVE },        // NOLINT
        { "promotionStatusAttested", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_STATUS_ATTESTED },    // NOLINT
        { "promotionStatusFinished", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_STATUS_FINISHED },    // NOLINT
        { "promotionStatusOver", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_STATUS_OVER },            // NOLINT
        { "promotionType", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_TYPE },
        { "promotionUGP", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_UGP },
        { "promotionVersion", IDS_BRAVE_REWARDS_INTERNALS_PROMOTION_VERSION },
        { "refreshButton", IDS_BRAVE_REWARDS_INTERNALS_REFRESH_BUTTON },
        { "retryCount", IDS_BRAVE_REWARDS_INTERNALS_RETRY_COUNT },
        { "tabGeneralInfo", IDS_BRAVE_REWARDS_INTERNALS_TAB_GENERAL_INFO },
        { "tabLogs", IDS_BRAVE_REWARDS_INTERNALS_TAB_LOGS },
        { "tabPromotions", IDS_BRAVE_REWARDS_INTERNALS_TAB_PROMOTIONS },
        { "tabContributions", IDS_BRAVE_REWARDS_INTERNALS_TAB_CONTRIBUTIONS },
        { "tabEventLogs", IDS_BRAVE_REWARDS_INTERNALS_TAB_EVENT_LOGS },
        { "totalAmount", IDS_BRAVE_REWARDS_INTERNALS_TOTAL_AMOUNT },
        { "totalBalance", IDS_BRAVE_REWARDS_INTERNALS_TOTAL_BALANCE },
        { "userId", IDS_BRAVE_REWARDS_INTERNALS_USER_ID },
        { "valid", IDS_BRAVE_REWARDS_INTERNALS_VALID },
        { "walletAddress", IDS_BRAVE_REWARDS_INTERNALS_WALLET_ADDRESS },
        { "walletInfo", IDS_BRAVE_REWARDS_INTERNALS_WALLET_INFO },
        { "walletNotCreated", IDS_BRAVE_REWARDS_INTERNALS_WALLET_NOT_CREATED },
        { "walletPaymentId", IDS_BRAVE_REWARDS_INTERNALS_WALLET_PAYMENT_ID },
        { "walletStatus", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS },
        { "walletStatusConnected", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS_CONNECTED },    // NOLINT
        { "walletStatusNotConnected", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS_NOT_CONNECTED },    // NOLINT
        { "walletStatusVerified", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS_VERIFIED },    // NOLINT
        { "walletStatusDisconnectedNotVerified", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS_DISCONNECTED_NOT_VERIFIED },    // NOLINT
        { "walletStatusDisconnectedVerified", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS_DISCONNECTED_VERIFIED },    // NOLINT
        { "walletStatusPending", IDS_BRAVE_REWARDS_INTERNALS_WALLET_STATUS_PENDING },    // NOLINT
      }
    }, {
#if BUILDFLAG(IPFS_ENABLED)
      std::string("tor-internals"), {
        { "tabGeneralInfo", IDS_TOR_INTERNALS_TAB_GENERAL_INFO },
        { "tabLogs", IDS_TOR_INTERNALS_TAB_LOGS },
        { "torControlEvents", IDS_TOR_INTERNALS_TOR_CONTROL_EVENTS },
        { "torVersion", IDS_TOR_INTERNALS_TOR_VERSION },
        { "torPid", IDS_TOR_INTERNALS_TOR_PID },
        { "torProxyURI", IDS_TOR_INTERNALS_TOR_PROXY_URI },
        { "torConnectionStatus", IDS_TOR_INTERNALS_TOR_CONNECTION_STATUS },
        { "torInitProgress", IDS_TOR_INTERNALS_TOR_INIT_PROGRESS },
      }
    }, {
#endif
      std::string("webcompat"), {
        // Report modal
        { "reportModalTitle", IDS_BRAVE_WEBCOMPATREPORTER_REPORT_MODAL_TITLE },
        { "reportExplanation", IDS_BRAVE_WEBCOMPATREPORTER_REPORT_EXPLANATION },
        { "reportDisclaimer", IDS_BRAVE_WEBCOMPATREPORTER_REPORT_DISCLAIMER },
        { "cancel", IDS_BRAVE_WEBCOMPATREPORTER_CANCEL },
        { "submit", IDS_BRAVE_WEBCOMPATREPORTER_SUBMIT },
        // Confirmation modal
        { "thankYou", IDS_BRAVE_WEBCOMPATREPORTER_THANK_YOU },
        { "confirmationNotice",
            IDS_BRAVE_WEBCOMPATREPORTER_CONFIRMATION_NOTICE },
      }
    }
  };
  // clang-format on
  AddLocalizedStringsBulk(source, localized_strings[name]);
}  // NOLINT(readability/fn_size)

content::WebUIDataSource* CreateWebUIDataSource(
    const std::string& name,
    const GritResourceMap* resource_map,
    size_t resource_map_size,
    int html_resource_id,
    bool disable_trusted_types_csp) {
  content::WebUIDataSource* source = content::WebUIDataSource::Create(name);
  // Some parts of Brave's UI pages are not yet migrated to work without doing
  // assignments of strings directly into |innerHTML| elements (i.e. see usage
  // of |dangerouslySetInnerHTML| in .tsx files). This will break Brave due to
  // committing a Trusted Types related violation now that Trusted Types are
  // enforced on WebUI pages (see crrev.com/c/2234238 and crrev.com/c/2353547).
  // We should migrate those pages not to require using |innerHTML|, but for now
  // we just restore pre-Cromium 87 behaviour for pages that are not ready yet.
  if (disable_trusted_types_csp) {
    source->DisableTrustedTypesCSP();
  } else {
    // Allow a policy to be created so that we
    // can allow trusted HTML and trusted lazy-load script sources.
    source->OverrideContentSecurityPolicy(
        network::mojom::CSPDirectiveName::TrustedTypes, "default");
  }

  source->UseStringsJs();
  source->SetDefaultResource(html_resource_id);
  // Add generated resource paths
  for (size_t i = 0; i < resource_map_size; ++i) {
    source->AddResourcePath(resource_map[i].name, resource_map[i].value);
  }
  CustomizeWebUIHTMLSource(name, source);
  return source;
}

}  // namespace

content::WebUIDataSource* CreateAndAddWebUIDataSource(
    content::WebUI* web_ui,
    const std::string& name,
    const GritResourceMap* resource_map,
    size_t resource_map_size,
    int html_resource_id,
    bool disable_trusted_types_csp) {
  content::WebUIDataSource* data_source =
      CreateWebUIDataSource(name, resource_map, resource_map_size,
                            html_resource_id, disable_trusted_types_csp);
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui), data_source);
  return data_source;
}
