/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_webui_source.h"

#include <map>

#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"

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

void CustomizeWebUIHTMLSource(const std::string &name, content::WebUIDataSource* source) {
  static std::map<std::string, std::vector<WebUISimpleItem> > resources = {
    {
      std::string("newtab"), {
        { "af7ae505a9eed503f8b8e6982036873e.woff2", IDR_BRAVE_COMMON_FONT_AWESOME_1 },
        { "fee66e712a8a08eef5805a46892932ad.woff", IDR_BRAVE_COMMON_FONT_AWESOME_2 },
        { "b06871f281fee6b241d60582ae9369b9.ttf", IDR_BRAVE_COMMON_FONT_AWESOME_3 },

        // New private tab requires Poppins 200 and 400
        // Other variants not added as they're under 10kb size and thus inlined by webpack
        { "1ff8e6c958ca4fa6d8e8680f32cddd0d.woff2", IDT_BRAVE_TEXT_FONT_POPPINS_200_NORMAL_DEVANAGARI },
        { "8a06c170adbf19e0dffcbe868719e6ce.woff2", IDT_BRAVE_TEXT_FONT_POPPINS_400_NORMAL_DEVANAGARI },

        { "img/toolbar/menu_btn.svg", IDR_BRAVE_COMMON_TOOLBAR_IMG },
        // Hash path is the MD5 of the file contents, webpack image loader does this
        { "fd85070af5114d6ac462c466e78448e4.svg", IDR_BRAVE_NEW_TAB_IMG1 },
        { "314e7529efec41c8867019815f4d8dad.svg", IDR_BRAVE_NEW_TAB_IMG4 },
        { "6c337c63662ee0ba4e57f6f8156d69ce.svg", IDR_BRAVE_NEW_TAB_IMG2 },
        { "50cc52a4f1743ea74a21da996fe44272.jpg", IDR_BRAVE_NEW_TAB_IMG14 },
        { "b6dd4b1292cfd4470e58486c56ad0832.svg", IDR_BRAVE_NEW_TAB_PRIVATE_ICON },
        // New tab Backgrounds
        { "dksfoto1.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND1 },
        { "dksfoto2.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND2 },
        { "dksfoto3.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND3 },
        { "dksfoto4.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND4 },
        { "dksfoto5.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND5 },
        { "dksfoto6.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND6 },
        { "dksfoto7.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND7 },
        { "dksfoto8.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND8 },
        { "dksfoto9.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND9 },
        { "dksfoto10.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND10 },
        { "dksfoto11.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND11 },
        { "Phoyoserge_Corsica.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND12 },
        { "Phoyoserge_Corsica2.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND13 },
        { "Phoyoserge_DowntownGriffith.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND14 },
        { "Phoyoserge_ElmatadorBeach.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND15 },
        { "Phoyoserge_ParisConciergeri.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND16 },
        { "Phoyoserge_Theroofparis.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND17 },
        { "Phoyoserge_TheSeantParis.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND18 },
        { "Phoyoserge_VeniseSunset.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND19 },
        { "Phoyoserge_Yosemite.jpg", IDR_BRAVE_NEW_TAB_BACKGROUND20 }
      }
    }, {
      std::string("welcome"), {
      { "51a13e5e543f312a990d4fd7e741d427.png", IDR_BRAVE_WELCOME_SLIDE_1_IMAGE },
      { "a6abd363c58f91a260f94f5beb32b172.png", IDR_BRAVE_WELCOME_SLIDE_2_IMAGE },
      { "fc31b8d3f7f3d32eec78365212f3002b.png", IDR_BRAVE_WELCOME_SLIDE_3_IMAGE },
      { "59ec9e5cdea1df1630f8c6e2b7a27ede.png", IDR_BRAVE_WELCOME_SLIDE_4_IMAGE },
      { "c2217e15737be3c82f5fef818d3fd26c.png", IDR_BRAVE_WELCOME_SLIDE_5_IMAGE },
      { "e9d936c617aad55fe1bc7a001f2defb4.svg", IDR_BRAVE_WELCOME_BACKGROUND_IMAGE }
      }
    }, {
      std::string("rewards"), {
        { "823882f2419bf6c16b8291e056ea4b52.svg", IDR_BRAVE_REWARDS_IMG_WALLET_ICON },
        { "53ef964d53d132d1849285f533c74df6.svg", IDR_BRAVE_REWARDS_IMG_FUNDS_ICON },
        { "9c14c003bf37abc636c0d266a0ac33a7.svg", IDR_BRAVE_REWARDS_IMG_ADS_DISABLED },
        { "ea846f2b45804dd849672945f1becd19.svg", IDR_BRAVE_REWARDS_IMG_CONTRIBUTE_DISABLED },
        { "4fcfa7f92c5fc22c2b6f34701bfdcd0a.jpeg", IDR_BRAVE_REWARDS_IMG_BART_TEMP }
      }
    }, {
      std::string("adblock"), {
      }
    }
  };
  AddResourcePaths(source, resources[name]);

  static std::map<std::string, std::vector<WebUISimpleItem> > localized_strings = {
    {
      std::string("newtab"), {
        { "adsBlocked", IDS_BRAVE_NEW_TAB_TOTAL_ADS_BLOCKED },
        { "trackersBlocked", IDS_BRAVE_NEW_TAB_TOTAL_TRACKERS_BLOCKED },
        { "httpsUpgraded", IDS_BRAVE_NEW_TAB_TOTAL_HTTPS_UPGRADES },
        { "estimatedTimeSaved", IDS_BRAVE_NEW_TAB_TOTAL_TIME_SAVED },
        { "thumbRemoved", IDS_BRAVE_NEW_TAB_THUMB_REMOVED },
        { "undoRemoved", IDS_BRAVE_NEW_TAB_UNDO_REMOVED },
        { "restoreAll", IDS_BRAVE_NEW_TAB_RESTORE_ALL },
        { "second", IDS_BRAVE_NEW_TAB_SECOND },
        { "seconds", IDS_BRAVE_NEW_TAB_SECONDS },
        { "minute", IDS_BRAVE_NEW_TAB_MINUTE },
        { "minutes", IDS_BRAVE_NEW_TAB_MINUTES },
        { "hour", IDS_BRAVE_NEW_TAB_HOUR },
        { "hours", IDS_BRAVE_NEW_TAB_HOURS },
        { "day", IDS_BRAVE_NEW_TAB_DAY },
        { "days", IDS_BRAVE_NEW_TAB_DAYS },
        { "privateNewTabTitle", IDS_BRAVE_PRIVATE_NEW_TAB_TITLE },
        { "privateNewTabDisclaimer1", IDS_BRAVE_PRIVATE_NEW_TAB_DISCLAIMER_1 },
        { "privateNewTabDisclaimer2", IDS_BRAVE_PRIVATE_NEW_TAB_DISCLAIMER_2 },
        { "duckduckGoSearchInfo", IDS_BRAVE_PRIVATE_NEW_TAB_DUCKDUCKGO_SEARCH_INFO },
        { "privateNewTabSearchLabel", IDS_BRAVE_PRIVATE_NEW_TAB_SEARCH_TOGGLE_LABEL }
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
        { "importNow", IDS_BRAVE_WELCOME_PAGE_IMPORT_BUTTON },
        { "manageShields", IDS_BRAVE_WELCOME_PAGE_SHIELDS_TITLE },
        { "adjustProtectionLevel", IDS_BRAVE_WELCOME_PAGE_SHIELDS_DESC },
        { "shieldSettings", IDS_BRAVE_WELCOME_PAGE_SHIELDS_BUTTON },
        { "customizePreferences", IDS_BRAVE_WELCOME_PAGE_PREFERENCES_TITLE },
        { "configure", IDS_BRAVE_WELCOME_PAGE_PREFERENCES_DESC },
        { "preferences", IDS_BRAVE_WELCOME_PAGE_PREFERENCES_BUTTON },
        { "skipWelcomeTour", IDS_BRAVE_WELCOME_PAGE_SKIP_BUTTON },
        { "next", IDS_BRAVE_WELCOME_PAGE_NEXT_BUTTON },
        { "done", IDS_BRAVE_WELCOME_PAGE_DONE_BUTTON }
      }
    }, {
      std::string("rewards"), {
        { "adsTitle",  IDS_BRAVE_REWARDS_LOCAL_ADS_TITLE },
        { "adsDesc",  IDS_BRAVE_REWARDS_LOCAL_ADS_DESC },
        { "adsDisabledText",  IDS_BRAVE_REWARDS_LOCAL_ADS_DISABLED_TEXT },
        { "contributionTitle",  IDS_BRAVE_REWARDS_LOCAL_CONTR_TITLE },
        { "contributionDesc",  IDS_BRAVE_REWARDS_LOCAL_CONTR_DESC },
        { "contributionMonthly",  IDS_BRAVE_REWARDS_LOCAL_CONTR_MONTHLY },
        { "contributionNextDate",  IDS_BRAVE_REWARDS_LOCAL_CONTR_NEXT_DATE },
        { "contributionSites",  IDS_BRAVE_REWARDS_LOCAL_CONTR_SITES },
        { "contributionDisabledText1",  IDS_BRAVE_REWARDS_LOCAL_CONTR_DISABLED_TEXT1 },
        { "contributionDisabledText2",  IDS_BRAVE_REWARDS_LOCAL_CONTR_DISABLED_TEXT2 },
        { "contributionSiteVisited",  IDS_BRAVE_REWARDS_LOCAL_CONTR_SITE_VISITED },
        { "contributionSiteAttention",  IDS_BRAVE_REWARDS_LOCAL_CONTR_ATTENTION },
        { "contributionVisitSome",  IDS_BRAVE_REWARDS_LOCAL_CONTR_VISIT_SOME },
        { "donationTitle",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TITLE },
        { "donationDesc",  IDS_BRAVE_REWARDS_LOCAL_DONAT_DESC },
        { "donationTotalDonations",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TOTAL_DONATIONS },
        { "donationTotal",  IDS_BRAVE_REWARDS_LOCAL_DONAT_TOTAL },
        { "donationList",  IDS_BRAVE_REWARDS_LOCAL_DONAT_LIST },
        { "donationVisitSome",  IDS_BRAVE_REWARDS_LOCAL_DONAT_VISIT_SOME },
        { "panelAddFunds",  IDS_BRAVE_REWARDS_LOCAL_PANEL_ADD_FUNDS },
        { "panelWithdrawFunds",  IDS_BRAVE_REWARDS_LOCAL_PANEL_WITHDRAW_FUNDS },
        { "tokens",  IDS_BRAVE_REWARDS_LOCAL_TOKENS },

        { "about", IDS_BRAVE_UI_ABOUT },
        { "addFunds", IDS_BRAVE_UI_ADD_FUNDS },
        { "allowTip", IDS_BRAVE_UI_ALLOW_TIP },
        { "braveRewards", IDS_BRAVE_UI_BRAVE_REWARDS },
        { "cancel", IDS_BRAVE_UI_CANCEL },
        { "claim", IDS_BRAVE_UI_CLAIM },
        { "copy", IDS_BRAVE_UI_COPY },
        { "currentDonation", IDS_BRAVE_UI_CURRENT_DONATION },
        { "detail", IDS_BRAVE_UI_DETAIL },
        { "donationAmount", IDS_BRAVE_UI_DONATION_AMOUNT },
        { "done", IDS_BRAVE_UI_DONE },
        { "earningsAds", IDS_BRAVE_UI_EARNINGS_ADS },
        { "expiresOn", IDS_BRAVE_UI_EXPIRES_ON },
        { "import", IDS_BRAVE_UI_IMPORT },
        { "makeMonthly", IDS_BRAVE_UI_MAKE_MONTHLY },
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
        { "noGrants", IDS_BRAVE_UI_NO_GRANTS },
        { "notEnoughTokens", IDS_BRAVE_UI_NOT_ENOUGH_TOKENS },
        { "on", IDS_BRAVE_UI_ON },
        { "oneTime", IDS_BRAVE_UI_ONE_TIME },
        { "oneTimeDonation", IDS_BRAVE_UI_ONE_TIME_DONATION },
        { "print", IDS_BRAVE_UI_PRINT },
        { "recoveryKeys", IDS_BRAVE_UI_RECOVERY_KEYS },
        { "recurring", IDS_BRAVE_UI_RECURRING },
        { "recurringDonations", IDS_BRAVE_UI_RECURRING_DONATIONS },
        { "remove", IDS_BRAVE_UI_REMOVE },
        { "restore", IDS_BRAVE_UI_RESTORE },
        { "rewardsBackupText1", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT1 },
        { "rewardsBackupText2", IDS_BRAVE_UI_REWARDS_BACKUP_TEXT2 },
        { "rewardsBannerText1", IDS_BRAVE_UI_REWARDS_BANNER_TEXT1 },
        { "rewardsBannerText2", IDS_BRAVE_UI_REWARDS_BANNER_TEXT2 },
        { "rewardsContribute", IDS_BRAVE_UI_REWARDS_CONTRIBUTE },
        { "rewardsContributeAttention", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_ATTENTION },
        { "rewardsContributeText1", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_TEXT1 },
        { "rewardsContributeVisited", IDS_BRAVE_UI_REWARDS_CONTRIBUTE_VISITED },
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
        { "rewardsRestoreText1", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT1 },
        { "rewardsRestoreText2", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT2 },
        { "rewardsRestoreText3", IDS_BRAVE_UI_REWARDS_RESTORE_TEXT3 },
        { "rewardsSummary", IDS_BRAVE_UI_REWARDS_SUMMARY },
        { "rewardsWhy", IDS_BRAVE_UI_REWARDS_WHY },
        { "saveAsFile", IDS_BRAVE_UI_SAVE_AS_FILE },
        { "seeAllItems", IDS_BRAVE_UI_SEE_ALL_ITEMS },
        { "seeAllSites", IDS_BRAVE_UI_SEE_ALL_SITES },
        { "sendDonation", IDS_BRAVE_UI_SEND_DONATION },
        { "sendTip", IDS_BRAVE_UI_SEND_TIP },
        { "settings", IDS_BRAVE_UI_SETTINGS },
        { "siteVisited", IDS_BRAVE_UI_SITE_VISITED },
        { "sites", IDS_BRAVE_UI_SITES },
        { "tipOnLike", IDS_BRAVE_UI_TIP_ON_LIKE },
        { "tokenBalance", IDS_BRAVE_UI_TOKEN_BALANCE },
        { "tokenGrant", IDS_BRAVE_UI_TOKEN_GRANT },
        { "tokens", IDS_BRAVE_UI_TOKENS },
        { "type", IDS_BRAVE_UI_TYPE },
        { "verifiedPublisher", IDS_BRAVE_UI_VERIFIED_PUBLISHER },
        { "walletActivity", IDS_BRAVE_UI_WALLET_ACTIVITY },
        { "walletBalance", IDS_BRAVE_UI_WALLET_BALANCE },
        { "welcome", IDS_BRAVE_UI_WELCOME },
        { "yourWallet", IDS_BRAVE_UI_YOUR_WALLET  }
      }
    }, {
      std::string("adblock"), {
        { "adsBlocked", IDS_ADBLOCK_TOTAL_ADS_BLOCKED },
        { "regionalAdblockEnabledTitle", IDS_ADBLOCK_REGIONAL_AD_BLOCK_ENABLED_TITLE},
        { "regionalAdblockEnabled", IDS_ADBLOCK_REGIONAL_AD_BLOCK_ENABLED },
        { "regionalAdblockDisabled", IDS_ADBLOCK_REGIONAL_AD_BLOCK_DISABLED },
      }
    }
  };
  AddLocalizedStringsBulk(source, localized_strings[name]);
}
