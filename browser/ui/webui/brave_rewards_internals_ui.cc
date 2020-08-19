/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_internals_generated_map.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"

#if defined(BRAVE_CHROMIUM_BUILD)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#else
#include "components/grit/components_resources.h"
#include "components/grit/components_scaled_resources.h"
#endif

namespace {

const int g_partial_log_max_lines = 5000;

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
  void GetBalance(const base::ListValue* args);
  void OnGetBalance(
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance);
  void GetContributions(const base::ListValue* args);
  void OnGetContributions(
      const std::vector<brave_rewards::ContributionInfo>& list);
  void GetPromotions(const base::ListValue* args);
  void OnGetPromotions(const std::vector<brave_rewards::Promotion>& list);
  void GetPartialLog(const base::ListValue* args);
  void OnGetPartialLog(const std::string& log);
  void GetFulllLog(const base::ListValue* args);
  void OnGetFulllLog(const std::string& log);
  void ClearLog(const base::ListValue* args);
  void OnClearLog(const bool success);
  void GetExternalWallet(const base::ListValue* args);
  void OnGetExternalWallet(
      int32_t result,
      std::unique_ptr<brave_rewards::ExternalWallet> wallet);
  void GetEventLogs(const base::ListValue* args);
  void OnGetEventLogs(const std::vector<brave_rewards::EventLog>&);

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
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getBalance",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetBalance,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getContributions",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetContributions,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getPromotions",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetPromotions,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getPartialLog",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetPartialLog,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getFullLog",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetFulllLog,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.clearLog",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::ClearLog,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getExternalWallet",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetExternalWallet,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getEventLogs",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::GetEventLogs,
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
    info_dict.SetInteger("bootStamp", info->boot_stamp);
  }
  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.onGetRewardsInternalsInfo", info_dict);
}

