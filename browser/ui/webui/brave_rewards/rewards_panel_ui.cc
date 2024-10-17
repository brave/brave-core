/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_panel_handler.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_web_ui_utils.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_panel_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"
#include "ui/base/webui/web_ui_util.h"

namespace brave_rewards {

namespace {

static constexpr webui::LocalizedString kStrings[] = {
    {"attention", IDS_REWARDS_PANEL_ATTENTION},
    {"cancel", IDS_REWARDS_PANEL_CANCEL},
    {"captchaContactSupport", IDS_REWARDS_CAPTCHA_CONTACT_SUPPORT},
    {"captchaDismiss", IDS_REWARDS_CAPTCHA_DISMISS},
    {"captchaMaxAttemptsExceededText",
     IDS_REWARDS_CAPTCHA_MAX_ATTEMPTS_EXCEEDED_TEXT},
    {"captchaMaxAttemptsExceededTitle",
     IDS_REWARDS_CAPTCHA_MAX_ATTEMPTS_EXCEEDED_TITLE},
    {"captchaSolvedText", IDS_REWARDS_CAPTCHA_SOLVED_TEXT},
    {"captchaSolvedTitle", IDS_REWARDS_CAPTCHA_SOLVED_TITLE},
    {"changeAmount", IDS_REWARDS_PANEL_CHANGE_AMOUNT},
    {"connectAccountText", IDS_REWARDS_CONNECT_ACCOUNT_TEXT},
    {"connectContributeHeader", IDS_REWARDS_CONNECT_CONTRIBUTE_HEADER},
    {"connectContributeText", IDS_REWARDS_CONNECT_CONTRIBUTE_TEXT},
    {"headerTitle", IDS_REWARDS_PANEL_HEADER_TITLE},
    {"headerText", IDS_REWARDS_PANEL_HEADER_TEXT},
    {"includeInAutoContribute", IDS_REWARDS_PANEL_INCLUDE_IN_AUTO_CONTRIBUTE},
    {"learnMore", IDS_REWARDS_LEARN_MORE},
    {"learnMoreAboutBAT", IDS_REWARDS_PANEL_LEARN_MORE_ABOUT_BAT},
    {"loading", IDS_BRAVE_REWARDS_LOADING_LABEL},
    {"monthlyTip", IDS_REWARDS_PANEL_MONTHLY_TIP},
    {"notificationAutoContributeCompletedText",
     IDS_REWARDS_NOTIFICATION_AUTO_CONTRIBUTE_COMPLETED_TEXT},
    {"notificationAutoContributeCompletedTitle",
     IDS_REWARDS_NOTIFICATION_AUTO_CONTRIBUTE_COMPLETED_TITLE},
    {"notificationMonthlyContributionFailedText",
     IDS_REWARDS_NOTIFICATION_MONTHLY_CONTRIBUTION_FAILED_TEXT},
    {"notificationMonthlyContributionFailedTitle",
     IDS_REWARDS_NOTIFICATION_MONTHLY_CONTRIBUTION_FAILED_TITLE},
    {"notificationMonthlyTipCompletedText",
     IDS_REWARDS_NOTIFICATION_MONTHLY_TIP_COMPLETED_TEXT},
    {"notificationMonthlyTipCompletedTitle",
     IDS_REWARDS_NOTIFICATION_MONTHLY_TIP_COMPLETED_TITLE},
    {"notificationWalletDisconnectedAction",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_ACTION},
    {"notificationWalletDisconnectedText",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_TEXT},
    {"notificationWalletDisconnectedTitle",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_TITLE},
    {"notificationUpholdBATNotAllowedText",
     IDS_REWARDS_NOTIFICATION_UPHOLD_BAT_NOT_ALLOWED_TEXT},
    {"notificationUpholdBATNotAllowedTitle",
     IDS_REWARDS_NOTIFICATION_UPHOLD_BAT_NOT_ALLOWED_TITLE},
    {"notificationUpholdInsufficientCapabilitiesText",
     IDS_REWARDS_NOTIFICATION_UPHOLD_INSUFFICIENT_CAPABILITIES_TEXT},
    {"notificationUpholdInsufficientCapabilitiesTitle",
     IDS_REWARDS_NOTIFICATION_UPHOLD_INSUFFICIENT_CAPABILITIES_TITLE},
    {"ok", IDS_REWARDS_PANEL_OK},
    {"onboardingClose", IDS_BRAVE_REWARDS_ONBOARDING_CLOSE},
    {"onboardingDone", IDS_BRAVE_REWARDS_ONBOARDING_DONE},
    {"onboardingContinue", IDS_BRAVE_REWARDS_ONBOARDING_CONTINUE},
    {"onboardingEarnHeader", IDS_BRAVE_REWARDS_ONBOARDING_EARN_HEADER},
    {"onboardingEarnText", IDS_BRAVE_REWARDS_ONBOARDING_EARN_TEXT},
    {"onboardingHowDoesItWork", IDS_BRAVE_REWARDS_ONBOARDING_HOW_DOES_IT_WORK},
    {"onboardingErrorHeader", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER},
    {"onboardingErrorHeaderDisabled",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_HEADER_DISABLED},
    {"onboardingErrorText", IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT},
    {"onboardingErrorTextDeclareCountry",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DECLARE_COUNTRY},
    {"onboardingErrorTextDisabled",
     IDS_BRAVE_REWARDS_ONBOARDING_ERROR_TEXT_DISABLED},
    {"onboardingGeoHeader", IDS_BRAVE_REWARDS_ONBOARDING_GEO_HEADER},
    {"onboardingGeoSuccessHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_HEADER},
    {"onboardingGeoSuccessText", IDS_BRAVE_REWARDS_ONBOARDING_GEO_SUCCESS_TEXT},
    {"onboardingGeoText", IDS_BRAVE_REWARDS_ONBOARDING_GEO_TEXT},
    {"onboardingSelectCountry", IDS_BRAVE_REWARDS_ONBOARDING_SELECT_COUNTRY},
    {"onboardingHowDoesBraveRewardsWork",
     IDS_BRAVE_REWARDS_ONBOARDING_HOW_DOES_BRAVE_REWARDS_WORK},
    {"onboardingStartUsingRewards",
     IDS_BRAVE_REWARDS_ONBOARDING_START_USING_REWARDS},
    {"onboardingTerms", IDS_BRAVE_REWARDS_ONBOARDING_TERMS},
    {"platformPublisherTitle", IDS_REWARDS_PANEL_PLATFORM_PUBLISHER_TITLE},
    {"refreshStatus", IDS_REWARDS_PANEL_REFRESH_STATUS},
    {"rewardsConnectAccount", IDS_REWARDS_CONNECT_ACCOUNT},
    {"rewardsLearnMore", IDS_REWARDS_LEARN_MORE},
    {"rewardsLogInToSeeBalance", IDS_REWARDS_LOG_IN_TO_SEE_BALANCE},
    {"rewardsNotNow", IDS_REWARDS_NOT_NOW},
    {"rewardsPaymentCheckStatus", IDS_REWARDS_PAYMENT_CHECK_STATUS},
    {"rewardsPaymentCompleted", IDS_REWARDS_PAYMENT_COMPLETED},
    {"rewardsPaymentPending", IDS_REWARDS_PAYMENT_PENDING},
    {"rewardsPaymentProcessing", IDS_REWARDS_PAYMENT_PROCESSING},
    {"rewardsPaymentSupport", IDS_REWARDS_PAYMENT_SUPPORT},
    {"rewardsSelfCustodyInviteHeader", IDS_REWARDS_SELF_CUSTODY_INVITE_HEADER},
    {"rewardsSelfCustodyInviteText", IDS_REWARDS_SELF_CUSTODY_INVITE_TEXT},
    {"rewardsSettings", IDS_REWARDS_PANEL_REWARDS_SETTINGS},
    {"rewardsTosUpdateHeading", IDS_REWARDS_TOS_UPDATE_HEADING},
    {"rewardsTosUpdateText", IDS_REWARDS_TOS_UPDATE_TEXT},
    {"rewardsTosUpdateLinkText", IDS_REWARDS_TOS_UPDATE_LINK_TEXT},
    {"rewardsTosUpdateButtonLabel", IDS_REWARDS_TOS_UPDATE_BUTTON_LABEL},
    {"rewardsVBATNoticeText1", IDS_REWARDS_VBAT_NOTICE_TEXT1},
    {"rewardsVBATNoticeTitle1", IDS_REWARDS_VBAT_NOTICE_TITLE1},
    {"sendTip", IDS_REWARDS_PANEL_SEND_TIP},
    {"set", IDS_REWARDS_PANEL_SET},
    {"summary", IDS_REWARDS_PANEL_SUMMARY},
    {"tip", IDS_REWARDS_PANEL_TIP},
    {"unverifiedCreator", IDS_REWARDS_PANEL_UNVERIFIED_CREATOR},
    {"unverifiedText", IDS_REWARDS_PANEL_UNVERIFIED_TEXT},
    {"verifiedCreator", IDS_REWARDS_PANEL_VERIFIED_CREATOR},
    {"walletAccountLink", IDS_REWARDS_WALLET_ACCOUNT_LINK},
    {"walletAutoContribute", IDS_REWARDS_WALLET_AUTO_CONTRIBUTE},
    {"walletBalanceTitle", IDS_REWARDS_WALLET_BALANCE_TITLE},
    {"walletDisconnected", IDS_REWARDS_WALLET_DISCONNECTED},
    {"walletEarningInfoText", IDS_REWARDS_WIDGET_EARNING_INFO_TEXT},
    {"walletEstimatedEarnings", IDS_REWARDS_ESTIMATED_EARNINGS_TITLE},
    {"walletLogIntoYourAccount", IDS_REWARDS_WALLET_LOG_INTO_YOUR_ACCOUNT},
    {"walletManageAds", IDS_REWARDS_WALLET_MANAGE_ADS},
    {"walletMonthlyTips", IDS_REWARDS_WALLET_MONTHLY_TIPS},
    {"walletOneTimeTips", IDS_REWARDS_WALLET_ONE_TIME_TIPS},
    {"walletRewardsFromAds", IDS_REWARDS_WALLET_REWARDS_FROM_ADS},
    {"walletRewardsSummary", IDS_REWARDS_WALLET_REWARDS_SUMMARY},
    {"walletUnverified", IDS_REWARDS_WALLET_UNVERIFIED},
    {"walletVerified", IDS_REWARDS_WALLET_VERIFIED}};

}  // namespace

RewardsPanelUI::RewardsPanelUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true) {
  auto* profile = Profile::FromWebUI(web_ui);
  if (auto* browser = chrome::FindLastActiveWithProfile(profile)) {
    panel_coordinator_ = RewardsPanelCoordinator::FromBrowser(browser);
  }

  auto plural_string_handler = std::make_unique<PluralStringHandler>();
  plural_string_handler->AddLocalizedString("publisherCountText",
                                            IDS_REWARDS_PUBLISHER_COUNT_TEXT);
  web_ui->AddMessageHandler(std::move(plural_string_handler));

  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kBraveRewardsPanelHost);
  source->AddLocalizedStrings(kStrings);

