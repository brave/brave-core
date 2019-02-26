/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"

#include "base/json/json_writer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_ui.h"

#if !defined(OS_ANDROID)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_internals_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#else
#include "components/brave_rewards/settings/resources/grit/brave_rewards_settings_generated_map.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#endif

namespace {
std::string BooleanToString(bool value) {
  return value ? "true" : "false";
}
}  // namespace

BraveRewardsInternalsUI::BraveRewardsInternalsUI(content::WebUI* web_ui,
                                                 const std::string& name)
    : BasicUI(web_ui,
              name,
#if !defined(OS_ANDROID)
              kBraveRewardsInternalsGenerated,
              kBraveRewardsInternalsGeneratedSize,
#else
              kBraveRewardsInternalsSettingsGenerated,
              kBraveRewardsInternalsSettingsGeneratedSize,
#endif
              IDR_BRAVE_REWARDS_INTERNALS_HTML),
      profile_(Profile::FromWebUI(web_ui)) {
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  rewards_service_->AddObserver(this);

  rewards_service_->GetRewardsInternalsInfo(
      base::Bind(&BraveRewardsInternalsUI::OnGetRewardsInternalsInfo,
                 base::Unretained(this)));
}

BraveRewardsInternalsUI::~BraveRewardsInternalsUI() {
  rewards_service_->RemoveObserver(this);
}

void BraveRewardsInternalsUI::CustomizeWebUIProperties(
    content::RenderViewHost* render_view_host) {
  DCHECK(IsSafeToSetWebUIProperties());

  if (render_view_host) {
    render_view_host->SetWebUIProperty("isRewardsEnabled",
                                       BooleanToString(IsRewardsEnabled()));
    if (internals_info_) {
      render_view_host->SetWebUIProperty("walletPaymentId",
                                         internals_info_->payment_id);
      render_view_host->SetWebUIProperty(
          "isKeyInfoSeedValid",
          BooleanToString(internals_info_->is_key_info_seed_valid));

      base::ListValue current_reconciles;
      for (const auto& item : internals_info_->current_reconciles) {
        auto current_reconcile_info = std::make_unique<base::DictionaryValue>();
        current_reconcile_info->SetString("viewingId", item.second.viewing_id_);
        current_reconcile_info->SetString("amount", item.second.amount_);
        current_reconcile_info->SetInteger("retryStep",
                                           item.second.retry_step_);
        current_reconcile_info->SetInteger("retryLevel",
                                           item.second.retry_level_);
        current_reconciles.Append(std::move(current_reconcile_info));
      }

      std::string json;
      base::JSONWriter::Write(current_reconciles, &json);
      render_view_host->SetWebUIProperty("currentReconciles", json);
    }
  }
}

bool BraveRewardsInternalsUI::IsRewardsEnabled() const {
  DCHECK(profile_);
  return profile_->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled);
}

void BraveRewardsInternalsUI::UpdateWebUIProperties() {
  if (IsSafeToSetWebUIProperties()) {
    CustomizeWebUIProperties(GetRenderViewHost());
    web_ui()->CallJavascriptFunctionUnsafe(
        "brave_rewards_internals.stateUpdated");
  }
}

void BraveRewardsInternalsUI::OnRewardsMainEnabled(
    brave_rewards::RewardsService* rewards_service,
    bool rewards_main_enabled) {
  UpdateWebUIProperties();
}

void BraveRewardsInternalsUI::OnWalletInitialized(
    brave_rewards::RewardsService* rewards_service,
    int error_code) {
  DCHECK(rewards_service_);
  rewards_service_->GetRewardsInternalsInfo(
      base::Bind(&BraveRewardsInternalsUI::OnGetRewardsInternalsInfo,
                 base::Unretained(this)));
}

void BraveRewardsInternalsUI::OnGetRewardsInternalsInfo(
    std::unique_ptr<brave_rewards::RewardsInternalsInfo> info) {
  internals_info_ = std::move(info);
  UpdateWebUIProperties();
}
