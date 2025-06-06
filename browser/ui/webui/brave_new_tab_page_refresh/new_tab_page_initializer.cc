// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/new_tab_page_initializer.h"

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/ntp_background/brave_ntp_custom_background_service_factory.h"
#include "brave/browser/resources/brave_new_tab_page_refresh/grit/brave_new_tab_page_refresh_generated_map.h"
#include "brave/browser/ui/brave_ui_features.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/ntp_background_images/browser/ntp_custom_images_source.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/themes/theme_syncable_service.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/sanitized_image_source.h"
#include "chrome/common/webui_url_constants.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/webui_util.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#endif

namespace brave_new_tab_page_refresh {

NewTabPageInitializer::NewTabPageInitializer(content::WebUI& web_ui)
    : web_ui_(web_ui) {}

NewTabPageInitializer::~NewTabPageInitializer() = default;

void NewTabPageInitializer::Initialize() {
  source_ = content::WebUIDataSource::CreateAndAdd(GetProfile(),
                                                   chrome::kChromeUINewTabHost);

  if (brave::ShouldNewTabShowBlankpage(GetProfile())) {
    source_->SetDefaultResource(IDR_BRAVE_BLANK_NEW_TAB_HTML);
  } else {
    webui::SetupWebUIDataSource(source_, kBraveNewTabPageRefreshGenerated,
                                IDR_BRAVE_NEW_TAB_PAGE_HTML);
  }

  AddBackgroundColorToSource(source_, web_ui_->GetWebContents());
  AddCSPOverrides();
  AddLoadTimeValues();
  AddStrings();
  AddPluralStrings();

  AddFaviconDataSource();
  AddCustomImageDataSource();
  AddSanitizedImageDataSource();

  web_ui_->AddRequestableScheme(content::kChromeUIUntrustedScheme);
  web_ui_->OverrideTitle(l10n_util::GetStringUTF16(IDS_NEW_TAB_TITLE));
}

Profile* NewTabPageInitializer::GetProfile() {
  return Profile::FromWebUI(&web_ui_.get());
}

void NewTabPageInitializer::AddCSPOverrides() {
  source_->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://image chrome://resources chrome://theme "
      "chrome://background-wallpaper chrome://custom-wallpaper "
      "chrome://branded-wallpaper chrome://favicon2 blob: data: 'self';");

  source_->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::FrameSrc,
      base::StrCat({"frame-src ", kNTPNewTabTakeoverRichMediaUrl, ";"}));
}

void NewTabPageInitializer::AddLoadTimeValues() {
  auto* profile = GetProfile();

  source_->AddBoolean(
      "customBackgroundFeatureEnabled",
      !profile->GetPrefs()->IsManagedPreference(GetThemePrefNameInMigration(
          ThemePrefInMigration::kNtpCustomBackgroundDict)));

  source_->AddString("sponsoredRichMediaBaseUrl",
                     kNTPNewTabTakeoverRichMediaUrl);

  source_->AddBoolean(
      "ntpSearchFeatureEnabled",
      base::FeatureList::IsEnabled(features::kBraveNtpSearchWidget));

  source_->AddBoolean("rewardsFeatureEnabled",
                      brave_rewards::IsSupportedForProfile(profile));

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  bool vpn_feature_enabled = brave_vpn::IsBraveVPNEnabled(profile->GetPrefs());
#else
  bool vpn_feature_enabled = false;
#endif
  source_->AddBoolean("vpnFeatureEnabled", vpn_feature_enabled);

  source_->AddBoolean("featureFlagBraveNewsFeedV2Enabled", true);
}