void RewardsInternalsDOMHandler::GetBalance(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }
  rewards_service_->FetchBalance(base::BindOnce(
      &RewardsInternalsDOMHandler::OnGetBalance,
      weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetBalance(
    int32_t result,
    std::unique_ptr<brave_rewards::Balance> balance) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value balance_value(base::Value::Type::DICTIONARY);

  if (result == 0 && balance) {
    balance_value.SetDoubleKey("total", balance->total);

    base::Value wallets(base::Value::Type::DICTIONARY);
    for (auto const& wallet : balance->wallets) {
      wallets.SetDoubleKey(wallet.first, wallet.second);
    }
    balance_value.SetKey("wallets", std::move(wallets));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.balance",
      std::move(balance_value));
}

void RewardsInternalsDOMHandler::GetContributions(const base::ListValue *args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetAllContributions(base::BindOnce(
      &RewardsInternalsDOMHandler::OnGetContributions,
      weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetContributions(
    const std::vector<brave_rewards::ContributionInfo>& list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value contributions(base::Value::Type::LIST);
  for (const auto & item : list) {
    base::Value contribution(base::Value::Type::DICTIONARY);
    contribution.SetStringKey("id", item.contribution_id);
    contribution.SetDoubleKey("amount", item.amount);
    contribution.SetIntKey("type", item.type);
    contribution.SetIntKey("step", item.step);
    contribution.SetIntKey("retryCount", item.retry_count);
    contribution.SetIntKey("createdAt", item.created_at);
    contribution.SetIntKey("processor", item.processor);
    base::Value publishers(base::Value::Type::LIST);
    for (const auto& publisher_item : item.publishers) {
      base::Value publisher(base::Value::Type::DICTIONARY);
      publisher.SetStringKey(
          "contributionId",
          publisher_item.contribution_id);
      publisher.SetStringKey("publisherKey", publisher_item.publisher_key);
      publisher.SetDoubleKey("totalAmount", publisher_item.total_amount);
      publisher.SetDoubleKey(
          "contributedAmount",
          publisher_item.contributed_amount);
      publishers.Append(std::move(publisher));
    }
    contribution.SetPath("publishers", std::move(publishers));
    contributions.Append(std::move(contribution));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.contributions",
      std::move(contributions));
}

void RewardsInternalsDOMHandler::GetPromotions(const base::ListValue *args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetAllPromotions(base::BindOnce(
      &RewardsInternalsDOMHandler::OnGetPromotions,
      weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetPromotions(
    const std::vector<brave_rewards::Promotion>& list) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::ListValue promotions;
  for (const auto & item : list) {
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetDouble("amount", item.amount);
    dict->SetString("promotionId", item.promotion_id);
    dict->SetInteger("expiresAt", item.expires_at);
    dict->SetInteger("type", item.type);
    dict->SetInteger("status", item.status);
    dict->SetInteger("claimedAt", item.claimed_at);
    dict->SetBoolean("legacyClaimed", item.legacy_claimed);
    dict->SetString("claimId", item.claim_id);
    dict->SetInteger("version", item.version);
    promotions.Append(std::move(dict));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.promotions",
      std::move(promotions));
}

void RewardsInternalsDOMHandler::GetPartialLog(const base::ListValue *args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->LoadDiagnosticLog(
      g_partial_log_max_lines,
      base::BindOnce(
          &RewardsInternalsDOMHandler::OnGetPartialLog,
          weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetPartialLog(const std::string& log) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.partialLog",
      base::Value(log));
}

void RewardsInternalsDOMHandler::GetFulllLog(const base::ListValue *args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->LoadDiagnosticLog(
      -1,
      base::BindOnce(
          &RewardsInternalsDOMHandler::OnGetFulllLog,
          weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetFulllLog(const std::string& log) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.fullLog",
      base::Value(log));
}

void RewardsInternalsDOMHandler::ClearLog(const base::ListValue *args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->ClearDiagnosticLog(
      base::BindOnce(
          &RewardsInternalsDOMHandler::OnClearLog,
          weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnClearLog(const bool success) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  if (!success) {
    return;
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.partialLog",
      base::Value(""));
}

void RewardsInternalsDOMHandler::GetExternalWallet(
    const base::ListValue* args) {
  CHECK_EQ(1U, args->GetSize());
  if (!rewards_service_) {
    return;
  }

  const std::string wallet_type = args->GetList()[0].GetString();
  rewards_service_->GetExternalWallet(
      wallet_type,
      base::BindOnce(
          &RewardsInternalsDOMHandler::OnGetExternalWallet,
          weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetExternalWallet(
    int32_t result,
    std::unique_ptr<brave_rewards::ExternalWallet> wallet) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }
  base::Value data(base::Value::Type::DICTIONARY);

  data.SetIntKey("result", result);
  base::Value wallet_dict(base::Value::Type::DICTIONARY);

  if (wallet) {
    wallet_dict.SetStringKey("address", wallet->address);
    wallet_dict.SetIntKey("status", static_cast<int>(wallet->status));
  }

  data.SetKey("wallet", std::move(wallet_dict));

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.externalWallet",
      data);
}

void RewardsInternalsDOMHandler::GetEventLogs(const base::ListValue* args) {
  if (!rewards_service_) {
    return;
  }

  rewards_service_->GetEventLogs(
      base::BindOnce(
          &RewardsInternalsDOMHandler::OnGetEventLogs,
          weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetEventLogs(
    const std::vector<brave_rewards::EventLog>& logs) {
  if (!web_ui()->CanCallJavascript()) {
    return;
  }

  base::Value data(base::Value::Type::LIST);

  for (const auto& log : logs) {
    base::Value item(base::Value::Type::DICTIONARY);
    item.SetStringKey("id", log.event_log_id);
    item.SetStringKey("key", log.key);
    item.SetStringKey("value", log.value);
    item.SetIntKey("createdAt", log.created_at);
    data.Append(std::move(item));
  }

  web_ui()->CallJavascriptFunctionUnsafe(
      "brave_rewards_internals.eventLogs",
      std::move(data));
}

}  // namespace

BraveRewardsInternalsUI::BraveRewardsInternalsUI(content::WebUI* web_ui,
                                                 const std::string& name)
    : BasicUI(web_ui,
              name,
              kBraveRewardsInternalsGenerated,
              kBraveRewardsInternalsGeneratedSize,
              IDR_BRAVE_REWARDS_INTERNALS_HTML) {
  auto handler_owner = std::make_unique<RewardsInternalsDOMHandler>();
  RewardsInternalsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsInternalsUI::~BraveRewardsInternalsUI() {
}
