/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/tip_panel_ui.h"

#include <memory>
#include <utility>

#include "brave/browser/ui/webui/brave_rewards/rewards_web_ui_utils.h"
#include "brave/browser/ui/webui/brave_rewards/tip_panel_handler.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/brave_rewards/resources/grit/tip_panel_generated_map.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"

namespace brave_rewards {

namespace {

static constexpr webui::LocalizedString kStrings[] = {
    {"balanceLabel", IDS_REWARDS_TIP_BALANCE_LABEL},
    {"monthlyToggleLabel", IDS_REWARDS_TIP_MONTHLY_TOGGLE_LABEL},
    {"feeNotice", IDS_REWARDS_TIP_FEE_NOTICE},
    {"termsOfService", IDS_BRAVE_REWARDS_ONBOARDING_TERMS},
    {"sendFormTitle", IDS_REWARDS_TIP_SEND_FORM_TITLE},
    {"sendButtonLabel", IDS_REWARDS_TIP_SEND_BUTTON_LABEL},
    {"sendWithButtonLabel", IDS_REWARDS_TIP_SEND_WITH_BUTTON_LABEL},
    {"web3ButtonLabel", IDS_REWARDS_TIP_WEB3_BUTTON_LABEL},
    {"verifiedTooltipTitle", IDS_REWARDS_TIP_VERIFIED_TOOLTIP_TITLE},
    {"verifiedTooltipText", IDS_REWARDS_TIP_VERIFIED_TOOLTIP_TEXT},
    {"monthlyTooltipText", IDS_REWARDS_TIP_MONTHLY_TOOLTIP_TEXT},
    {"learnMoreLabel", IDS_REWARDS_TIP_LEARN_MORE_LABEL},
    {"customAmountLabel", IDS_REWARDS_TIP_CUSTOM_AMOUNT_LABEL},
    {"monthlySetTitle", IDS_REWARDS_TIP_MONTHLY_SET_TITLE},
    {"monthlySetText", IDS_REWARDS_TIP_MONTHLY_SET_TEXT},
    {"providerMismatchTitle", IDS_REWARDS_TIP_PROVIDER_MISMATCH_TITLE},
    {"providerMismatchText", IDS_REWARDS_TIP_PROVIDER_MISMATCH_TEXT},
    {"providerMismatchWeb3Text", IDS_REWARDS_TIP_PROVIDER_MISMATCH_WEB3_TEXT},
    {"web3OnlyTitle", IDS_REWARDS_TIP_WEB3_ONLY_TITLE},
    {"reconnectTitle", IDS_REWARDS_TIP_RECONNECT_TITLE},
    {"reconnectText", IDS_REWARDS_TIP_RECONNECT_TEXT},
    {"reconnectWeb3Text", IDS_REWARDS_TIP_RECONNECT_WEB3_TEXT},
    {"reconnectButtonLabel", IDS_REWARDS_TIP_RECONNECT_BUTTON_LABEL},
    {"insufficientBalanceTitle", IDS_REWARDS_TIP_INSUFFICIENT_BALANCE_TITLE},
    {"insufficientBalanceText", IDS_REWARDS_TIP_INSUFFICIENT_BALANCE_TEXT},
    {"contributionFailedTitle", IDS_REWARDS_TIP_CONTRIBUTION_FAILED_TITLE},
    {"contributionFailedText", IDS_REWARDS_TIP_CONTRIBUTION_FAILED_TEXT},
    {"tryAgainButtonLabel", IDS_REWARDS_TIP_TRY_AGAIN_BUTTON_LABEL},
    {"contributionSentTitle", IDS_REWARDS_TIP_CONTRIBUTION_SENT_TITLE},
    {"contributionSentText", IDS_REWARDS_TIP_CONTRIBUTION_SENT_TEXT},
    {"shareButtonLabel", IDS_REWARDS_TIP_SHARE_BUTTON_LABEL},
    {"shareText", IDS_REWARDS_TIP_SHARE_TEXT},
    {"unexpectedErrorTitle", IDS_REWARDS_TIP_UNEXPECTED_ERROR_TITLE},
    {"unexpectedErrorText", IDS_REWARDS_TIP_UNEXPECTED_ERROR_TEXT},
    {"defaultCreatorDescription", IDS_REWARDS_TIP_DEFAULT_CREATOR_DESCRIPTION},
    {"platformPublisherTitle", IDS_REWARDS_PANEL_PLATFORM_PUBLISHER_TITLE},
    {"selfCustodyTitle", IDS_REWARDS_TIP_SELF_CUSTODY_TITLE},
    {"selfCustodyHeader", IDS_REWARDS_TIP_SELF_CUSTODY_HEADER},
    {"selfCustodyText", IDS_REWARDS_TIP_SELF_CUSTODY_TEXT},
    {"selfCustodySendButtonLabel",
     IDS_REWARDS_TIP_SELF_CUSTODY_SEND_BUTTON_LABEL},
    {"selfCustodyNoWeb3Label", IDS_REWARDS_TIP_SELF_CUSTODY_NO_WEB3_LABEL}};

}  // namespace

TipPanelUI::TipPanelUI(content::WebUI* web_ui)
    : TopChromeWebUIController(web_ui, true) {
  auto* source = content::WebUIDataSource::CreateAndAdd(
      web_ui->GetWebContents()->GetBrowserContext(), kBraveTipPanelHost);

  source->AddLocalizedStrings(kStrings);

  webui::SetupWebUIDataSource(
      source, base::make_span(kTipPanelGenerated, kTipPanelGeneratedSize),
      IDR_TIP_PANEL_HTML);

  source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ImgSrc,
      "img-src chrome://resources chrome://theme chrome://rewards-image "
      "chrome://favicon2 blob: data: 'self';");
}

TipPanelUI::~TipPanelUI() = default;

WEB_UI_CONTROLLER_TYPE_IMPL(TipPanelUI)

void TipPanelUI::BindInterface(
    mojo::PendingReceiver<TipPanelHandlerFactory> receiver) {
  factory_receiver_.reset();
  factory_receiver_.Bind(std::move(receiver));
}

void TipPanelUI::CreateHandler(
    mojo::PendingRemote<mojom::TipPanel> panel,
    mojo::PendingReceiver<mojom::TipPanelHandler> handler) {
  DCHECK(panel);
  handler_ = std::make_unique<TipPanelHandler>(std::move(panel),
                                               std::move(handler), embedder(),
                                               Profile::FromWebUI(web_ui()));
}

TipPanelUIConfig::TipPanelUIConfig()
    : DefaultTopChromeWebUIConfig(content::kChromeUIScheme,
                                  kBraveTipPanelHost) {}

bool TipPanelUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  return !ShouldBlockRewardsWebUI(browser_context, GURL(kBraveTipPanelURL));
}

bool TipPanelUIConfig::ShouldAutoResizeHost() {
  return true;
}

}  // namespace brave_rewards
