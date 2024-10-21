/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_page_data_source.h"

#include <memory>

#include "base/feature_list.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/rewards_page_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"

namespace brave_rewards {

namespace {

static constexpr webui::ResourcePath kResources[] = {
    {"favicon.ico", IDR_BRAVE_REWARDS_FAVICON}};

static constexpr webui::LocalizedString kStrings[] = {
    {"acAmountLabel", IDS_REWARDS_AC_AMOUNT_LABEL},
    {"acAmountText", IDS_REWARDS_AC_AMOUNT_TEXT},
    {"acAttentionLabel", IDS_REWARDS_AC_ATTENTION_LABEL},
    {"acDisabledText", IDS_REWARDS_AC_DISABLED_TEXT},
    {"acDisabledTitle", IDS_REWARDS_AC_DISABLED_TITLE},
    {"acEmptyListText", IDS_REWARDS_AC_EMPTY_LIST_TEXT},
    {"acInfoText", IDS_REWARDS_AC_INFO_TEXT},
    {"acInfoTitle", IDS_REWARDS_AC_INFO_TITLE},
    {"acNextContributionLabel", IDS_REWARDS_AC_NEXT_CONTRIBUTION_LABEL},
    {"acSiteCountLabel", IDS_REWARDS_AC_SITE_COUNT_LABEL},
    {"acSiteLabel", IDS_REWARDS_AC_SITE_LABEL},
    {"acTitle", IDS_REWARDS_AC_TITLE},
    {"adsBrowserUpgradeRequiredText",
     IDS_REWARDS_ADS_BROWSER_UPGRADE_REQUIRED_TEXT},
    {"adsHistoryButtonLabel", IDS_REWARDS_ADS_HISTORY_BUTTON_LABEL},
    {"adsHistoryMarkInappropriateLabel",
     IDS_REWARDS_ADS_HISTORY_MARK_INAPPROPRIATE_LABEL},
    {"adsHistoryEmptyText", IDS_REWARDS_ADS_HISTORY_EMPTY_TEXT},
    {"adsHistoryTitle", IDS_REWARDS_ADS_HISTORY_TITLE},
    {"adsHistoryText", IDS_REWARDS_ADS_HISTORY_TEXT},
    {"adsRegionNotSupportedText", IDS_REWARDS_ADS_REGION_NOT_SUPPORTED_TEXT},
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
    {"benefitsStoreSubtext", IDS_REWARDS_BENEFITS_STORE_SUBTEXT},
    {"benefitsStoreText", IDS_REWARDS_BENEFITS_STORE_TEXT},
    {"benefitsTitle", IDS_REWARDS_BENEFITS_TITLE},
    {"cancelButtonLabel", IDS_REWARDS_PANEL_CANCEL},
    {"captchaMaxAttemptsExceededText",
     IDS_REWARDS_CAPTCHA_MAX_ATTEMPTS_EXCEEDED_TEXT},
    {"captchaMaxAttemptsExceededTitle",
     IDS_REWARDS_CAPTCHA_MAX_ATTEMPTS_EXCEEDED_TITLE},
    {"captchaSolvedText", IDS_REWARDS_CAPTCHA_SOLVED_TEXT},
    {"captchaSolvedTitle", IDS_REWARDS_CAPTCHA_SOLVED_TITLE},
    {"captchaSupportButtonLabel", IDS_REWARDS_CAPTCHA_CONTACT_SUPPORT},
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
    {"contributeAboutMethodsLink", IDS_REWARDS_CONTRIBUTE_ABOUT_METHODS_LINK},
    {"contributeAmountTitle", IDS_REWARDS_CONTRIBUTE_AMOUNT_TITLE},
    {"contributeAvailableMethodsText",
     IDS_REWARDS_CONTRIBUTE_AVAILABLE_METHODS_TEXT},
    {"contributeBalanceTitle", IDS_REWARDS_CONTRIBUTE_BALANCE_TITLE},
    {"contributeBalanceUnavailableText",
     IDS_REWARDS_CONTRIBUTE_BALANCE_UNAVAILABLE_TEXT},
    {"contributeButtonLabel", IDS_REWARDS_CONTRIBUTE_BUTTON_LABEL},
    {"contributeChooseMethodText", IDS_REWARDS_CONTRIBUTE_CHOOSE_METHOD_TEXT},
    {"contributeCustodialSubtext", IDS_REWARDS_CONTRIBUTE_CUSTODIAL_SUBTEXT},
    {"contributeErrorText", IDS_REWARDS_CONTRIBUTE_ERROR_TEXT},
    {"contributeErrorTitle", IDS_REWARDS_CONTRIBUTE_ERROR_TITLE},
    {"contributeInsufficientFundsButtonLabel",
     IDS_REWARDS_CONTRIBUTE_INSUFFICIENT_FUNDS_BUTTON_LABEL},
    {"contributeLoginButtonLabel", IDS_REWARDS_CONTRIBUTE_LOGIN_BUTTON_LABEL},
    {"contributeLoggedOutText", IDS_REWARDS_CONTRIBUTE_LOGGED_OUT_TEXT},
    {"contributeLoggedOutTitle", IDS_REWARDS_CONTRIBUTE_LOGGED_OUT_TITLE},
    {"contributeLoggedOutWeb3ButtonLabel",
     IDS_REWARDS_CONTRIBUTE_LOGGED_OUT_WEB3BUTTON_LABEL},
    {"contributeLoggedOutWeb3Text", IDS_REWARDS_CONTRIBUTE_LOGGED_OUT_WEB3TEXT},
    {"contributeMonthlyLabel", IDS_REWARDS_CONTRIBUTE_MONTHLY_LABEL},
    {"contributeOtherLabel", IDS_REWARDS_CONTRIBUTE_OTHER_LABEL},
    {"contributeRecurringLabel", IDS_REWARDS_CONTRIBUTE_RECURRING_LABEL},
    {"contributeSendAmountButtonLabel",
     IDS_REWARDS_CONTRIBUTE_SEND_AMOUNT_BUTTON_LABEL},
    {"contributeSendButtonLabel", IDS_REWARDS_CONTRIBUTE_SEND_BUTTON_LABEL},
    {"contributeSendingText", IDS_REWARDS_CONTRIBUTE_SENDING_TEXT},
    {"contributeSuccessText", IDS_REWARDS_CONTRIBUTE_SUCCESS_TEXT},
    {"contributeSuccessTitle", IDS_REWARDS_CONTRIBUTE_SUCCESS_TITLE},
    {"contributeWeb3Label", IDS_REWARDS_CONTRIBUTE_WEB3LABEL},
    {"contributeWeb3Subtext", IDS_REWARDS_CONTRIBUTE_WEB3SUBTEXT},
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
    {"notificationAutoContributeCompletedText",
     IDS_REWARDS_NOTIFICATION_AUTO_CONTRIBUTE_COMPLETED_TEXT},
    {"notificationAutoContributeCompletedTitle",
     IDS_REWARDS_NOTIFICATION_AUTO_CONTRIBUTE_COMPLETED_TITLE},
    {"notificationWalletDisconnectedAction",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_ACTION},
    {"notificationWalletDisconnectedText",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_TEXT},
    {"notificationWalletDisconnectedTitle",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_TITLE},
    {"notificationMonthlyContributionFailedTitle",
     IDS_REWARDS_NOTIFICATION_MONTHLY_CONTRIBUTION_FAILED_TITLE},
    {"notificationMonthlyContributionFailedText",
     IDS_REWARDS_NOTIFICATION_MONTHLY_CONTRIBUTION_FAILED_TEXT},
    {"notificationMonthlyTipCompletedText",
     IDS_REWARDS_NOTIFICATION_MONTHLY_TIP_COMPLETED_TEXT},
    {"notificationMonthlyTipCompletedTitle",
     IDS_REWARDS_NOTIFICATION_MONTHLY_TIP_COMPLETED_TITLE},
    {"onboardingButtonLabel", IDS_REWARDS_ONBOARDING_BUTTON_LABEL},
    {"onboardingErrorCountryDeclaredText",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DECLARE_COUNTRY},
    {"onboardingErrorDisabledText",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DISABLED},
    {"onboardingErrorDisabledTitle",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER_DISABLED},
    {"onboardingErrorText", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT},
    {"onboardingErrorTitle", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER},
    {"onboardingLearnMoreLabel", IDS_REWARDS_WIDGET_HOW_DOES_IT_WORK},
    {"onboardingSuccessLearnMoreLabel",
     IDS_BRAVE_REWARDS_ONBOARDING_HOW_DOES_IT_WORK},
    {"onboardingSuccessText", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_TEXT},
    {"onboardingSuccessTitle", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_HEADER},
    {"onboardingTermsText", IDS_REWARDS_ONBOARDING_TERMS_TEXT},
    {"onboardingTextItem1", IDS_REWARDS_ONBOARDING_TEXT_ITEM_1},
    {"onboardingTextItem2", IDS_REWARDS_ONBOARDING_TEXT_ITEM_2},
    {"onboardingTitle", IDS_REWARDS_ONBOARDING_TITLE},
    {"payoutAccountBalanceLabel", IDS_REWARDS_PAYOUT_ACCOUNT_BALANCE_LABEL},
    {"payoutAccountConnectedLabel", IDS_REWARDS_PAYOUT_ACCOUNT_CONNECTED_LABEL},
    {"payoutAccountDetailsTitle", IDS_REWARDS_PAYOUT_ACCOUNT_DETAILS_TITLE},
    {"payoutAccountLabel", IDS_REWARDS_PAYOUT_ACCOUNT_LABEL},
    {"payoutAccountLink", IDS_REWARDS_PAYOUT_ACCOUNT_LINK},
    {"payoutAccountLoggedOutTitle",
     IDS_REWARDS_PAYOUT_ACCOUNT_LOGGED_OUT_TITLE},
    {"payoutAccountLoginButtonLabel",
     IDS_REWARDS_PAYOUT_ACCOUNT_LOGIN_BUTTON_LABEL},
    {"payoutAccountLoginText", IDS_REWARDS_PAYOUT_ACCOUNT_LOGIN_TEXT},
    {"payoutAccountTitle", IDS_REWARDS_PAYOUT_ACCOUNT_TITLE},
    {"payoutAccountTooltip", IDS_REWARDS_PAYOUT_ACCOUNT_TOOLTIP},
    {"payoutCheckStatusLink", IDS_REWARDS_PAYMENT_CHECK_STATUS},
    {"payoutCompletedText", IDS_REWARDS_PAYMENT_COMPLETED},
    {"payoutPendingText", IDS_REWARDS_PAYMENT_PENDING},
    {"payoutProcessingText", IDS_REWARDS_PAYMENT_PROCESSING},
    {"payoutSupportLink", IDS_REWARDS_PAYMENT_SUPPORT},
    {"recurringListEmptyText", IDS_REWARDS_RECURRING_LIST_EMPTY_TEXT},
    {"recurringNextContributionLabel",
     IDS_REWARDS_RECURRING_NEXT_CONTRIBUTION_LABEL},
    {"recurringTitle", IDS_REWARDS_RECURRING_TITLE},
    {"removeButtonLabel", IDS_REWARDS_REMOVE_BUTTON_LABEL},
    {"resetButtonLabel", IDS_BRAVE_UI_RESET},
    {"resetConsentText", IDS_BRAVE_UI_REWARDS_RESET_CONSENT},
    {"resetRewardsText", IDS_BRAVE_UI_REWARDS_RESET_TEXT},
    {"resetRewardsTitle", IDS_BRAVE_UI_RESET_WALLET},
    {"rewardsPageTitle", IDS_REWARDS_PAGE_TITLE},
    {"selfCustodyInviteDismissButtonLabel", IDS_REWARDS_NOT_NOW},
    {"selfCustodyInviteText", IDS_REWARDS_SELF_CUSTODY_INVITE_TEXT},
    {"selfCustodyInviteTitle", IDS_REWARDS_SELF_CUSTODY_INVITE_HEADER},
    {"showAllButtonLabel", IDS_REWARDS_SHOW_ALL_BUTTON_LABEL},
    {"tosUpdateAcceptButtonLabel", IDS_REWARDS_TOS_UPDATE_BUTTON_LABEL},
    {"tosUpdateLink", IDS_REWARDS_TOS_UPDATE_LINK_TEXT},
    {"tosUpdateRequiredText", IDS_REWARDS_TOS_UPDATE_TEXT},
    {"tosUpdateRequiredTitle", IDS_REWARDS_TOS_UPDATE_HEADING},
    {"wdpCheckboxLabel", IDS_REWARDS_WDP_CHECKBOX_LABEL},
    {"wdpOptInText", IDS_REWARDS_WDP_OPT_IN_TEXT},
    {"wdpOptInTitle", IDS_REWARDS_WDP_OPT_IN_TITLE}};

}  // namespace

void CreateAndAddRewardsPageDataSource(content::WebUI& web_ui,
                                       const std::string& host) {
  auto* browser_context = web_ui.GetWebContents()->GetBrowserContext();
  auto* source = content::WebUIDataSource::CreateAndAdd(browser_context, host);

  webui::SetupWebUIDataSource(
      source,
      UNSAFE_TODO(
          base::make_span(kRewardsPageGenerated, kRewardsPageGeneratedSize)),
      IDR_NEW_BRAVE_REWARDS_PAGE_HTML);

  // Adaptive captcha challenges are displayed in an iframe on the Rewards
  // panel. In order to display these challenges we need to specify in CSP that
  // frames can be loaded from the adaptive captcha server URL.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc,
      "frame-src 'self' " +
          brave_adaptive_captcha::ServerUtil::GetInstance()->GetServerUrl("/") +
          ";");

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

  content::URLDataSource::Add(
      browser_context,
      std::make_unique<FaviconSource>(Profile::FromWebUI(&web_ui),
                                      chrome::FaviconUrlFormat::kFavicon2));
}

}  // namespace brave_rewards
