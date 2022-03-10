/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_internals_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {

const int g_partial_log_max_lines = 5000;

class RewardsInternalsDOMHandler : public content::WebUIMessageHandler {
 public:
  RewardsInternalsDOMHandler();
  RewardsInternalsDOMHandler(const RewardsInternalsDOMHandler&) = delete;
  RewardsInternalsDOMHandler& operator=(const RewardsInternalsDOMHandler&) =
      delete;
  ~RewardsInternalsDOMHandler() override;

  void Init();

  // WebUIMessageHandler implementation.
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;
  void RegisterMessages() override;

 private:
  void HandleGetRewardsInternalsInfo(base::Value::ConstListView args);
  void OnGetRewardsInternalsInfo(ledger::type::RewardsInternalsInfoPtr info);
  void GetBalance(base::Value::ConstListView args);
  void OnGetBalance(const ledger::type::Result result,
                    ledger::type::BalancePtr balance);
  void GetContributions(base::Value::ConstListView args);
  void OnGetContributions(ledger::type::ContributionInfoList contributions);
  void GetPromotions(base::Value::ConstListView args);
  void OnGetPromotions(ledger::type::PromotionList list);
  void GetPartialLog(base::Value::ConstListView args);
  void OnGetPartialLog(const std::string& log);
  void GetFulllLog(base::Value::ConstListView args);
  void OnGetFulllLog(const std::string& log);
  void ClearLog(base::Value::ConstListView args);
  void OnClearLog(const bool success);
  void GetExternalWallet(base::Value::ConstListView args);
  void OnGetExternalWallet(const ledger::type::Result result,
                           ledger::type::ExternalWalletPtr wallet);
  void GetEventLogs(base::Value::ConstListView args);
  void OnGetEventLogs(ledger::type::EventLogs logs);
  void GetAdDiagnostics(base::Value::ConstListView args);
  void OnGetAdDiagnostics(const bool success, const std::string& json);

  raw_ptr<brave_rewards::RewardsService> rewards_service_ =
      nullptr;                                            // NOT OWNED
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // NOT OWNED
  raw_ptr<Profile> profile_ = nullptr;
  base::WeakPtrFactory<RewardsInternalsDOMHandler> weak_ptr_factory_;
};

RewardsInternalsDOMHandler::RewardsInternalsDOMHandler()
    : rewards_service_(nullptr), profile_(nullptr), weak_ptr_factory_(this) {}

RewardsInternalsDOMHandler::~RewardsInternalsDOMHandler() {}

void RewardsInternalsDOMHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getRewardsInternalsInfo",
      base::BindRepeating(
          &RewardsInternalsDOMHandler::HandleGetRewardsInternalsInfo,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getBalance",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetBalance,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getContributions",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetContributions,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getPromotions",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetPromotions,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getPartialLog",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetPartialLog,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getFullLog",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetFulllLog,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.clearLog",
      base::BindRepeating(&RewardsInternalsDOMHandler::ClearLog,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getExternalWallet",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetExternalWallet,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getEventLogs",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetEventLogs,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getAdDiagnostics",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetAdDiagnostics,
                          base::Unretained(this)));
}

void RewardsInternalsDOMHandler::Init() {
  profile_ = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  rewards_service_->StartProcess(base::DoNothing());
  ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile_);
}

void RewardsInternalsDOMHandler::OnJavascriptAllowed() {}

