/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_panel_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_rewards/rewards_panel/rewards_panel_coordinator.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_rewards/rewards_panel_handler.h"
#include "brave/components/brave_adaptive_captcha/server_util.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_panel_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "services/network/public/mojom/content_security_policy.mojom.h"

namespace {

static constexpr webui::LocalizedString kStrings[] = {
    {"attention", IDS_REWARDS_PANEL_ATTENTION},
    {"braveTalkBraveRewardsDescription",
     IDS_REWARDS_BRAVE_TALK_BRAVE_REWARDS_DESCRIPTION},
    {"braveTalkCanStartFreeCall", IDS_REWARDS_BRAVE_TALK_CAN_START_FREE_CALL},
    {"braveTalkClickAnywhereToBraveTalk",
     IDS_REWARDS_BRAVE_TALK_CLICK_ANYWHERE_TO_BRAVE_TALK},
    {"braveTalkOptInRewardsTerms", IDS_REWARDS_BRAVE_TALK_OPT_IN_REWARDS_TERMS},
    {"braveTalkPrivateAdsDescription",
     IDS_REWARDS_BRAVE_TALK_PRIVATE_ADS_DESCRIPTION},
    {"braveTalkTurnOnPrivateAds", IDS_REWARDS_BRAVE_TALK_TURN_ON_PRIVATE_ADS},
    {"braveTalkTurnOnPrivateAdsToStartCall",
     IDS_REWARDS_BRAVE_TALK_TURN_ON_PRIVATE_ADS_TO_START_CALL},
    {"braveTalkTurnOnRewards", IDS_REWARDS_BRAVE_TALK_TURN_ON_REWARDS},
    {"braveTalkTurnOnRewardsToStartCall",
     IDS_REWARDS_BRAVE_TALK_TURN_ON_REWARDS_TO_START_CALL},
    {"braveTalkWantLearnMore", IDS_REWARDS_BRAVE_TALK_WANT_LEARN_MORE},
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
    {"grantCaptchaAmountAds", IDS_REWARDS_GRANT_CAPTCHA_AMOUNT_ADS},
    {"grantCaptchaAmountUGP", IDS_REWARDS_GRANT_CAPTCHA_AMOUNT_UGP},
    {"grantCaptchaErrorText", IDS_REWARDS_GRANT_CAPTCHA_ERROR_TEXT},
    {"grantCaptchaErrorTitle", IDS_REWARDS_GRANT_CAPTCHA_ERROR_TITLE},
    {"grantCaptchaExpiration", IDS_REWARDS_GRANT_CAPTCHA_EXPIRATION},
    {"grantCaptchaFailedTitle", IDS_REWARDS_GRANT_CAPTCHA_FAILED_TITLE},
    {"grantCaptchaHint", IDS_REWARDS_GRANT_CAPTCHA_HINT},
    {"grantCaptchaPassedTextAds", IDS_REWARDS_GRANT_CAPTCHA_PASSED_TEXT_ADS},
    {"grantCaptchaPassedTextUGP", IDS_REWARDS_GRANT_CAPTCHA_PASSED_TEXT_UGP},
    {"grantCaptchaPassedTitleAds", IDS_REWARDS_GRANT_CAPTCHA_PASSED_TITLE_ADS},
    {"grantCaptchaPassedTitleUGP", IDS_REWARDS_GRANT_CAPTCHA_PASSED_TITLE_UGP},
    {"grantCaptchaTitle", IDS_REWARDS_GRANT_CAPTCHA_TITLE},
    {"includeInAutoContribute", IDS_REWARDS_PANEL_INCLUDE_IN_AUTO_CONTRIBUTE},
    {"monthlyTip", IDS_REWARDS_PANEL_MONTHLY_TIP},
    {"notificationAddFunds", IDS_REWARDS_NOTIFICATION_ADD_FUNDS},
    {"notificationAddFundsText", IDS_REWARDS_NOTIFICATION_ADD_FUNDS_TEXT},
    {"notificationAddFundsTitle", IDS_REWARDS_NOTIFICATION_ADD_FUNDS_TITLE},
    {"notificationAdGrantAmount", IDS_REWARDS_NOTIFICATION_AD_GRANT_AMOUNT},
    {"notificationAdGrantTitle", IDS_REWARDS_NOTIFICATION_AD_GRANT_TITLE},
    {"notificationAutoContributeCompletedText",
     IDS_REWARDS_NOTIFICATION_AUTO_CONTRIBUTE_COMPLETED_TEXT},
    {"notificationAutoContributeCompletedTitle",
     IDS_REWARDS_NOTIFICATION_AUTO_CONTRIBUTE_COMPLETED_TITLE},
    {"notificationBackupWalletAction",
     IDS_REWARDS_NOTIFICATION_BACKUP_WALLET_ACTION},
    {"notificationBackupWalletText",
     IDS_REWARDS_NOTIFICATION_BACKUP_WALLET_TEXT},
    {"notificationBackupWalletTitle",
     IDS_REWARDS_NOTIFICATION_BACKUP_WALLET_TITLE},
    {"notificationClaimRewards", IDS_REWARDS_NOTIFICATION_CLAIM_REWARDS},
    {"notificationClaimTokens", IDS_REWARDS_NOTIFICATION_CLAIM_TOKENS},
    {"notificationGrantDaysRemaining",
     IDS_REWARDS_NOTIFICATION_GRANT_DAYS_REMAINING},
    {"notificationInsufficientFundsText",
     IDS_REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS_TEXT},
    {"notificationMonthlyContributionFailedText",
     IDS_REWARDS_NOTIFICATION_MONTHLY_CONTRIBUTION_FAILED_TEXT},
    {"notificationMonthlyContributionFailedTitle",
     IDS_REWARDS_NOTIFICATION_MONTHLY_CONTRIBUTION_FAILED_TITLE},
    {"notificationMonthlyTipCompletedText",
     IDS_REWARDS_NOTIFICATION_MONTHLY_TIP_COMPLETED_TEXT},
    {"notificationMonthlyTipCompletedTitle",
     IDS_REWARDS_NOTIFICATION_MONTHLY_TIP_COMPLETED_TITLE},
    {"notificationPendingTipFailedText",
     IDS_REWARDS_NOTIFICATION_PENDING_TIP_FAILED_TEXT},
    {"notificationPendingTipFailedTitle",
     IDS_REWARDS_NOTIFICATION_PENDING_TIP_FAILED_TITLE},
    {"notificationPublisherVerifiedText",
     IDS_REWARDS_NOTIFICATION_PUBLISHER_VERIFIED_TEXT},
    {"notificationPublisherVerifiedTitle",
     IDS_REWARDS_NOTIFICATION_PUBLISHER_VERIFIED_TITLE},
    {"notificationReconnect", IDS_REWARDS_NOTIFICATION_RECONNECT},
    {"notificationWalletDisconnectedAction",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_ACTION},
    {"notificationWalletDisconnectedText",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_TEXT},
    {"notificationWalletDisconnectedTitle",
     IDS_REWARDS_NOTIFICATION_WALLET_DISCONNECTED_TITLE},
    {"notificationTokenGrantTitle", IDS_REWARDS_NOTIFICATION_TOKEN_GRANT_TITLE},
    {"notificationWalletVerifiedText",
     IDS_REWARDS_NOTIFICATION_WALLET_VERIFIED_TEXT},
    {"notificationWalletVerifiedTitle",
     IDS_REWARDS_NOTIFICATION_WALLET_VERIFIED_TITLE},
    {"ok", IDS_REWARDS_PANEL_OK},
    {"onboardingEarnHeader", IDS_BRAVE_REWARDS_ONBOARDING_EARN_HEADER},
    {"onboardingEarnText", IDS_BRAVE_REWARDS_ONBOARDING_EARN_TEXT},
    {"onboardingPanelAcHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_AC_HEADER},
    {"onboardingPanelAcText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_AC_TEXT},
    {"onboardingPanelAdsHeader", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_ADS_HEADER},
    {"onboardingPanelAdsText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_ADS_TEXT},
    {"onboardingPanelBitflyerLearnMore",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_BITFLYER_LEARN_MORE},
    {"onboardingPanelBitflyerNote",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_BITFLYER_NOTE},
    {"onboardingPanelBitflyerText",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_BITFLYER_TEXT},
    {"onboardingPanelCompleteHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_COMPLETE_HEADER},
    {"onboardingPanelCompleteText",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_COMPLETE_TEXT},
    {"onboardingPanelRedeemHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_REDEEM_HEADER},
    {"onboardingPanelRedeemText",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_REDEEM_TEXT},
    {"onboardingPanelScheduleHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SCHEDULE_HEADER},
    {"onboardingPanelScheduleText",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SCHEDULE_TEXT},
    {"onboardingPanelSetupHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SETUP_HEADER},
    {"onboardingPanelSetupText", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_SETUP_TEXT},
    {"onboardingPanelTippingHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_TIPPING_HEADER},
    {"onboardingPanelTippingText",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_TIPPING_TEXT},
    {"onboardingPanelVerifyHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_VERIFY_HEADER},
    {"onboardingPanelVerifyLater",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_VERIFY_LATER},
    {"onboardingPanelVerifyNow", IDS_BRAVE_REWARDS_ONBOARDING_PANEL_VERIFY_NOW},
    {"onboardingPanelVerifySubtext",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_VERIFY_SUBTEXT},
    {"onboardingPanelWelcomeHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_WELCOME_HEADER},
    {"onboardingPanelWelcomeText",
     IDS_BRAVE_REWARDS_ONBOARDING_PANEL_WELCOME_TEXT},
    {"onboardingSetupAdsHeader", IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_HEADER},
    {"onboardingSetupAdsSubheader",
     IDS_BRAVE_REWARDS_ONBOARDING_SETUP_ADS_SUBHEADER},
    {"onboardingSetupContributeHeader",
     IDS_BRAVE_REWARDS_ONBOARDING_SETUP_CONTRIBUTE_HEADER},
    {"onboardingSetupContributeSubheader",
     IDS_BRAVE_REWARDS_ONBOARDING_SETUP_CONTRIBUTE_SUBHEADER},
    {"onboardingStartUsingRewards",
     IDS_BRAVE_REWARDS_ONBOARDING_START_USING_REWARDS},
    {"onboardingTakeTour", IDS_BRAVE_REWARDS_ONBOARDING_TAKE_TOUR},
    {"onboardingTerms", IDS_BRAVE_REWARDS_ONBOARDING_TERMS},
    {"onboardingTourBack", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_BACK},
    {"onboardingTourBegin", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_BEGIN},
    {"onboardingTourContinue", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_CONTINUE},
    {"onboardingTourDone", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_DONE},
    {"onboardingTourSkip", IDS_BRAVE_REWARDS_ONBOARDING_TOUR_SKIP},
    {"onboardingTourSkipForNow",
     IDS_BRAVE_REWARDS_ONBOARDING_TOUR_SKIP_FOR_NOW},
    {"pendingTipText", IDS_REWARDS_PANEL_PENDING_TIP_TEXT},
    {"pendingTipTitle", IDS_REWARDS_PANEL_PENDING_TIP_TITLE},
    {"pendingTipTitleRegistered",
     IDS_REWARDS_PANEL_PENDING_TIP_TITLE_REGISTERED},
    {"platformPublisherTitle", IDS_REWARDS_PANEL_PLATFORM_PUBLISHER_TITLE},
    {"refreshStatus", IDS_REWARDS_PANEL_REFRESH_STATUS},
    {"rewardsLogInToSeeBalance", IDS_REWARDS_LOG_IN_TO_SEE_BALANCE},
    {"rewardsPaymentCheckStatus", IDS_REWARDS_PAYMENT_CHECK_STATUS},
    {"rewardsPaymentCompleted", IDS_REWARDS_PAYMENT_COMPLETED},
    {"rewardsPaymentPending", IDS_REWARDS_PAYMENT_PENDING},
    {"rewardsPaymentProcessing", IDS_REWARDS_PAYMENT_PROCESSING},
    {"sendTip", IDS_REWARDS_PANEL_SEND_TIP},
    {"set", IDS_REWARDS_PANEL_SET},
    {"summary", IDS_REWARDS_PANEL_SUMMARY},
    {"tip", IDS_REWARDS_PANEL_TIP},
    {"unverifiedCreator", IDS_REWARDS_PANEL_UNVERIFIED_CREATOR},
    {"verifiedCreator", IDS_REWARDS_PANEL_VERIFIED_CREATOR},
    {"walletAccountLink", IDS_REWARDS_WALLET_ACCOUNT_LINK},
    {"walletAddFunds", IDS_REWARDS_WALLET_ADD_FUNDS},
    {"walletAutoContribute", IDS_REWARDS_WALLET_AUTO_CONTRIBUTE},
    {"walletDisconnected", IDS_REWARDS_WALLET_DISCONNECTED},
    {"walletDisconnectLink", IDS_REWARDS_WALLET_DISCONNECT_LINK},
    {"walletEstimatedEarnings", IDS_REWARDS_WALLET_ESTIMATED_EARNINGS},
    {"walletLogIntoYourAccount", IDS_REWARDS_WALLET_LOG_INTO_YOUR_ACCOUNT},
    {"walletMonthlyTips", IDS_REWARDS_WALLET_MONTHLY_TIPS},
    {"walletOneTimeTips", IDS_REWARDS_WALLET_ONE_TIME_TIPS},
    {"walletPending", IDS_REWARDS_WALLET_PENDING},
    {"walletPendingText", IDS_REWARDS_WALLET_PENDING_TEXT},
    {"walletRewardsFromAds", IDS_REWARDS_WALLET_REWARDS_FROM_ADS},
    {"walletRewardsSummary", IDS_REWARDS_WALLET_REWARDS_SUMMARY},
    {"walletUnverified", IDS_REWARDS_WALLET_UNVERIFIED},
    {"walletVerified", IDS_REWARDS_WALLET_VERIFIED},
    {"walletYourBalance", IDS_REWARDS_WALLET_YOUR_BALANCE}};

}  // namespace

RewardsPanelUI::RewardsPanelUI(content::WebUI* web_ui)
    : MojoBubbleWebUIController(web_ui, true) {
  auto* profile = Profile::FromWebUI(web_ui);
  if (auto* browser = chrome::FindLastActiveWithProfile(profile)) {
    panel_coordinator_ =
        brave_rewards::RewardsPanelCoordinator::FromBrowser(browser);
  }

  auto* source = content::WebUIDataSource::Create(kBraveRewardsPanelHost);
  source->AddLocalizedStrings(kStrings);

  webui::SetupWebUIDataSource(source,
                              base::make_span(kBraveRewardsPanelGenerated,
                                              kBraveRewardsPanelGeneratedSize),
                              IDR_BRAVE_REWARDS_PANEL_HTML);

  // Adaptive captcha challenges are displayed in an iframe on the Rewards
  // panel. In order to display these challenges we need to specify in CSP that
  // frames can be loaded from the adaptive captcha server URL.
  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ChildSrc,
      "frame-src 'self' " + brave_adaptive_captcha::GetServerUrl("/") + ";");

  content::WebUIDataSource::Add(web_ui->GetWebContents()->GetBrowserContext(),
                                source);

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
    mojo::PendingRemote<brave_rewards::mojom::Panel> panel,
    mojo::PendingReceiver<brave_rewards::mojom::PanelHandler> receiver) {
  DCHECK(panel);

  auto* profile = Profile::FromWebUI(web_ui());
  auto* rewards = brave_rewards::RewardsServiceFactory::GetForProfile(profile);

  panel_handler_ = std::make_unique<RewardsPanelHandler>(
      std::move(panel), std::move(receiver), embedder(), rewards,
      panel_coordinator_);
}
