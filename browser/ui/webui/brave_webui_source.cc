/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_webui_source.h"

#include <map>
#include <vector>

#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_ui_data_source.h"

#if !defined(OS_ANDROID)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "components/grit/brave_components_resources.h"
#else
#include "components/grit/components_resources.h"
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

}  // namespace

void CustomizeWebUIHTMLSource(const std::string &name,
    content::WebUIDataSource* source) {
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
        { "ntp-1.webp", IDR_BRAVE_NEW_TAB_BACKGROUND1 },
        { "ntp-2.webp", IDR_BRAVE_NEW_TAB_BACKGROUND2 },
        { "ntp-3.webp", IDR_BRAVE_NEW_TAB_BACKGROUND3 },
        { "ntp-4.webp", IDR_BRAVE_NEW_TAB_BACKGROUND4 },
        { "ntp-5.webp", IDR_BRAVE_NEW_TAB_BACKGROUND5 },
        { "ntp-6.webp", IDR_BRAVE_NEW_TAB_BACKGROUND6 },
        { "ntp-7.webp", IDR_BRAVE_NEW_TAB_BACKGROUND7 },
        { "ntp-8.webp", IDR_BRAVE_NEW_TAB_BACKGROUND8 },
        { "ntp-9.webp", IDR_BRAVE_NEW_TAB_BACKGROUND9 },
        { "ntp-10.webp", IDR_BRAVE_NEW_TAB_BACKGROUND10 },
        { "ntp-11.webp", IDR_BRAVE_NEW_TAB_BACKGROUND11 },
        { "ntp-12.webp", IDR_BRAVE_NEW_TAB_BACKGROUND12 },
        { "ntp-13.webp", IDR_BRAVE_NEW_TAB_BACKGROUND13 },
        { "ntp-14.webp", IDR_BRAVE_NEW_TAB_BACKGROUND14 },
        { "ntp-15.webp", IDR_BRAVE_NEW_TAB_BACKGROUND15 },
        { "ntp-16.webp", IDR_BRAVE_NEW_TAB_BACKGROUND16 },

        // private tab
        { "c168145d6bf1abf2c0322636366f7dbe.svg", IDR_BRAVE_PRIVATE_TAB_TOR_IMG },               // NOLINT
        { "dbdc336ccc651b8a7c925b3482d6e65a.svg", IDR_BRAVE_PRIVATE_TAB_IMG }
      }
    }, {
      std::string("rewards"), {
        { "favicon.ico", IDR_BRAVE_REWARDS_FAVICON },
        { "4fcfa7f92c5fc22c2b6f34701bfdcd0a.jpeg", IDR_BRAVE_REWARDS_IMG_BART_TEMP },            // NOLINT
        { "6dd79d472f9c73429b26dae4ef14575e.svg", IDR_BRAVE_REWARDS_IMG_WALLET_BG },             // NOLINT
        { "c9255cc2aa3d81ca6328e82d25a95766.png", IDR_BRAVE_REWARDS_IMG_CAPTCHA_BAT },           // NOLINT
        { "1bb9aa85741c6d1c077f043324aae835.svg", IDR_BRAVE_REWARDS_IMG_WELCOME_BG },            // NOLINT
        { "88eeadb981d67d5e096afb9b8fe26df7.svg", IDR_BRAVE_REWARDS_IMG_BAT },
        { "87186eec176189163ce037bcc7676f2a.svg", IDR_BRAVE_REWARDS_IMG_BTC },
        { "7d9f0ededf215a4702ae5c457f7779ae.svg", IDR_BRAVE_REWARDS_IMG_ETH },
        { "2c6f798a519beabb327149c349912f5f.svg", IDR_BRAVE_REWARDS_IMG_LTC },
      }
    }, {
      std::string("welcome"), {
        { "favicon.ico", IDR_BRAVE_WELCOME_PAGE_FAVICON }
      }
    }, {
      std::string("adblock"), {
      }
    }, {
      std::string("donate"), {
        { "2e7994eaf768ee4a99272ea96cb39849.svg", IDR_BRAVE_DONATE_BG_1 },
        { "4364e454dba7ea966b117f643832e871.svg", IDR_BRAVE_DONATE_BG_2 },
      }
    }
  };
  AddResourcePaths(source, resources[name]);

  static std::map<std::string, std::vector<WebUISimpleItem> >
                                                           localized_strings = {
    {
      std::string("newtab"), {
        { "adsBlocked", IDS_BRAVE_NEW_TAB_TOTAL_ADS_BLOCKED },
        { "trackersBlocked", IDS_BRAVE_NEW_TAB_TOTAL_TRACKERS_BLOCKED },
        { "httpsUpgraded", IDS_BRAVE_NEW_TAB_TOTAL_HTTPS_UPGRADES },
        { "estimatedTimeSaved", IDS_BRAVE_NEW_TAB_TOTAL_TIME_SAVED },
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
        { "photoBy", IDS_BRAVE_NEW_TAB_PHOTO_BY },
        { "preferencesPageTitle", IDS_BRAVE_NEW_TAB_PREFERENCES_PAGE_TITLE },
        { "bookmarksPageTitle", IDS_BRAVE_NEW_TAB_BOOKMARKS_PAGE_TITLE },
        { "historyPageTitle", IDS_BRAVE_NEW_TAB_HISTORY_PAGE_TITLE },

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
        { "boxTorButton", IDS_BRAVE_PRIVATE_NEW_TAB_BOX_TOR_BUTTON }
      }
    }, {
      std::string("welcome"), {
        { "welcome", IDS_BRAVE_WELCOME_PAGE_MAIN_TITLE },
        { "whatIsBrave", IDS_BRAVE_WELCOME_PAGE_MAIN_DESC },
        { "letsGo", IDS_BRAVE_WELCOME_PAGE_MAIN_BUTTON },
        { "enableBraveRewards", IDS_BRAVE_WELCOME_PAGE_REWARDS_TITLE },
        { "setupBraveRewards", IDS_BRAVE_WELCOME_PAGE_REWARDS_DESC },
        { "enableRewards", IDS_BRAVE_WELCOME_PAGE_REWARDS_BUTTON },
        { "importFromAnotherBrowser", IDS_BRAVE_WELCOME_PAGE_IMPORT_TITLE },
        { "setupImport", IDS_BRAVE_WELCOME_PAGE_IMPORT_DESC },
        { "import", IDS_BRAVE_WELCOME_PAGE_IMPORT_BUTTON },
        { "manageShields", IDS_BRAVE_WELCOME_PAGE_SHIELDS_TITLE },
        { "adjustProtectionLevel", IDS_BRAVE_WELCOME_PAGE_SHIELDS_DESC },
        { "shieldSettings", IDS_BRAVE_WELCOME_PAGE_SHIELDS_BUTTON },
        { "setDefaultSearchEngine", IDS_BRAVE_WELCOME_PAGE_SEARCH_TITLE },
        { "chooseSearchEngine", IDS_BRAVE_WELCOME_PAGE_SEARCH_DESC },
        { "search", IDS_BRAVE_WELCOME_PAGE_SEARCH_BUTTON },
        { "findToolbarTheme", IDS_BRAVE_WELCOME_PAGE_THEME_TITLE },
        { "chooseTheme", IDS_BRAVE_WELCOME_PAGE_THEME_DESC },
        { "theme", IDS_BRAVE_WELCOME_PAGE_THEME_BUTTON },
        { "skipWelcomeTour", IDS_BRAVE_WELCOME_PAGE_SKIP_BUTTON },
        { "next", IDS_BRAVE_WELCOME_PAGE_NEXT_BUTTON },
        { "done", IDS_BRAVE_WELCOME_PAGE_DONE_BUTTON },
      }
    }, {
      std::string("rewards"), {
        { "adsCurrentEarnings",  IDS_BRAVE_REWARDS_LOCAL_ADS_CURRENT_EARNINGS },
        { "adsDesc",  IDS_BRAVE_REWARDS_LOCAL_ADS_DESC },
        { "adsDisabledTextOne",  IDS_BRAVE_REWARDS_LOCAL_ADS_DISABLED_TEXT_ONE },                // NOLINT
        { "adsDisabledTextTwo",  IDS_BRAVE_REWARDS_LOCAL_ADS_DISABLED_TEXT_TWO },                // NOLINT
        { "adsNotificationsReceived",  IDS_BRAVE_REWARDS_LOCAL_ADS_NOTIFICATIONS_RECEIVED },     // NOLINT
        { "adsNotSupported", IDS_BRAVE_REWARDS_LOCAL_ADS_NOT_SUPPORTED },
        { "adsPaymentDate",  IDS_BRAVE_REWARDS_LOCAL_ADS_PAYMENT_DATE },
        { "adsPagesViewed",  IDS_BRAVE_REWARDS_LOCAL_ADS_PAGES_VIEWED },
        { "adsPerHour",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR },
        { "adsPerHour1",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_1 },
        { "adsPerHour2",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_2 },
        { "adsPerHour3",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_3 },
        { "adsPerHour4",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_4 },
        { "adsPerHour5",  IDS_BRAVE_REWARDS_LOCAL_ADS_PER_HOUR_5 },
        { "adsTitle",  IDS_BRAVE_REWARDS_LOCAL_ADS_TITLE },

        { "bat", IDS_BRAVE_UI_BAT_REWARDS_TEXT },
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
        { "contributionAllowed",  IDS_BRAVE_REWARDS_LOCAL_CONTR_ALLOWED },
        { "contributionNonVerified",  IDS_BRAVE_REWARDS_LOCAL_CONTR_ALLOW_NON_VERIFIED },        // NOLINT
        { "contributionVideos",  IDS_BRAVE_REWARDS_LOCAL_CONTR_ALLOW_VIDEOS },
        { "contributionVisit1",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_1 },
        { "contributionVisit5",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_5 },
        { "contributionVisit10",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_10 },
        { "contributionTime5",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_5 },
        { "contributionTime8",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_8 },
        { "contributionTime60",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TIME_60 },

        { "deviceOffline", IDS_BRAVE_REWARDS_LOCAL_DEVICE_OFFLINE },
        { "donationTitle",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TITLE },
        { "donationDesc",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DESC },
        { "donationTotalDonations",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TOTAL_DONATIONS },            // NOLINT
        { "donationVisitSome",  IDS_BRAVE_REWARDS_LOCAL_DONAT_VISIT_SOME },
        { "donationAbility",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY },
        { "donationAbilityYT",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY_YT },
        { "donationAbilityTwitter",  IDS_BRAVE_REWARDS_LOCAL_DONAT_ABILITY_TW },
        { "donationDisabledText1",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DISABLED_TEXT1 },              // NOLINT
        { "donationDisabledText2",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DISABLED_TEXT2 },              // NOLINT
        { "donationNextDate",  IDS_BRAVE_REWARDS_LOCAL_DONAT_NEXT_DATE },

        { "panelAddFunds",  IDS_BRAVE_REWARDS_LOCAL_PANEL_ADD_FUNDS },
        { "panelWithdrawFunds",  IDS_BRAVE_REWARDS_LOCAL_PANEL_WITHDRAW_FUNDS },
        { "tokens",  IDS_BRAVE_REWARDS_LOCAL_TOKENS },
        { "walletRecoverySuccess",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RECOVERY_SUCCESS },           // NOLINT
        { "walletRestored",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RESTORED },
        { "walletRecoveryFail",  IDS_BRAVE_REWARDS_LOCAL_WALLET_RECOVERY_FAIL },                 // NOLINT
        { "almostThere",  IDS_BRAVE_REWARDS_LOCAL_ALMOST_THERE },
        { "notQuite",  IDS_BRAVE_REWARDS_LOCAL_NOT_QUITE },
        { "proveHuman",  IDS_BRAVE_REWARDS_LOCAL_PROVE_HUMAN },
        { "serverNotResponding",  IDS_BRAVE_REWARDS_LOCAL_SERVER_NOT_RESPONDING },               // NOLINT
        { "uhOh",  IDS_BRAVE_REWARDS_LOCAL_UH_OH },
        { "grantGoneTitle",  IDS_BRAVE_REWARDS_LOCAL_GRANT_GONE_TITLE },
        { "grantGoneButton",  IDS_BRAVE_REWARDS_LOCAL_GRANT_GONE_BUTTON },
        { "grantGoneText",  IDS_BRAVE_REWARDS_LOCAL_GRANT_GONE_TEXT },
        { "grantGeneralErrorTitle",  IDS_BRAVE_REWARDS_LOCAL_GENERAL_GRANT_ERROR_TITLE },        // NOLINT
        { "grantGeneralErrorButton",  IDS_BRAVE_REWARDS_LOCAL_GENERAL_GRANT_ERROR_BUTTON },      // NOLINT
        { "grantGeneralErrorText",  IDS_BRAVE_REWARDS_LOCAL_GENERAL_GRANT_ERROR_TEXT },          // NOLINT
        { "walletCorrupted",  IDS_BRAVE_REWARDS_LOCAL_WALLET_CORRUPTED },
        { "walletCorruptedNow",  IDS_BRAVE_REWARDS_LOCAL_WALLET_CORRUPTED_NOW },
        { "grantAlreadyClaimedText", IDS_BRAVE_REWARDS_LOCAL_GRANT_ALREADY_CLAIMED_TEXT },       // NOLINT

        { "about", IDS_BRAVE_UI_ABOUT },
        { "accept", IDS_BRAVE_UI_ACCEPT },
        { "activityCopy", IDS_BRAVE_UI_ACTIVITY_COPY },
        { "activityNote", IDS_BRAVE_UI_ACTIVITY_NOTE },
        { "addFunds", IDS_BRAVE_UI_ADD_FUNDS },
        { "addFundsFAQ", IDS_BRAVE_UI_ADD_FUNDS_FAQ},
        { "addFundsNote", IDS_BRAVE_UI_ADD_FUNDS_NOTE},
        { "addFundsQR", IDS_BRAVE_UI_ADD_FUNDS_QR},
        { "addFundsText", IDS_BRAVE_UI_ADD_FUNDS_TEXT},
        { "addFundsTitle", IDS_BRAVE_UI_ADD_FUNDS_TITLE},
        { "allowTip", IDS_BRAVE_UI_ALLOW_TIP },
        { "amount", IDS_BRAVE_UI_AMOUNT },
        { "backup", IDS_BRAVE_UI_BACKUP },
        { "braveAdsDesc", IDS_BRAVE_UI_BRAVE_ADS_DESC },
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
        { "captchaLuckyDay", IDS_BRAVE_UI_CAPTCHA_LUCKY_DAY },
        { "captchaOnTheWay", IDS_BRAVE_UI_CAPTCHA_ON_THE_WAY },
        { "captchaProveHuman", IDS_BRAVE_UI_CAPTCHA_PROVE_HUMAN },
        { "captchaTarget", IDS_BRAVE_UI_CAPTCHA_TARGET },
        { "captchaMissedTarget", IDS_BRAVE_UI_CAPTCHA_MISSED_TARGET },
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
        { "grantExpire", IDS_BRAVE_UI_GRANT_EXPIRE },
        { "grants", IDS_BRAVE_UI_GRANTS },
        { "import", IDS_BRAVE_UI_IMPORT },
        { "includeInAuto", IDS_BRAVE_UI_INCLUDE_IN_AUTO },
        { "learnMore", IDS_BRAVE_UI_LEARN_MORE },
        { "makeMonthly", IDS_BRAVE_UI_MAKE_MONTHLY },
        { "manageWallet", IDS_BRAVE_UI_MANAGE_WALLET },
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
        { "newGrant", IDS_BRAVE_UI_NEW_GRANT },
        { "newTokenGrant", IDS_BRAVE_UI_NEW_TOKEN_GRANT },
        { "noActivity", IDS_BRAVE_UI_NO_ACTIVITY },
        { "noGrants", IDS_BRAVE_UI_NO_GRANTS },
        { "notEnoughTokens", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS },
        { "noThankYou", IDS_BRAVE_UI_NO_THANK_YOU },
        { "off", IDS_BRAVE_UI_OFF },
        { "ok", IDS_BRAVE_UI_OK },
        { "on", IDS_BRAVE_UI_ON },
        { "oneTime", IDS_BRAVE_UI_ONE_TIME },
        { "oneTimeDonation", IDS_BRAVE_UI_ONE_TIME_DONATION },
        { "openBalance", IDS_BRAVE_UI_OPEN_BALANCE },
        { "payment", IDS_BRAVE_UI_PAYMENT },
        { "paymentMonthly", IDS_BRAVE_UI_PAYMENT_MONTHLY },
        { "paymentNotMade", IDS_BRAVE_UI_PAYMENT_NOT_MADE },
        { "paymentWarning", IDS_BRAVE_UI_PAYMENT_WARNING },
        { "pinnedSitesHeader", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_HEADER },
        { "pinnedSitesMsg", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_MSG },
        { "pinnedSitesOne", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_ONE },
        { "pinnedSitesTwo", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_TWO },
        { "pinnedSitesThree", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_THREE },
        { "pinnedSitesFour", IDS_BRAVE_UI_PAYMENT_PINNED_SITES_FOUR },
        { "pleaseNote", IDS_BRAVE_UI_PLEASE_NOTE },
        { "print", IDS_BRAVE_UI_PRINT },
        { "readyToTakePart", IDS_BRAVE_UI_READY_TO_TAKE_PART },
        { "readyToTakePartOptInText", IDS_BRAVE_UI_READY_TO_TAKE_PART_OPT_IN_TEXT },             // NOLINT
        { "readyToTakePartStart", IDS_BRAVE_UI_READY_TO_TAKE_PART_START },
        { "recoveryKeys", IDS_BRAVE_UI_RECOVERY_KEYS },
        { "recurring", IDS_BRAVE_UI_RECURRING },
        { "recurringDonation", IDS_BRAVE_UI_RECURRING_DONATION },
        { "recurringDonations", IDS_BRAVE_UI_RECURRING_DONATIONS },
        { "remove", IDS_BRAVE_UI_REMOVE },
        { "reservedAmountText", IDS_BRAVE_UI_RESERVED_AMOUNT_TEXT },
        { "reservedMoreLink", IDS_BRAVE_UI_RESERVED_MORE_LINK },
        { "restore", IDS_BRAVE_UI_RESTORE },
        { "restoreAll", IDS_BRAVE_UI_RESTORE_ALL },
        { "reviewSitesMsg", IDS_BRAVE_UI_REVIEW_SITE_MSG },
        { "rewardsBackupText1", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT1 },
        { "rewardsBackupText2", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT2 },
        { "rewardsBackupText3", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT3 },
        { "rewardsBannerText1", IDS_BRAVE_UI_REWARDS_BANNER_TEXT1 },
        { "rewardsBannerText2", IDS_BRAVE_UI_REWARDS_BANNER_TEXT2 },
        { "rewardsContribute", IDS_BRAVE_UI_REWARDS_CONTRIBUTE },
        { "rewardsContributeAttention", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_ATTENTION },             // NOLINT
        { "rewardsContributeAttentionScore", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_ATTENTION_SCORE },  // NOLINT
        { "rewardsContributeText1", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_TEXT1 },
        { "rewardsOffText1", IDS_BRAVE_UI_REWARDS_OFF_TEXT1 },
        { "rewardsOffText2", IDS_BRAVE_UI_REWARDS_OFF_TEXT2 },
        { "rewardsOffText3", IDS_BRAVE_UI_REWARDS_OFF_TEXT3 },
        { "rewardsOffText4", IDS_BRAVE_UI_REWARDS_OFF_TEXT4 },
        { "rewardsPanelEmptyText1", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT1 },
        { "rewardsPanelEmptyText2", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT2 },
        { "rewardsPanelEmptyText3", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT3 },
        { "rewardsPanelEmptyText4", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT4 },
        { "rewardsPanelEmptyText5", IDS_BRAVE_UI_REWARDS_PANEL_EMPTY_TEXT5 },
        { "rewardsPanelOffText1", IDS_BRAVE_UI_REWARDS_PANEL_OFF_TEXT1 },
        { "rewardsPanelOffText2", IDS_BRAVE_UI_REWARDS_PANEL_OFF_TEXT2 },
        { "rewardsPanelText1", IDS_BRAVE_UI_REWARDS_PANEL_TEXT1 },
        { "rewardsPanelText2", IDS_BRAVE_UI_REWARDS_PANEL_TEXT2 },
        { "rewardsPanelText3", IDS_BRAVE_UI_REWARDS_PANEL_TEXT3 },
        { "rewardsPanelText4", IDS_BRAVE_UI_REWARDS_PANEL_TEXT4 },
        { "rewardsRestoreText1", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT1 },
        { "rewardsRestoreText2", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT2 },
        { "rewardsRestoreText3", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT3 },
        { "rewardsRestoreText4", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT4 },
        { "rewardsSummary", IDS_BRAVE_UI_REWARDS_SUMMARY },
        { "rewardsWhy", IDS_BRAVE_UI_REWARDS_WHY },
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
        { "tokens", IDS_BRAVE_UI_TOKENS },
        { "total", IDS_BRAVE_UI_TOTAL },
        { "transactions", IDS_BRAVE_UI_TRANSACTIONS },
        { "turnOnRewardsDesc", IDS_BRAVE_UI_TURN_ON_REWARDS_DESC },
        { "turnOnRewardsTitle", IDS_BRAVE_UI_TURN_ON_REWARDS_TITLE },
        { "type", IDS_BRAVE_UI_TYPE },
        { "verifiedPublisher", IDS_BRAVE_UI_VERIFIED_PUBLISHER },
        { "viewMonthly", IDS_BRAVE_UI_VIEW_MONTHLY },
        { "walletActivity", IDS_BRAVE_UI_WALLET_ACTIVITY },
        { "walletAddress", IDS_BRAVE_UI_WALLET_ADDRESS },
        { "walletBalance", IDS_BRAVE_UI_WALLET_BALANCE },
        { "walletFailedButton", IDS_BRAVE_UI_WALLET_FAILED_BUTTON },
        { "walletFailedTitle", IDS_BRAVE_UI_WALLET_FAILED_TITLE },
        { "walletFailedText", IDS_BRAVE_UI_WALLET_FAILED_TEXT },
        { "welcome", IDS_BRAVE_UI_WELCOME },
        { "welcomeButtonTextOne", IDS_BRAVE_UI_WELCOME_BUTTON_TEXT_ONE},
        { "welcomeButtonTextTwo", IDS_BRAVE_UI_WELCOME_BUTTON_TEXT_TWO},
        { "welcomeDescOne", IDS_BRAVE_UI_WELCOME_DESC_ONE},
        { "welcomeDescTwo", IDS_BRAVE_UI_WELCOME_DESC_TWO},
        { "welcomeFooterTextOne", IDS_BRAVE_UI_WELCOME_FOOTER_TEXT_ONE},
        { "welcomeFooterTextTwo", IDS_BRAVE_UI_WELCOME_FOOTER_TEXT_TWO},
        { "welcomeHeaderOne", IDS_BRAVE_UI_WELCOME_HEADER_ONE},
        { "welcomeHeaderTwo", IDS_BRAVE_UI_WELCOME_HEADER_TWO},
        { "whyBraveRewards", IDS_BRAVE_UI_WHY_BRAVE_REWARDS },
        { "whyBraveRewardsDesc1", IDS_BRAVE_UI_WHY_BRAVE_REWARDS_DESC_1 },
        { "whyBraveRewardsDesc2", IDS_BRAVE_UI_WHY_BRAVE_REWARDS_DESC_2 },
        { "yourWallet", IDS_BRAVE_UI_YOUR_WALLET }
      }
    }, {
      std::string("sync"), {
        // Shared strings
        { "remove", IDS_BRAVE_SYNC_SHARED_REMOVE_PARTIAL },
        { "copied", IDS_BRAVE_SYNC_SHARED_COPIED_TEXT },
        { "wordCount",  IDS_BRAVE_SYNC_SHARED_WORD_COUNT_TEXT },
        { "ok", IDS_BRAVE_SYNC_SHARED_OK_BUTTON },
        { "cancel", IDS_BRAVE_SYNC_SHARED_CANCEL_BUTTON },
        { "cancelDeviceSyncing", IDS_BRAVE_SYNC_SHARED_CANCEL_SYNCING_TITLE },
        { "cancelDeviceSyncingButton", IDS_BRAVE_SYNC_SHARED_CANCEL_SYNCING_BUTTON },            // NOLINT
        { "thisSyncChain", IDS_BRAVE_SYNC_SHARED_FROM_THIS_CHAIN_PARTIAL },
        { "lookingForDevice", IDS_BRAVE_SYNC_SCAN_CODE_LOOKING_FOR_DEVICE_BUTTON },              // NOLINT
        { "viewSyncCode", IDS_BRAVE_SYNC_ENABLED_VIEW_CODE_BUTTON },
        // Enabled Content
        { "braveSync", IDS_BRAVE_SYNC_ENABLED_BRAVE_TITLE },
        { "syncChainDevices", IDS_BRAVE_SYNC_ENABLED_DEVICES_CHAIN_TITLE },
        { "deviceName", IDS_BRAVE_SYNC_ENABLED_TABLE_DEVICE_NAME_TITLE },
        { "thisDevice", IDS_BRAVE_SYNC_ENABLED_TABLE_THIS_DEVICE_TEXT },
        { "addedOn", IDS_BRAVE_SYNC_ENABLED_TABLE_ADDED_ON_TITLE },
        { "addDevice", IDS_BRAVE_SYNC_ENABLED_ADD_DEVICE_BUTTON },
        { "settingsTitle", IDS_BRAVE_SYNC_ENABLED_SETTINGS_TITLE },
        { "settingsDescription", IDS_BRAVE_SYNC_ENABLED_SETTINGS_DESCRIPTION },
        { "settings", IDS_BRAVE_SYNC_ENABLED_TABLE_SETTINGS },
        { "bookmarks", IDS_BRAVE_SYNC_ENABLED_BOOKMARKS_LABEL },
        { "savedSiteSettings", IDS_BRAVE_SYNC_ENABLED_SITE_SETTINGS_LABEL },
        { "browsingHistory", IDS_BRAVE_SYNC_ENABLED_HISTORY_LABEL },
        { "leaveSyncChain", IDS_BRAVE_SYNC_ENABLED_LEAVE_CHAIN_BUTTON },
        // Disabled Content
        { "syncTitle", IDS_BRAVE_SYNC_DISABLED_DESCRIPTION },
        { "syncDescription", IDS_BRAVE_SYNC_DISABLED_NEW_CHAIN_DESCRIPTION },
        { "startSyncChain", IDS_BRAVE_SYNC_DISABLED_START_NEW_CHAIN_BUTTON },
        { "enterSyncChainCode", IDS_BRAVE_SYNC_DISABLED_ENTER_CODE_BUTTON },
        // [modal] Enter Sync Code
        { "enterSyncCode", IDS_BRAVE_SYNC_ENTER_CODE_TITLE },
        { "enterSyncCodeDescription", IDS_BRAVE_SYNC_ENTER_CODE_DESCRIPTION },
        { "confirmCode", IDS_BRAVE_SYNC_ENTER_CODE_CONFIRM_CODE_BUTTON },
        // [modal] Remove Main Device
        { "thisDeviceRemovalDescription", IDS_BRAVE_SYNC_REMOVE_THIS_DEVICE_DESCRIPTION },       // NOLINT
        { "joinSyncChain", IDS_BRAVE_SYNC_REMOVE_THIS_DEVICE_JOIN_CHAIN_INSTRUCTIONS },          // NOLINT
        // [modal] Remove Other Device
        { "otherDeviceRemovalDescription", IDS_BRAVE_SYNC_REMOVE_OTHER_DEVICE_DESCRIPTION },     // NOLINT
        // [modal] Reset Sync
        { "warning", IDS_BRAVE_SYNC_RESET_WARNING_TITLE },
        { "removing", IDS_BRAVE_SYNC_RESET_REMOVING_PARTIAL },
        { "deleteSyncChain", IDS_BRAVE_SYNC_RESET_DELETE_CHAIN_PARTIAL },
        { "deleteSyncDescription", IDS_BRAVE_SYNC_RESET_REMOVAL_INSTRUCTIONS },
        { "startSyncChainHowTo", IDS_BRAVE_SYNC_RESET_START_NEW_INSTRUCTIONS },
        { "areYouSure", IDS_BRAVE_SYNC_RESET_ARE_YOU_SURE_TITLE },
        // [modal] Scan Code
        { "scanThisCode", IDS_BRAVE_SYNC_SCAN_CODE_TITLE },
        { "scanThisCodeHowToPartial1", IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_1 },         // NOLINT
        { "scanThisCodeHowToPartial2", IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_2 },         // NOLINT
        { "scanThisCodeHowToPartial3", IDS_BRAVE_SYNC_SCAN_CODE_DESCRIPTION_PARTIAL_3 },         // NOLINT
        // [modal] View Code
        { "chainCode", IDS_BRAVE_SYNC_VIEW_CODE_TITLE },
        { "chainCodeDescriptionPartial1", IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_1 },      // NOLINT
        { "chainCodeDescriptionPartial2", IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_2 },      // NOLINT
        { "chainCodeDescriptionPartial3", IDS_BRAVE_SYNC_VIEW_CODE_DESCRIPTION_PARTIAL_3 },      // NOLINT
        { "useCameraInstead", IDS_BRAVE_SYNC_VIEW_CODE_USE_CAMERA_BUTTON },
        { "qrCode", IDS_BRAVE_SYNC_VIEW_CODE_QR_CODE },
        // [modal] Choose Device Type
        { "letsSync", IDS_BRAVE_SYNC_CHOOSE_DEVICE_TITLE },
        { "chooseDeviceType", IDS_BRAVE_SYNC_CHOOSE_DEVICE_DESCRIPTION },
        { "phoneTablet", IDS_BRAVE_SYNC_CHOOSE_DEVICE_MOBILE_TITLE },
        { "computer", IDS_BRAVE_SYNC_CHOOSE_DEVICE_COMPUTER_TITLE },
        // errors
        { "errorWrongCodeTitle", IDS_BRAVE_SYNC_ERROR_WRONG_CODE_TITLE },
        { "errorWrongCodeDescription", IDS_BRAVE_SYNC_ERROR_WRONG_CODE_DESCRIPTION },            // NOLINT
        { "errorMissingDeviceNameTitle", IDS_BRAVE_SYNC_ERROR_MISSING_DEVICE_NAME_TITLE },       // NOLINT
        { "errorMissingCodeTitle", IDS_BRAVE_SYNC_ERROR_MISSING_SYNC_CODE_TITLE },               // NOLINT
        { "errorSyncInitFailedTitle", IDS_BRAVE_SYNC_ERROR_INIT_FAILED_TITLE },
        { "errorSyncInitFailedDescription", IDS_BRAVE_SYNC_ERROR_INIT_FAILED_DESCRIPTION },      // NOLINT
        { "errorSyncRequiresCorrectTimeTitle", IDS_BRAVE_SYNC_REQUIRES_CORRECT_TIME_TITLE },     // NOLINT
        { "errorSyncRequiresCorrectTimeDescription", IDS_BRAVE_SYNC_REQUIRES_CORRECT_TIME_DESCRIPTION }, // NOLINT
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
      std::string("donate"), {
        { "about", IDS_BRAVE_UI_ABOUT },
        { "addFunds", IDS_BRAVE_UI_ADD_FUNDS },
        { "autoTipText", IDS_BRAVE_UI_AUTO_TIP_TEXT },
        { "bat", IDS_BRAVE_UI_BAT_TEXT },
        { "donationAmount", IDS_BRAVE_UI_DONATION_AMOUNT },
        { "doMonthly", IDS_BRAVE_UI_DO_MONTHLY },
        { "firstTipDateText", IDS_BRAVE_UI_FIRST_TIP_TEXT },
        { "makeMonthly", IDS_BRAVE_UI_MAKE_MONTHLY },
        { "notEnoughTokens", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS },
        { "on", IDS_BRAVE_UI_ON },
        { "monthlyText", IDS_BRAVE_UI_MONTHLY_TEXT },
        { "rewardsBannerText1", IDS_BRAVE_UI_REWARDS_BANNER_TEXT1 },
        { "rewardsBannerText2", IDS_BRAVE_UI_REWARDS_BANNER_TEXT2 },
        { "sendDonation", IDS_BRAVE_UI_SEND_DONATION },
        { "siteBannerNoticeNote", IDS_BRAVE_UI_SITE_BANNER_NOTICE_NOTE },
        { "siteBannerNoticeText", IDS_BRAVE_UI_SITE_BANNER_NOTICE_TEXT },
        { "thankYou", IDS_BRAVE_UI_THANK_YOU },
        { "tipText", IDS_BRAVE_UI_TIP_TEXT },
        { "tokens", IDS_BRAVE_UI_TOKENS },
        { "unVerifiedTextMore", IDS_BRAVE_UI_SITE_UNVERIFIED_TEXT_MORE },
        { "walletBalance", IDS_BRAVE_UI_WALLET_BALANCE },
        { "welcome", IDS_BRAVE_UI_WELCOME },
      }
    }, {
      std::string("rewards-internals"), {
        { "amount", IDS_BRAVE_REWARDS_INTERNALS_AMOUNT },
        { "currentReconcile", IDS_BRAVE_REWARDS_INTERNALS_CURRENT_RECONCILE },
        { "invalid", IDS_BRAVE_REWARDS_INTERNALS_INVALID },
        { "keyInfoSeed", IDS_BRAVE_REWARDS_INTERNALS_KEY_INFO_SEED },
        { "refreshButton", IDS_BRAVE_REWARDS_INTERNALS_REFRESH_BUTTON },
        { "retryLevel", IDS_BRAVE_REWARDS_INTERNALS_RETRY_LEVEL },
        { "retryStep", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP },
        { "retryStepCurrent", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_CURRENT },
        { "retryStepFinal", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_FINAL },
        { "retryStepPayload", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_PAYLOAD },
        { "retryStepPrepare", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_PREPARE },
        { "retryStepProof", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_PROOF },
        { "retryStepReconcile", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_RECONCILE },              // NOLINT
        { "retryStepRegister", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_REGISTER },                // NOLINT
        { "retryStepUnknown", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_UNKNOWN },
        { "retryStepViewing", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_VIEWING },
        { "retryStepVote", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_VOTE },
        { "retryStepWinners", IDS_BRAVE_REWARDS_INTERNALS_RETRY_STEP_WINNERS },
        { "rewardsNotEnabled", IDS_BRAVE_REWARDS_INTERNALS_REWARDS_NOT_ENABLED },                // NOLINT
        { "valid", IDS_BRAVE_REWARDS_INTERNALS_VALID },
        { "viewingId", IDS_BRAVE_REWARDS_INTERNALS_VIEWING_ID },
        { "walletPaymentId", IDS_BRAVE_REWARDS_INTERNALS_WALLET_PAYMENT_ID },
      }
    }
  };
  AddLocalizedStringsBulk(source, localized_strings[name]);
}  // NOLINT(readability/fn_size)
