/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"

#include <memory>
#include <string>
#include <utility>

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"

#if !defined(OS_ANDROID)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_internals_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#else
#include "components/brave_rewards/settings/resources/grit/brave_rewards_internals_generated_map.h"
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#endif

namespace {

class RewardsInternalsDOMHandler : public content::WebUIMessageHandler {
 public:
  RewardsInternalsDOMHandler();
  ~RewardsInternalsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

 private:
  bool IsRewardsEnabled() const;
  void HandleGetRewardsEnabled(const base::ListValue* args);
  void HandleGetRewardsInternalsInfo(const base::ListValue* args);
  void OnGetRewardsInternalsInfo(
      std::unique_ptr<brave_rewards::RewardsInternalsInfo> info);
  void OnPreferenceChanged();

  brave_rewards::RewardsService* rewards_service_;  // NOT OWNED
  Profile* profile_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  base::WeakPtrFactory<RewardsInternalsDOMHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RewardsInternalsDOMHandler);
};

RewardsInternalsDOMHandler::RewardsInternalsDOMHandler()
    : rewards_service_(nullptr), profile_(nullptr), weak_ptr_factory_(this) {}

RewardsInternalsDOMHandler::~RewardsInternalsDOMHandler() {}

void RewardsInternalsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getRewardsEnabled",
      base::BindRepeating(&RewardsInternalsDOMHandler::HandleGetRewardsEnabled,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getRewardsInternalsInfo",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::HandleGetRewardsInternalsInfo,
          base::Unretained(this)));
}

void RewardsInternalsDOMHandler::Init() {
  profile_ = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  PrefService* prefs = profile_->GetPrefs();
  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(
      brave_rewards::prefs::kBraveRewardsEnabled,
      base::BindRepeating(&RewardsInternalsDOMHandler::OnPreferenceChanged,
                          base::Unretained(this)));
}

bool RewardsInternalsDOMHandler::IsRewardsEnabled() const {
  DCHECK(profile_);
  return profile_->GetPrefs()->GetBoolean(
      brave_rewards::prefs::kBraveRewardsEnabled);
}

void RewardsInternalsDOMHandler::HandleGetRewardsEnabled(
    const base::ListValue* args) {
  if (!web_ui()->CanCallJavascript())
    return;
  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.onGetRewardsEnabled",
      base::Value(IsRewardsEnabled()));
}

void RewardsInternalsDOMHandler::HandleGetRewardsInternalsInfo(
    const base::ListValue* args) {
  rewards_service_->GetRewardsInternalsInfo(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetRewardsInternalsInfo,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnPreferenceChanged() {
  if (!web_ui()->CanCallJavascript())
    return;
  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.onGetRewardsEnabled",
      base::Value(IsRewardsEnabled()));
}

void RewardsInternalsDOMHandler::OnGetRewardsInternalsInfo(
    std::unique_ptr<brave_rewards::RewardsInternalsInfo> info) {
  if (!web_ui()->CanCallJavascript())
    return;
  base::DictionaryValue info_dict;
  if (info) {
    info_dict.SetString("walletPaymentId", info->payment_id);
    info_dict.SetBoolean("isKeyInfoSeedValid", info->is_key_info_seed_valid);
    auto current_reconciles = std::make_unique<base::ListValue>();
    for (const auto& item : info->current_reconciles) {
      auto reconcile_info = std::make_unique<base::DictionaryValue>();
      reconcile_info->SetString("viewingId", item.second.viewing_id_);
      reconcile_info->SetString("amount", item.second.amount_);
      reconcile_info->SetInteger("retryStep", item.second.retry_step_);
      reconcile_info->SetInteger("retryLevel", item.second.retry_level_);
      current_reconciles->Append(std::move(reconcile_info));
    }
    info_dict.SetList("currentReconciles", std::move(current_reconciles));
    info_dict.SetString("personaId", info->persona_id);
    info_dict.SetString("userId", info->user_id);
    info_dict.SetInteger("bootStamp", info->boot_stamp);
  }
  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.onGetRewardsInternalsInfo", info_dict);
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
              kBraveRewardsInternalsGenerated,
              kBraveRewardsInternalsGeneratedSize,
#endif
              IDR_BRAVE_REWARDS_INTERNALS_HTML) {
  auto handler_owner = std::make_unique<RewardsInternalsDOMHandler>();
  RewardsInternalsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsInternalsUI::~BraveRewardsInternalsUI() {
}