void RewardsInternalsDOMHandler::OnJavascriptDisallowed() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void RewardsInternalsDOMHandler::HandleGetRewardsInternalsInfo(
    base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetRewardsInternalsInfo(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetRewardsInternalsInfo,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetRewardsInternalsInfo(
    ledger::type::RewardsInternalsInfoPtr info) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::DictionaryValue info_dict;
  if (info) {
    info_dict.SetString("walletPaymentId", info->payment_id);
    info_dict.SetBoolean("isKeyInfoSeedValid", info->is_key_info_seed_valid);
    info_dict.SetInteger("bootStamp", info->boot_stamp);
  }
  CallJavascriptFunction("brave_rewards_internals.onGetRewardsInternalsInfo",
                         info_dict);
}

void RewardsInternalsDOMHandler::GetBalance(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->FetchBalance(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetBalance,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetBalance(
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value balance_value(base::Value::Type::DICTIONARY);

  if (result == ledger::type::Result::LEDGER_OK && balance) {
    balance_value.SetDoubleKey("total", balance->total);

    base::Value wallets(base::Value::Type::DICTIONARY);
    for (auto const& wallet : balance->wallets) {
      wallets.SetDoubleKey(wallet.first, wallet.second);
    }
    balance_value.SetKey("wallets", std::move(wallets));
  }

  CallJavascriptFunction("brave_rewards_internals.balance",
                         std::move(balance_value));
}

void RewardsInternalsDOMHandler::GetContributions(
    base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetAllContributions(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetContributions,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetContributions(
    ledger::type::ContributionInfoList contributions) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value list(base::Value::Type::LIST);
  for (const auto& item : contributions) {
    base::Value contribution(base::Value::Type::DICTIONARY);
    contribution.SetStringKey("id", item->contribution_id);
    contribution.SetDoubleKey("amount", item->amount);
    contribution.SetIntKey("type", static_cast<int>(item->type));
    contribution.SetIntKey("step", static_cast<int>(item->step));
    contribution.SetIntKey("retryCount", item->retry_count);
    contribution.SetIntKey("createdAt", item->created_at);
    contribution.SetIntKey("processor", static_cast<int>(item->processor));
    base::Value publishers(base::Value::Type::LIST);
    for (const auto& publisher_item : item->publishers) {
      base::Value publisher(base::Value::Type::DICTIONARY);
      publisher.SetStringKey("contributionId", publisher_item->contribution_id);
      publisher.SetStringKey("publisherKey", publisher_item->publisher_key);
      publisher.SetDoubleKey("totalAmount", publisher_item->total_amount);
      publisher.SetDoubleKey("contributedAmount",
                             publisher_item->contributed_amount);
      publishers.Append(std::move(publisher));
    }
    contribution.SetPath("publishers", std::move(publishers));
    list.Append(std::move(contribution));
  }

  CallJavascriptFunction("brave_rewards_internals.contributions",
                         std::move(list));
}

void RewardsInternalsDOMHandler::GetPromotions(
    base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetAllPromotions(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetPromotions,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetPromotions(
    ledger::type::PromotionList list) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::ListValue promotions;
  for (const auto& item : list) {
    auto dict = std::make_unique<base::DictionaryValue>();
    dict->SetDouble("amount", item->approximate_value);
    dict->SetString("promotionId", item->id);
    dict->SetInteger("expiresAt", item->expires_at);
    dict->SetInteger("type", static_cast<int>(item->type));
    dict->SetInteger("status", static_cast<int>(item->status));
    dict->SetInteger("claimedAt", item->claimed_at);
    dict->SetBoolean("legacyClaimed", item->legacy_claimed);
    dict->SetString("claimId", item->claim_id);
    dict->SetInteger("version", item->version);
    promotions.Append(std::move(dict));
  }

  CallJavascriptFunction("brave_rewards_internals.promotions",
                         std::move(promotions));
}

void RewardsInternalsDOMHandler::GetPartialLog(
    base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->LoadDiagnosticLog(
      g_partial_log_max_lines,
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetPartialLog,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetPartialLog(const std::string& log) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards_internals.partialLog",
                         base::Value(log));
}

void RewardsInternalsDOMHandler::GetFulllLog(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->LoadDiagnosticLog(
      -1, base::BindOnce(&RewardsInternalsDOMHandler::OnGetFulllLog,
                         weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetFulllLog(const std::string& log) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards_internals.fullLog", base::Value(log));
}

void RewardsInternalsDOMHandler::ClearLog(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->ClearDiagnosticLog(base::BindOnce(
      &RewardsInternalsDOMHandler::OnClearLog, weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnClearLog(const bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (!success) {
    return;
  }

  CallJavascriptFunction("brave_rewards_internals.partialLog", base::Value(""));
}

void RewardsInternalsDOMHandler::GetExternalWallet(
    base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetExternalWallet(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetExternalWallet,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetExternalWallet(
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::DICTIONARY);
  data.SetIntKey("result", static_cast<int>(result));
  base::Value wallet_dict(base::Value::Type::DICTIONARY);

  if (wallet) {
    wallet_dict.SetStringKey("address", wallet->address);
    wallet_dict.SetStringKey("memberId", wallet->member_id);
    wallet_dict.SetIntKey("status", static_cast<int>(wallet->status));
    wallet_dict.SetStringKey("type", wallet->type);
  }

  data.SetKey("wallet", std::move(wallet_dict));

  CallJavascriptFunction("brave_rewards_internals.externalWallet", data);
}

void RewardsInternalsDOMHandler::GetEventLogs(base::Value::ConstListView args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetEventLogs(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetEventLogs,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetEventLogs(ledger::type::EventLogs logs) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value data(base::Value::Type::LIST);

  for (const auto& log : logs) {
    base::Value item(base::Value::Type::DICTIONARY);
    item.SetStringKey("id", log->event_log_id);
    item.SetStringKey("key", log->key);
    item.SetStringKey("value", log->value);
    item.SetIntKey("createdAt", log->created_at);
    data.Append(std::move(item));
  }

  CallJavascriptFunction("brave_rewards_internals.eventLogs", std::move(data));
}

void RewardsInternalsDOMHandler::GetAdDiagnostics(
    base::Value::ConstListView args) {
  if (!ads_service_) {
    NOTREACHED();
    return;
  }

  AllowJavascript();

  ads_service_->GetAdDiagnostics(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetAdDiagnostics,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetAdDiagnostics(const bool success,
                                                    const std::string& json) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value diagnostics(base::Value::Type::LIST);
  if (success && !json.empty()) {
    absl::optional<base::Value> serialized_json = base::JSONReader::Read(json);
    if (serialized_json && serialized_json->is_list() &&
        !serialized_json->GetList().empty()) {
      diagnostics = std::move(*serialized_json);
    }
  }

#if DCHECK_IS_ON()
  DCHECK(diagnostics.is_list()) << "Diagnostics should be a list";
  for (const auto& entry : diagnostics.GetList()) {
    DCHECK(entry.is_dict()) << "Diagnostics entry should be a dictionary";
    DCHECK(entry.FindKey("key")) << "Diagnostics entry should have 'key' key";
    DCHECK(entry.FindKey("value"))
        << "Diagnostics entry should have 'value' key";
  }
#endif  // DCHECK_IS_ON()

  CallJavascriptFunction("brave_rewards_internals.adDiagnostics", diagnostics);
}

}  // namespace

BraveRewardsInternalsUI::BraveRewardsInternalsUI(content::WebUI* web_ui,
                                                 const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kBraveRewardsInternalsGenerated,
                              kBraveRewardsInternalsGeneratedSize,
                              IDR_BRAVE_REWARDS_INTERNALS_HTML);

  auto handler_owner = std::make_unique<RewardsInternalsDOMHandler>();
  RewardsInternalsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsInternalsUI::~BraveRewardsInternalsUI() = default;