  webui::SetupWebUIDataSource(
      source,
      UNSAFE_TODO(base::make_span(kBraveRewardsPanelGenerated,
                                  kBraveRewardsPanelGeneratedSize)),
      IDR_BRAVE_REWARDS_PANEL_HTML);

  // Adaptive captcha challenges are displayed in an iframe on the Rewards
  // panel. In order to display these challenges we need to specify in CSP that
  // frames can be loaded from the adaptive captcha server URL.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc,
      "frame-src 'self' " +
          brave_adaptive_captcha::ServerUtil::GetInstance()->GetServerUrl("/") +
          ";");

  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));
}

RewardsPanelUI::~RewardsPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(RewardsPanelUI)

void RewardsPanelUI::BindInterface(
    mojo::PendingReceiver<PanelHandlerFactory> receiver) {
  panel_factory_receiver_.reset();
  panel_factory_receiver_.Bind(std::move(receiver));
}

void RewardsPanelUI::CreatePanelHandler(
    mojo::PendingRemote<mojom::Panel> panel,
    mojo::PendingReceiver<mojom::PanelHandler> receiver) {
  DCHECK(panel);

  auto* profile = Profile::FromWebUI(web_ui());
  auto* rewards = RewardsServiceFactory::GetForProfile(profile);

  panel_handler_ = std::make_unique<RewardsPanelHandler>(
      std::move(panel), std::move(receiver), embedder(), rewards,
      panel_coordinator_);
}

RewardsPanelUIConfig::RewardsPanelUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  kBraveRewardsPanelHost) {}

bool RewardsPanelUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return !ShouldBlockRewardsWebUI(browser_context, GURL(kBraveRewardsPanelURL));
}

bool RewardsPanelUIConfig::ShouldAutoResizeHost() {
  return true;
}

}  // namespace brave_rewards
