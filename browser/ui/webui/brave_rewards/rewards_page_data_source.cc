/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_page_data_source.h"

#include "base/feature_list.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/rewards_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace brave_rewards {

namespace {

static constexpr webui::ResourcePath kResources[] = {
    {"favicon.ico", IDR_BRAVE_REWARDS_FAVICON}};

static constexpr webui::LocalizedString kStrings[] = {
    {"adsHistoryButtonLabel", IDS_REWARDS_ADS_HISTORY_BUTTON_LABEL},
    {"adsHistoryMarkInappropriateLabel",
     IDS_REWARDS_ADS_HISTORY_MARK_INAPPROPRIATE_LABEL},
    {"adsHistoryEmptyText", IDS_REWARDS_ADS_HISTORY_EMPTY_TEXT},
    {"adsHistoryTitle", IDS_REWARDS_ADS_HISTORY_TITLE},
    {"adsHistoryText", IDS_REWARDS_ADS_HISTORY_TEXT},
    {"adsSettingsAdsPerHourNoneText",
     IDS_REWARDS_ADS_SETTINGS_ADS_PER_HOUR_NONE_TEXT},
    {"adsSettingsAdsPerHourText", IDS_REWARDS_ADS_SETTINGS_ADS_PER_HOUR_TEXT},
    {"adsSettingsAdTypeTitle", IDS_REWARDS_ADS_SETTINGS_AD_TYPE_TITLE},
    {"adsSettingsAdViewsTitle", IDS_REWARDS_ADS_SETTINGS_AD_VIEWS_TITLE},
    {"adsSettingsButtonLabel", IDS_REWARDS_ADS_SETTINGS_BUTTON_LABEL},
    {"adsSettingsEarningsLabel", IDS_REWARDS_ADS_SETTINGS_EARNINGS_LABEL},
    {"adsSettingsPayoutDateLabel", IDS_REWARDS_ADS_SETTINGS_PAYOUT_DATE_LABEL},
    {"adsSettingsTotalAdsLabel", IDS_REWARDS_ADS_SETTINGS_TOTAL_ADS_LABEL},
    {"adsSettingsNewsOffTooltip", IDS_REWARDS_ADS_SETTINGS_NEWS_OFF_TOOLTIP},
    {"adsSettingsNewsOnTooltip", IDS_REWARDS_ADS_SETTINGS_NEWS_ON_TOOLTIP},
    {"adsSettingsSearchConnectedTooltip",
     IDS_REWARDS_ADS_SETTINGS_SEARCH_CONNECTED_TOOLTIP},
    {"adsSettingsSearchTooltip", IDS_REWARDS_ADS_SETTINGS_SEARCH_TOOLTIP},
    {"adsSettingsSubdivisionLabel", IDS_REWARDS_ADS_SETTINGS_SUBDIVISION_LABEL},
    {"adsSettingsSubdivisionText", IDS_REWARDS_ADS_SETTINGS_SUBDIVISION_TEXT},
    {"adsSettingsSubdivisionDisabledLabel",
     IDS_REWARDS_ADS_SETTINGS_SUBDIVISION_DISABLED_LABEL},
    {"adsSettingsSubdivisionAutoLabel",
     IDS_REWARDS_ADS_SETTINGS_SUBDIVISION_AUTO_LABEL},
    {"adsSettingsTitle", IDS_REWARDS_ADS_SETTINGS_TITLE},
    {"adsSettingsText", IDS_REWARDS_ADS_SETTINGS_TEXT},
    {"adTypeInlineContentLabel", IDS_REWARDS_AD_TYPE_INLINE_CONTENT_LABEL},
    {"adTypeNewTabPageLabel", IDS_REWARDS_AD_TYPE_NEW_TAB_PAGE_LABEL},
    {"adTypeNotificationLabel", IDS_REWARDS_AD_TYPE_NOTIFICATION_LABEL},
    {"adTypeOffLabel", IDS_REWARDS_AD_TYPE_OFF_LABEL},
    {"adTypeOnLabel", IDS_REWARDS_AD_TYPE_ON_LABEL},
    {"adTypeSearchResultLabel", IDS_REWARDS_AD_TYPE_SEARCH_RESULT_LABEL},
    {"appErrorTitle", IDS_REWARDS_APP_ERROR_TITLE},
    {"authorizeDeviceLimitReachedText",
     IDS_REWARDS_AUTHORIZE_DEVICE_LIMIT_REACHED_TEXT},
    {"authorizeDeviceLimitReachedTitle",
     IDS_REWARDS_AUTHORIZE_DEVICE_LIMIT_REACHED_TITLE},
    {"authorizeErrorTitle", IDS_REWARDS_AUTHORIZE_ERROR_TITLE},
    {"authorizeFlaggedWalletText1",
     IDS_REWARDS_AUTHORIZE_FLAGGED_WALLET_TEXT_1},
    {"authorizeFlaggedWalletText2",
     IDS_REWARDS_AUTHORIZE_FLAGGED_WALLET_TEXT_2},
    {"authorizeFlaggedWalletText3",
     IDS_REWARDS_AUTHORIZE_FLAGGED_WALLET_TEXT_3},
    {"authorizeFlaggedWalletText4",
     IDS_REWARDS_AUTHORIZE_FLAGGED_WALLET_TEXT_4},
    {"authorizeFlaggedWalletTitle", IDS_REWARDS_AUTHORIZE_FLAGGED_WALLET_TITLE},
    {"authorizeKycRequiredText", IDS_REWARDS_AUTHORIZE_KYC_REQUIRED_TEXT},
    {"authorizeKycRequiredTitle", IDS_REWARDS_AUTHORIZE_KYC_REQUIRED_TITLE},
    {"authorizeMismatchedCountriesText",
     IDS_REWARDS_AUTHORIZE_MISMATCHED_COUNTRIES_TEXT},
    {"authorizeMismatchedCountriesTitle",
     IDS_REWARDS_AUTHORIZE_MISMATCHED_COUNTRIES_TITLE},
    {"authorizeMismatchedProviderAccountsText",
     IDS_REWARDS_AUTHORIZE_MISMATCHED_PROVIDER_ACCOUNTS_TEXT},
    {"authorizeMismatchedProviderAccountsTitle",
     IDS_REWARDS_AUTHORIZE_MISMATCHED_PROVIDER_ACCOUNTS_TITLE},
    {"authorizeProcessingText", IDS_REWARDS_AUTHORIZE_PROCESSING_TEXT},
    {"authorizeProviderUnavailableTitle",
     IDS_REWARDS_AUTHORIZE_PROVIDER_UNAVAILABLE_TITLE},
    {"authorizeProviderUnavailableText1",
     IDS_REWARDS_AUTHORIZE_PROVIDER_UNAVAILABLE_TEXT_1},
    {"authorizeProviderUnavailableText2",
     IDS_REWARDS_AUTHORIZE_PROVIDER_UNAVAILABLE_TEXT_2},
    {"authorizeRegionNotSupportedText1",
     IDS_REWARDS_AUTHORIZE_REGION_NOT_SUPPORTED_TEXT_1},
    {"authorizeRegionNotSupportedText2",
     IDS_REWARDS_AUTHORIZE_REGION_NOT_SUPPORTED_TEXT_2},
    {"authorizeRegionNotSupportedTitle",
     IDS_REWARDS_AUTHORIZE_REGION_NOT_SUPPORTED_TITLE},
    {"authorizeSignatureVerificationErrorText",
     IDS_REWARDS_AUTHORIZE_SIGNATURE_VERIFICATION_ERROR_TEXT},
    {"authorizeSignatureVerificationErrorTitle",
     IDS_REWARDS_AUTHORIZE_SIGNATURE_VERIFICATION_ERROR_TITLE},
    {"authorizeUnexpectedErrorText",
     IDS_REWARDS_AUTHORIZE_UNEXPECTED_ERROR_TEXT},
    {"authorizeUnexpectedErrorTitle",
     IDS_REWARDS_AUTHORIZE_UNEXPECTED_ERROR_TITLE},
    {"authorizeUpholdBatNotAllowedText",
     IDS_REWARDS_AUTHORIZE_UPHOLD_BAT_NOT_ALLOWED_TEXT},
    {"authorizeUpholdBatNotAllowedTitle",
     IDS_REWARDS_AUTHORIZE_UPHOLD_BAT_NOT_ALLOWED_TITLE},
    {"authorizeUpholdInsufficientCapabilitiesText",
     IDS_REWARDS_AUTHORIZE_UPHOLD_INSUFFICIENT_CAPABILITIES_TEXT},
    {"authorizeUpholdInsufficientCapabilitiesTitle",
     IDS_REWARDS_AUTHORIZE_UPHOLD_INSUFFICIENT_CAPABILITIES_TITLE},
    {"cancelButtonLabel", IDS_REWARDS_PANEL_CANCEL},
    {"closeButtonLabel", IDS_BRAVE_REWARDS_ONBOARDING_CLOSE},
    {"connectAccountSubtext", IDS_REWARDS_CONNECT_ACCOUNT_SUBTEXT},
    {"connectAccountText", IDS_REWARDS_CONNECT_ACCOUNT_TEXT_2},
    {"connectButtonLabel", IDS_REWARDS_CONNECT_ACCOUNT},
    {"connectCustodialTitle", IDS_REWARDS_CONNECT_CUSTODIAL_TITLE},
    {"connectCustodialTooltip", IDS_REWARDS_CONNECT_CUSTODIAL_TOOLTIP},
    {"connectLoginText", IDS_REWARDS_CONNECT_LOGIN_TEXT},
    {"connectProviderNotAvailable", IDS_REWARDS_CONNECT_PROVIDER_NOT_AVAILABLE},
    {"connectRegionsLearnMoreText",
     IDS_REWARDS_CONNECT_REGIONS_LEARN_MORE_TEXT},
    {"connectSelfCustodyError", IDS_REWARDS_CONNECT_SELF_CUSTODY_ERROR},
    {"connectSelfCustodyNote", IDS_REWARDS_CONNECT_SELF_CUSTODY_NOTE},
    {"connectSelfCustodyTerms", IDS_REWARDS_CONNECT_SELF_CUSTODY_TERMS},
    {"connectSelfCustodyTitle", IDS_REWARDS_CONNECT_SELF_CUSTODY_TITLE},
    {"connectSelfCustodyTooltip", IDS_REWARDS_CONNECT_SELF_CUSTODY_TOOLTIP},
    {"connectSolanaButtonLabel", IDS_REWARDS_CONNECT_SOLANA_BUTTON_LABEL},
    {"connectSolanaMessage", IDS_REWARDS_CONNECT_SOLANA_MESSAGE},
    {"connectText", IDS_REWARDS_CONNECT_TEXT},
    {"connectTitle", IDS_REWARDS_CONNECT_TITLE},
    {"continueButtonLabel", IDS_BRAVE_REWARDS_ONBOARDING_CONTINUE},
    {"countrySelectPlaceholder", IDS_BRAVE_REWARDS_ONBOARDING_SELECT_COUNTRY},
    {"countrySelectTitle", IDS_REWARDS_COUNTRY_SELECT_TITLE},
    {"countrySelectText", IDS_REWARDS_COUNTRY_SELECT_TEXT},
    {"doneButtonLabel", IDS_BRAVE_REWARDS_ONBOARDING_DONE},
    {"earningsAdsReceivedText", IDS_REWARDS_EARNINGS_ADS_RECEIVED_TEXT},
    {"earningsEstimateText", IDS_REWARDS_EARNINGS_ESTIMATE_TEXT},
    {"earningsRangeTooltip", IDS_REWARDS_EARNINGS_RANGE_TOOLTIP},
    {"helpButtonLabel", IDS_REWARDS_HELP_BUTTON_LABEL},
    {"learnMoreLink", IDS_REWARDS_LEARN_MORE},
    {"moreButtonLabel", IDS_REWARDS_MORE_BUTTON_LABEL},
    {"navigationCreatorsLabel", IDS_REWARDS_NAVIGATION_CREATORS_LABEL},
    {"navigationExploreLabel", IDS_REWARDS_NAVIGATION_EXPLORE_LABEL},
    {"navigationHomeLabel", IDS_REWARDS_NAVIGATION_HOME_LABEL},
    {"newBadgeText", IDS_REWARDS_NEW_BADGE_TEXT},
    {"onboardingButtonLabel", IDS_REWARDS_ONBOARDING_BUTTON_LABEL},
    {"onboardingErrorCountryDeclaredText",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DECLARE_COUNTRY},
    {"onboardingErrorDisabledText",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DISABLED},
    {"onboardingErrorDisabledTitle",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER_DISABLED},
    {"onboardingErrorText", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT},
    {"onboardingErrorTitle", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER},
    {"onboardingLearnMoreLabel", IDS_REWARDS_LEARN_MORE},
    {"onboardingSuccessLearnMoreLabel",
     IDS_BRAVE_REWARDS_ONBOARDING_HOW_DOES_IT_WORK},
    {"onboardingSuccessText", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_TEXT},
    {"onboardingSuccessTitle", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_HEADER},
    {"onboardingTermsText", IDS_REWARDS_ONBOARDING_TERMS_TEXT},
    {"onboardingTextItem1", IDS_REWARDS_ONBOARDING_TEXT_ITEM_1},
    {"onboardingTextItem2", IDS_REWARDS_ONBOARDING_TEXT_ITEM_2},
    {"onboardingTitle", IDS_REWARDS_ONBOARDING_TITLE},
    {"resetButtonLabel", IDS_BRAVE_UI_RESET},
    {"resetConsentText", IDS_BRAVE_UI_REWARDS_RESET_CONSENT},
    {"resetRewardsText", IDS_BRAVE_UI_REWARDS_RESET_TEXT},
    {"resetRewardsTitle", IDS_BRAVE_UI_RESET_WALLET},
    {"rewardsPageTitle", IDS_REWARDS_PAGE_TITLE}};

}  // namespace

void CreateAndAddRewardsPageDataSource(content::WebUI& web_ui,
                                       const std::string& host) {
  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui.GetWebContents()->GetBrowserContext(), host);

  webui::SetupWebUIDataSource(
      source, base::make_span(kRewardsPageGenerated, kRewardsPageGeneratedSize),
      IDR_NEW_BRAVE_REWARDS_PAGE_HTML);

  // Override img-src to allow chrome://rewards-image support.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://resources chrome://theme chrome://rewards-image "
      "chrome://favicon2 blob: data: 'self';");

  source->AddResourcePaths(kResources);
  source->AddLocalizedStrings(kStrings);

#if BUILDFLAG(IS_ANDROID)
  source->AddString("platform", "android");
#else
  source->AddString("platform", "desktop");
#endif

  source->AddBoolean("isBubble", host == kRewardsPageTopHost);

  source->AddBoolean(
      "animatedBackgroundEnabled",
      base::FeatureList::IsEnabled(features::kAnimatedBackgroundFeature));
}

}  // namespace brave_rewards
