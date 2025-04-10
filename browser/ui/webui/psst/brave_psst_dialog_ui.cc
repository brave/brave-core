/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/brave_psst_dialog_ui.h"

#include "base/check.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/psst/browser/core/psst_consent_dialog.mojom.h"
#include "brave/components/psst/resources/grit/brave_psst_dialog_generated.h"
#include "brave/components/psst/resources/grit/brave_psst_dialog_generated_map.h"
#include "brave/components/psst/resources/grit/brave_psst_resources.h"
#include "components/grit/brave_components_resources.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/js/grit/mojo_bindings_resources.h"

using content::WebUIMessageHandler;

namespace psst {

RewardsDOMHandler::RewardsDOMHandler()= default;
RewardsDOMHandler::~RewardsDOMHandler() = default;

void RewardsDOMHandler::RegisterMessages() {
//FireWebUIListener("std::string_view event_name", 0);

  // web_ui()->RegisterMessageCallback(
  //   "webcompat_reporter.getCapturedScreenshot",
  //   base::BindRepeating(
  //       &RewardsDOMHandler::HandleCloseDialog,
  //       base::Unretained(this)));

//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.restartBrowser",
//       base::BindRepeating(&RewardsDOMHandler::RestartBrowser,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.isInitialized",
//       base::BindRepeating(&RewardsDOMHandler::IsInitialized,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getUserType",
//       base::BindRepeating(&RewardsDOMHandler::GetUserType,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.acceptTermsOfServiceUpdate",
//       base::BindRepeating(&RewardsDOMHandler::AcceptTermsOfServiceUpdate,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.isTermsOfServiceUpdateRequired",
//       base::BindRepeating(&RewardsDOMHandler::IsTermsOfServiceUpdateRequired,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getRewardsParameters",
//       base::BindRepeating(&RewardsDOMHandler::GetRewardsParameters,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.isAutoContributeSupported",
//       base::BindRepeating(&RewardsDOMHandler::IsAutoContributeSupported,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getAutoContributeProperties",
//       base::BindRepeating(&RewardsDOMHandler::GetAutoContributeProperties,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getReconcileStamp",
//       base::BindRepeating(&RewardsDOMHandler::GetReconcileStamp,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.saveSetting",
//       base::BindRepeating(&RewardsDOMHandler::SaveSetting,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.excludePublisher",
//       base::BindRepeating(&RewardsDOMHandler::ExcludePublisher,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.restorePublishers",
//       base::BindRepeating(&RewardsDOMHandler::RestorePublishers,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.restorePublisher",
//       base::BindRepeating(&RewardsDOMHandler::RestorePublisher,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getContributionAmount",
//       base::BindRepeating(&RewardsDOMHandler::GetAutoContributionAmount,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.removeRecurringTip",
//       base::BindRepeating(&RewardsDOMHandler::RemoveRecurringTip,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getRecurringTips",
//       base::BindRepeating(&RewardsDOMHandler::GetRecurringTips,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getOneTimeTips",
//       base::BindRepeating(&RewardsDOMHandler::GetOneTimeTips,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getContributionList",
//       base::BindRepeating(&RewardsDOMHandler::GetContributionList,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getAdsData",
//       base::BindRepeating(&RewardsDOMHandler::GetAdsData,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getAdsHistory",
//       base::BindRepeating(&RewardsDOMHandler::GetAdsHistory,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.toggleAdThumbUp",
//       base::BindRepeating(&RewardsDOMHandler::ToggleAdThumbUp,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.toggleAdThumbDown",
//       base::BindRepeating(&RewardsDOMHandler::ToggleAdThumbDown,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.toggleAdOptIn",
//       base::BindRepeating(&RewardsDOMHandler::ToggleAdOptIn,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.toggleAdOptOut",
//       base::BindRepeating(&RewardsDOMHandler::ToggleAdOptOut,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.toggleSavedAd",
//       base::BindRepeating(&RewardsDOMHandler::ToggleSavedAd,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.toggleFlaggedAd",
//       base::BindRepeating(&RewardsDOMHandler::ToggleFlaggedAd,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.saveAdsSetting",
//       base::BindRepeating(&RewardsDOMHandler::SaveAdsSetting,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getStatement",
//       base::BindRepeating(&RewardsDOMHandler::GetStatement,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getExcludedSites",
//       base::BindRepeating(&RewardsDOMHandler::GetExcludedSites,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.fetchBalance",
//       base::BindRepeating(&RewardsDOMHandler::FetchBalance,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getExternalWallet",
//       base::BindRepeating(&RewardsDOMHandler::GetExternalWallet,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.connectExternalWallet",
//       base::BindRepeating(&RewardsDOMHandler::ConnectExternalWallet,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getBalanceReport",
//       base::BindRepeating(&RewardsDOMHandler::GetBalanceReport,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getCountryCode",
//       base::BindRepeating(&RewardsDOMHandler::GetCountryCode,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.completeReset",
//       base::BindRepeating(&RewardsDOMHandler::CompleteReset,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getOnboardingStatus",
//       base::BindRepeating(&RewardsDOMHandler::GetOnboardingStatus,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.enableRewards",
//       base::BindRepeating(&RewardsDOMHandler::EnableRewards,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getExternalWalletProviders",
//       base::BindRepeating(&RewardsDOMHandler::GetExternalWalletProviders,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.beginExternalWalletLogin",
//       base::BindRepeating(&RewardsDOMHandler::BeginExternalWalletLogin,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.reconnectExternalWallet",
//       base::BindRepeating(&RewardsDOMHandler::ReconnectExternalWallet,
//                           base::Unretained(this)));
//   web_ui()->RegisterMessageCallback(
//       "brave_rewards.getIsUnsupportedRegion",
//       base::BindRepeating(&RewardsDOMHandler::GetIsUnsupportedRegion,
//                           base::Unretained(this)));

//   web_ui()->RegisterMessageCallback(
//       "getPluralString",
//       base::BindRepeating(&RewardsDOMHandler::GetPluralString,
//                           base::Unretained(this)));
}

void RewardsDOMHandler::Init() {
 // Profile* profile = Profile::FromWebUI(web_ui());

//   rewards_service_ =
//       brave_rewards::RewardsServiceFactory::GetForProfile(profile);
//   ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);

//   if (rewards_service_) {
//     rewards_service_->OnRewardsPageShown();
//   }

  // Configure a pref change registrar to update brave://rewards when settings
  // are changed via brave://settings
  //sInitPrefChangeRegistrar();
}

void RewardsDOMHandler::BindInterface(mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper> pending_receiver) {
  LOG(INFO) << "[PSST] RewardsDOMHandler::BindInterface #100 receivers.size:" << receivers_.size();
  receivers_.Add(this, std::move(pending_receiver));
  LOG(INFO) << "[PSST] RewardsDOMHandler::BindInterface #200 receivers.size:" << receivers_.size();
}

void RewardsDOMHandler::SetClientPage(::mojo::PendingRemote<psst_consent_dialog::mojom::PsstConsentDialog> dialog) {
  client_page_.Bind(std::move(dialog));
}

BravePsstDialogUI::BravePsstDialogUI(content::WebUI* web_ui)
    : ConstrainedWebDialogUI(web_ui) {
  LOG(INFO) << "[PSST] BravePsstDialogUI Created #100";
  auto* source = CreateAndAddWebUIDataSource(
      web_ui, kBravePsstHost, kBravePsstDialogGenerated, IDR_BRAVE_PSST_DIALOG_HTML);

  LOG(INFO) << "[PSST] BravePsstDialogUI Created source:" << (source == nullptr);
  
  // #if BUILDFLAG(IS_ANDROID)
  // source->AddBoolean("isAndroid", true);
  // #else
  // source->AddBoolean("isAndroid", false);
  // #endif

  //source->AddResourcePath("mojo/public/js/mojo_bindings.js", IDR_MOJO_MOJO_BINDINGS_JS);

  auto handler_owner = std::make_unique<RewardsDOMHandler>();
  handler_ = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler_->Init();
}

BravePsstDialogUI::~BravePsstDialogUI() = default;

void BravePsstDialogUI::BindInterface(mojo::PendingReceiver<psst_consent_dialog::mojom::PsstConsentHelper>
  pending_receiver) {
    DCHECK(handler_);
LOG(INFO) << "[PSST] BravePsstDialogUI::BindInterface";
handler_->BindInterface(std::move(pending_receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(BravePsstDialogUI)
}  // namespace psst