void NewTabPageInitializer::AddStrings() {
  static constexpr webui::LocalizedString kStrings[] = {
      {"addTopSiteLabel", IDS_NEW_TAB_ADD_TOP_SITE_LABEL},
      {"addTopSiteTitle", IDS_NEW_TAB_ADD_TOP_SITE_TITLE},
      {"backgroundSettingsTitle", IDS_NEW_TAB_BACKGROUND_SETTINGS_TITLE},
      {"braveBackgroundLabel", IDS_NEW_TAB_BRAVE_BACKGROUND_LABEL},
      {"braveNewsAdvertBadge", IDS_BRAVE_NEWS_ADVERT_BADGE},
      {"braveNewsCaughtUp", IDS_BRAVE_NEWS_CAUGHT_UP},
      {"braveNewsChannel-Brave", IDS_BRAVE_NEWS_CHANNEL_BRAVE},
      {"braveNewsChannel-Business", IDS_BRAVE_NEWS_CHANNEL_BUSINESS},
      {"braveNewsChannel-Cars", IDS_BRAVE_NEWS_CHANNEL_CARS},
      {"braveNewsChannel-Celebrities", IDS_BRAVE_NEWS_CHANNEL_CELEBRITIES},
      {"braveNewsChannel-Crypto", IDS_BRAVE_NEWS_CHANNEL_CRYPTO},
      {"braveNewsChannel-Culture", IDS_BRAVE_NEWS_CHANNEL_CULTURE},
      {"braveNewsChannel-Education", IDS_BRAVE_NEWS_CHANNEL_EDUCATION},
      {"braveNewsChannel-Entertainment", IDS_BRAVE_NEWS_CHANNEL_ENTERTAINMENT},
      {"braveNewsChannel-Fashion", IDS_BRAVE_NEWS_CHANNEL_FASHION},
      {"braveNewsChannel-Film and TV", IDS_BRAVE_NEWS_CHANNEL_FILM_AND_TV},
      {"braveNewsChannel-Food", IDS_BRAVE_NEWS_CHANNEL_FOOD},
      {"braveNewsChannel-Fun", IDS_BRAVE_NEWS_CHANNEL_FUN},
      {"braveNewsChannel-Gaming", IDS_BRAVE_NEWS_CHANNEL_GAMING},
      {"braveNewsChannel-Health", IDS_BRAVE_NEWS_CHANNEL_HEALTH},
      {"braveNewsChannel-Home", IDS_BRAVE_NEWS_CHANNEL_HOME},
      {"braveNewsChannel-Lifestyle", IDS_BRAVE_NEWS_CHANNEL_LIFESTYLE},
      {"braveNewsChannel-Music", IDS_BRAVE_NEWS_CHANNEL_MUSIC},
      {"braveNewsChannel-Politics", IDS_BRAVE_NEWS_CHANNEL_POLITICS},
      {"braveNewsChannel-Regional News", IDS_BRAVE_NEWS_CHANNEL_REGIONAL_NEWS},
      {"braveNewsChannel-Science", IDS_BRAVE_NEWS_CHANNEL_SCIENCE},
      {"braveNewsChannel-Sports", IDS_BRAVE_NEWS_CHANNEL_SPORTS},
      {"braveNewsChannel-Technology", IDS_BRAVE_NEWS_CHANNEL_TECHNOLOGY},
      {"braveNewsChannel-Top News", IDS_BRAVE_NEWS_CHANNEL_TOP_NEWS},
      {"braveNewsChannel-Top Sources", IDS_BRAVE_NEWS_CHANNEL_TOP_SOURCES},
      {"braveNewsChannel-Travel", IDS_BRAVE_NEWS_CHANNEL_TRAVEL},
      {"braveNewsChannel-UK News", IDS_BRAVE_NEWS_CHANNEL_UK_NEWS},
      {"braveNewsChannel-US News", IDS_BRAVE_NEWS_CHANNEL_US_NEWS},
      {"braveNewsChannel-Weather", IDS_BRAVE_NEWS_CHANNEL_WEATHER},
      {"braveNewsChannel-World News", IDS_BRAVE_NEWS_CHANNEL_WORLD_NEWS},
      {"braveNewsChannelsHeader", IDS_BRAVE_NEWS_BROWSE_CHANNELS_HEADER},
      {"braveNewsFollowingFeed", IDS_BRAVE_NEWS_FEEDS_HEADING},
      {"braveNewsForYouFeed", IDS_BRAVE_NEWS_FOR_YOU_FEED},
      {"braveNewsHideContentFrom", IDS_BRAVE_NEWS_HIDE_CONTENT_FROM},
      {"braveNewsNoArticlesMessage", IDS_BRAVE_NEWS_NO_ARTICLES_MESSAGE},
      {"braveNewsNoArticlesTitle", IDS_BRAVE_NEWS_NO_ARTICLES_TITLE},
      {"braveNewsNoContentActionLabel", IDS_BRAVE_NEWS_NO_CONTENT_ACTION_LABEL},
      {"braveNewsNoContentHeading", IDS_BRAVE_NEWS_NO_CONTENT_HEADING},
      {"braveNewsNoContentMessage", IDS_BRAVE_NEWS_NO_CONTENT_MESSAGE},
      {"braveNewsOfflineMessage", IDS_BRAVE_NEWS_OFFLINE_MESSAGE},
      {"braveNewsOfflineTitle", IDS_BRAVE_NEWS_OFFLINE_TITLE},
      {"braveNewsPublishersHeading", IDS_BRAVE_NEWS_PUBLISHERS_HEADING},
      {"braveNewsRefreshFeed", IDS_BRAVE_NEWS_REFRESH_FEED},
      {"braveNewsShowAll", IDS_BRAVE_NEWS_SHOW_ALL},
      {"braveNewsShowLess", IDS_BRAVE_NEWS_SHOW_LESS},
      {"braveNewsSourcesRecommendation", IDS_BRAVE_NEWS_SOURCES_RECOMMENDATION},
      {"cancelButtonLabel", IDS_NEW_TAB_CANCEL_BUTTON_LABEL},
      {"clockFormatLabel", IDS_NEW_TAB_CLOCK_FORMAT_LABEL},
      {"clockFormatOption12HourText",
       IDS_NEW_TAB_CLOCK_FORMAT_OPTION12HOUR_TEXT},
      {"clockFormatOption24HourText",
       IDS_NEW_TAB_CLOCK_FORMAT_OPTION24HOUR_TEXT},
      {"clockFormatOptionAutomaticText",
       IDS_NEW_TAB_CLOCK_FORMAT_OPTION_AUTOMATIC_TEXT},
      {"clockSettingsTitle", IDS_NEW_TAB_CLOCK_SETTINGS_TITLE},
      {"customBackgroundLabel", IDS_NEW_TAB_CUSTOM_BACKGROUND_LABEL},
      {"customBackgroundTitle", IDS_NEW_TAB_CUSTOM_BACKGROUND_LABEL},
      {"customizeSearchEnginesLink", IDS_NEW_TAB_CUSTOMIZE_SEARCH_ENGINES_LINK},
      {"editTopSiteLabel", IDS_NEW_TAB_EDIT_TOP_SITE_LABEL},
      {"editTopSiteTitle", IDS_NEW_TAB_EDIT_TOP_SITE_TITLE},
      {"enabledSearchEnginesLabel", IDS_NEW_TAB_ENABLED_SEARCH_ENGINES_LABEL},
      {"gradientBackgroundLabel", IDS_NEW_TAB_GRADIENT_BACKGROUND_LABEL},
      {"gradientBackgroundTitle", IDS_NEW_TAB_GRADIENT_BACKGROUND_LABEL},
      {"hideTopSitesLabel", IDS_NEW_TAB_HIDE_TOP_SITES_LABEL},
      {"newsBackButtonLabel", IDS_BRAVE_NEWS_BACK_BUTTON},
      {"newsContentAvailableButtonLabel", IDS_BRAVE_NEWS_NEW_CONTENT_AVAILABLE},
      {"newsEnableButtonLabel", IDS_BRAVE_NEWS_OPT_IN_ACTION_LABEL},
      {"newsEnableText", IDS_BRAVE_NEWS_INTRO_TITLE},
      {"newsManageFeedsButtonLabel",
       IDS_NEW_TAB_NEWS_MANAGE_FEEDS_BUTTON_LABEL},
      {"newsNoMatchingFeedsText", IDS_BRAVE_NEWS_DIRECT_SEARCH_NO_RESULTS},
      {"newsOptInDismissButtonLabel", IDS_BRAVE_NEWS_OPT_OUT_ACTION_LABEL},
      {"newsOptInText", IDS_BRAVE_NEWS_INTRO_DESCRIPTION},
      {"newsQueryTooShortText", IDS_BRAVE_NEWS_SEARCH_QUERY_TOO_SHORT},
      {"newsSearchFeedsButtonLabel", IDS_BRAVE_NEWS_DIRECT_SEARCH_BUTTON},
      {"newsSettingsChannelsTitle", IDS_BRAVE_NEWS_BROWSE_CHANNELS_HEADER},
      {"newsSettingsDiscoverTitle", IDS_BRAVE_NEWS_DISCOVER_TITLE},
      {"newsSettingsFollowingTitle", IDS_BRAVE_NEWS_FEEDS_HEADING},
      {"newsSettingsPopularTitle", IDS_BRAVE_NEWS_POPULAR_TITLE},
      {"newsSettingsQueryPlaceholder", IDS_BRAVE_NEWS_SEARCH_PLACEHOLDER_LABEL},
      {"newsSettingsSourcesTitle", IDS_BRAVE_NEWS_ALL_SOURCES_HEADER},
      {"newsSettingsSuggestionsText", IDS_BRAVE_NEWS_SUGGESTIONS_SUBTITLE},
      {"newsSettingsSuggestionsTitle", IDS_BRAVE_NEWS_SUGGESTIONS_TITLE},
      {"newsSettingsTitle", IDS_BRAVE_NEWS_SETTINGS_TITLE},
      {"newsUnfollowButtonLabel", IDS_BRAVE_NEWS_FOLLOW_BUTTON_FOLLOWING},
      {"newsViewAllButtonLabel", IDS_BRAVE_NEWS_VIEW_ALL_BUTTON},
      {"newsWidgetTitle", IDS_NEW_TAB_NEWS_WIDGET_TITLE},
      {"photoCreditsText", IDS_NEW_TAB_PHOTO_CREDITS_TEXT},
      {"randomizeBackgroundLabel", IDS_NEW_TAB_RANDOMIZE_BACKGROUND_LABEL},
      {"removeTopSiteLabel", IDS_NEW_TAB_REMOVE_TOP_SITE_LABEL},
      {"rewardsBalanceTitle", IDS_NEW_TAB_REWARDS_BALANCE_TITLE},
      {"rewardsConnectButtonLabel", IDS_NEW_TAB_REWARDS_CONNECT_BUTTON_LABEL},
      {"rewardsConnectText", IDS_NEW_TAB_REWARDS_CONNECT_TEXT},
      {"rewardsConnectTitle", IDS_NEW_TAB_REWARDS_CONNECT_TITLE},
      {"rewardsFeatureText1", IDS_REWARDS_ONBOARDING_TEXT_ITEM_1},
      {"rewardsFeatureText2", IDS_REWARDS_ONBOARDING_TEXT_ITEM_2},
      {"rewardsOnboardingButtonLabel",
       IDS_NEW_TAB_REWARDS_ONBOARDING_BUTTON_LABEL},
      {"rewardsOnboardingLink", IDS_NEW_TAB_REWARDS_ONBOARDING_LINK},
      {"rewardsWidgetTitle", IDS_NEW_TAB_REWARDS_WIDGET_TITLE},
      {"saveChangesButtonLabel", IDS_NEW_TAB_SAVE_CHANGES_BUTTON_LABEL},
      {"searchAskLeoDescription", IDS_OMNIBOX_ASK_LEO_DESCRIPTION},
      {"searchBoxPlaceholderText", IDS_NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT},
      {"searchBoxPlaceholderTextBrave",
       IDS_NEW_TAB_SEARCH_BOX_PLACEHOLDER_TEXT_BRAVE},
      {"searchCustomizeEngineListText",
       IDS_NEW_TAB_SEARCH_CUSTOMIZE_ENGINE_LIST_TEXT},
      {"searchSettingsTitle", IDS_NEW_TAB_SEARCH_SETTINGS_TITLE},
      {"searchSuggestionsDismissButtonLabel",
       IDS_NEW_TAB_SEARCH_SUGGESTIONS_DISMISS_BUTTON_LABEL},
      {"searchSuggestionsEnableButtonLabel",
       IDS_NEW_TAB_SEARCH_SUGGESTIONS_ENABLE_BUTTON_LABEL},
      {"searchSuggestionsPromptText",
       IDS_NEW_TAB_SEARCH_SUGGESTIONS_PROMPT_TEXT},
      {"searchSuggestionsPromptTitle",
       IDS_NEW_TAB_SEARCH_SUGGESTIONS_PROMPT_TITLE},
      {"settingsTitle", IDS_NEW_TAB_SETTINGS_TITLE},
      {"showBackgroundsLabel", IDS_NEW_TAB_SHOW_BACKGROUNDS_LABEL},
      {"showClockLabel", IDS_NEW_TAB_SHOW_CLOCK_LABEL},
      {"showNewsLabel", IDS_NEW_TAB_SHOW_NEWS_LABEL},
      {"showRewardsWidgetLabel", IDS_NEW_TAB_SHOW_REWARDS_WIDGET_LABEL},
      {"showSearchBoxLabel", IDS_NEW_TAB_SHOW_SEARCH_BOX_LABEL},
      {"showSponsoredImagesEarningText",
       IDS_NEW_TAB_SHOW_SPONSORED_IMAGES_EARNING_TEXT},
      {"showSponsoredImagesLabel", IDS_NEW_TAB_SHOW_SPONSORED_IMAGES_LABEL},
      {"showStatsLabel", IDS_NEW_TAB_SHOW_STATS_LABEL},
      {"showTalkWidgetLabel", IDS_NEW_TAB_SHOW_TALK_WIDGET_LABEL},
      {"showTopSitesLabel", IDS_NEW_TAB_SHOW_TOP_SITES_LABEL},
      {"showVpnWidgetLabel", IDS_NEW_TAB_SHOW_VPN_WIDGET_LABEL},
      {"solidBackgroundLabel", IDS_NEW_TAB_SOLID_BACKGROUND_LABEL},
      {"solidBackgroundTitle", IDS_NEW_TAB_SOLID_BACKGROUND_LABEL},
      {"statsAdsBlockedText", IDS_NEW_TAB_STATS_ADS_BLOCKED_TEXT},
      {"statsBandwidthSavedText", IDS_NEW_TAB_STATS_BANDWIDTH_SAVED_TEXT},
      {"statsTimeSavedText", IDS_NEW_TAB_STATS_TIME_SAVED_TEXT},
      {"statsTitle", IDS_NEW_TAB_STATS_TITLE},
      {"talkDescriptionText", IDS_NEW_TAB_TALK_DESCRIPTION_TEXT},
      {"talkDescriptionTitle", IDS_NEW_TAB_TALK_DESCRIPTION_TITLE},
      {"talkStartCallLabel", IDS_NEW_TAB_TALK_START_CALL_LABEL},
      {"talkWidgetTitle", IDS_NEW_TAB_TALK_WIDGET_TITLE},
      {"topSiteRemovedText", IDS_NEW_TAB_TOP_SITE_REMOVED_TEXT},
      {"topSiteRemovedTitle", IDS_NEW_TAB_TOP_SITE_REMOVED_TITLE},
      {"topSitesCustomOptionText", IDS_NEW_TAB_TOP_SITES_CUSTOM_OPTION_TEXT},
      {"topSitesCustomOptionTitle", IDS_NEW_TAB_TOP_SITES_CUSTOM_OPTION_TITLE},
      {"topSitesMostVisitedOptionText",
       IDS_NEW_TAB_TOP_SITES_MOST_VISITED_OPTION_TEXT},
      {"topSitesMostVisitedOptionTitle",
       IDS_NEW_TAB_TOP_SITES_MOST_VISITED_OPTION_TITLE},
      {"topSitesSettingsTitle", IDS_NEW_TAB_TOP_SITES_SETTINGS_TITLE},
      {"topSitesShowCustomLabel", IDS_NEW_TAB_TOP_SITES_SHOW_CUSTOM_LABEL},
      {"topSitesShowMostVisitedLabel",
       IDS_NEW_TAB_TOP_SITES_SHOW_MOST_VISITED_LABEL},
      {"topSitesTitleLabel", IDS_NEW_TAB_TOP_SITES_TITLE_LABEL},
      {"topSitesURLLabel", IDS_NEW_TAB_TOP_SITES_URL_LABEL},
      {"undoButtonLabel", IDS_NEW_TAB_UNDO_BUTTON_LABEL},
      {"uploadBackgroundLabel", IDS_NEW_TAB_UPLOAD_BACKGROUND_LABEL},
      {"vpnChangeRegionLabel", IDS_NEW_TAB_VPN_CHANGE_REGION_LABEL},
      {"vpnFeatureText1", IDS_NEW_TAB_VPN_FEATURE_TEXT1},
      {"vpnFeatureText2", IDS_NEW_TAB_VPN_FEATURE_TEXT2},
      {"vpnFeatureText3", IDS_NEW_TAB_VPN_FEATURE_TEXT3},
      {"vpnRestorePurchaseLabel", IDS_NEW_TAB_VPN_RESTORE_PURCHASE_LABEL},
      {"vpnStartTrialLabel", IDS_NEW_TAB_VPN_START_TRIAL_LABEL},
      {"vpnOptimalText", IDS_NEW_TAB_VPN_OPTIMAL_TEXT},
      {"vpnPoweredByText", IDS_NEW_TAB_VPN_POWERED_BY_TEXT},
      {"vpnStatusConnected", IDS_NEW_TAB_VPN_STATUS_CONNECTED},
      {"vpnStatusConnecting", IDS_NEW_TAB_VPN_STATUS_CONNECTING},
      {"vpnStatusDisconnected", IDS_NEW_TAB_VPN_STATUS_DISCONNECTED},
      {"vpnStatusDisconnecting", IDS_NEW_TAB_VPN_STATUS_DISCONNECTING},
      {"vpnWidgetTitle", IDS_NEW_TAB_VPN_WIDGET_TITLE},
      {"widgetSettingsTitle", IDS_NEW_TAB_WIDGET_SETTINGS_TITLE}};

  source_->AddLocalizedStrings(kStrings);
}

void NewTabPageInitializer::AddPluralStrings() {
  auto handler = std::make_unique<PluralStringHandler>();
  handler->AddLocalizedString("newsSourceCountText",
                              IDS_BRAVE_NEWS_SOURCE_COUNT);
  web_ui_->AddMessageHandler(std::move(handler));
}

void NewTabPageInitializer::AddFaviconDataSource() {
  auto* profile = GetProfile();
  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

void NewTabPageInitializer::AddCustomImageDataSource() {
  auto* profile = GetProfile();
  auto* custom_background_service =
      BraveNTPCustomBackgroundServiceFactory::GetForContext(profile);
  if (!custom_background_service) {
    return;
  }
  auto source = std::make_unique<ntp_background_images::NTPCustomImagesSource>(
      custom_background_service);
  content::URLDataSource::Add(profile, std::move(source));
}

void NewTabPageInitializer::AddSanitizedImageDataSource() {
  auto* profile = GetProfile();
  content::URLDataSource::Add(profile,
                              std::make_unique<SanitizedImageSource>(profile));
}

}  // namespace brave_new_tab_page_refresh
