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
  void HandleGetRewardsInternalsInfo(const base::Value::List& args);
  void OnGetRewardsInternalsInfo(ledger::type::RewardsInternalsInfoPtr info);
  void GetBalance(const base::Value::List& args);
  void OnGetBalance(const ledger::type::Result result,
                    ledger::type::BalancePtr balance);
  void GetContributions(const base::Value::List& args);
  void OnGetContributions(ledger::type::ContributionInfoList contributions);
  void GetPromotions(const base::Value::List& args);
  void OnGetPromotions(ledger::type::PromotionList list);
  void GetPartialLog(const base::Value::List& args);
  void OnGetPartialLog(const std::string& log);
  void GetFulllLog(const base::Value::List& args);
  void OnGetFulllLog(const std::string& log);
  void ClearLog(const base::Value::List& args);
  void OnClearLog(const bool success);
  void GetExternalWallet(const base::Value::List& args);
  void OnGetExternalWallet(const ledger::type::Result result,
                           ledger::type::ExternalWalletPtr wallet);
  void GetEventLogs(const base::Value::List& args);
  void OnGetEventLogs(ledger::type::EventLogs logs);
  void GetAdDiagnostics(const base::Value::List& args);
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
    const base::Value::List& args) {
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

  base::Value::Dict info_dict;
  if (info) {
    info_dict.Set("walletPaymentId", info->payment_id);
    info_dict.Set("isKeyInfoSeedValid", info->is_key_info_seed_valid);
    info_dict.Set("bootStamp", static_cast<double>(info->boot_stamp));
  }
  CallJavascriptFunction("brave_rewards_internals.onGetRewardsInternalsInfo",
                         base::Value(std::move(info_dict)));
}

void RewardsInternalsDOMHandler::GetBalance(const base::Value::List& args) {
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

  base::Value::Dict balance_value;

  if (result == ledger::type::Result::LEDGER_OK && balance) {
    balance_value.Set("total", balance->total);

    base::Value::Dict wallets;
    for (auto const& wallet : balance->wallets) {
      wallets.Set(wallet.first, wallet.second);
    }
    balance_value.Set("wallets", std::move(wallets));
  }

  CallJavascriptFunction("brave_rewards_internals.balance",
                         base::Value(std::move(balance_value)));
}

void RewardsInternalsDOMHandler::GetContributions(
    const base::Value::List& args) {
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

  base::Value::List list;
  for (const auto& item : contributions) {
    base::Value::Dict contribution;
    contribution.Set("id", item->contribution_id);
    contribution.Set("amount", item->amount);
    contribution.Set("type", static_cast<int>(item->type));
    contribution.Set("step", static_cast<int>(item->step));
    contribution.Set("retryCount", item->retry_count);
    contribution.Set("createdAt", static_cast<double>(item->created_at));
    contribution.Set("processor", static_cast<int>(item->processor));
    base::Value::List publishers;
    for (const auto& publisher_item : item->publishers) {
      base::Value::Dict publisher;
      publisher.Set("contributionId", publisher_item->contribution_id);
      publisher.Set("publisherKey", publisher_item->publisher_key);
      publisher.Set("totalAmount", publisher_item->total_amount);
      publisher.Set("contributedAmount", publisher_item->contributed_amount);
      publishers.Append(std::move(publisher));
    }
    contribution.Set("publishers", std::move(publishers));
    list.Append(std::move(contribution));
  }

  CallJavascriptFunction("brave_rewards_internals.contributions",
                         base::Value(std::move(list)));
}

void RewardsInternalsDOMHandler::GetPromotions(const base::Value::List& args) {
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

  base::Value::List promotions;
  for (const auto& item : list) {
    base::Value::Dict dict;
    dict.Set("amount", item->approximate_value);
    dict.Set("promotionId", item->id);
    dict.Set("expiresAt", static_cast<double>(item->expires_at));
    dict.Set("type", static_cast<int>(item->type));
    dict.Set("status", static_cast<int>(item->status));
    dict.Set("claimedAt", static_cast<double>(item->claimed_at));
    dict.Set("legacyClaimed", item->legacy_claimed);
    dict.Set("claimId", item->claim_id);
    dict.Set("version", static_cast<int>(item->version));
    promotions.Append(std::move(dict));
  }

  CallJavascriptFunction("brave_rewards_internals.promotions",
                         base::Value(std::move(promotions)));
}

void RewardsInternalsDOMHandler::GetPartialLog(const base::Value::List& args) {
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

void RewardsInternalsDOMHandler::GetFulllLog(const base::Value::List& args) {
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

void RewardsInternalsDOMHandler::ClearLog(const base::Value::List& args) {
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
    const base::Value::List& args) {
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

  base::Value::Dict data;
  data.Set("result", static_cast<int>(result));
  base::Value::Dict wallet_dict;

  if (wallet) {
    wallet_dict.Set("address", wallet->address);
    wallet_dict.Set("memberId", wallet->member_id);
    wallet_dict.Set("status", static_cast<int>(wallet->status));
    wallet_dict.Set("type", wallet->type);
  }

  data.Set("wallet", std::move(wallet_dict));

  CallJavascriptFunction("brave_rewards_internals.externalWallet",
                         base::Value(std::move(data)));
}

void RewardsInternalsDOMHandler::GetEventLogs(const base::Value::List& args) {
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

  base::Value::List data;

  for (const auto& log : logs) {
    base::Value::Dict item;
    item.Set("id", log->event_log_id);
    item.Set("key", log->key);
    item.Set("value", log->value);
    item.Set("createdAt", static_cast<double>(log->created_at));
    data.Append(std::move(item));
  }

  CallJavascriptFunction("brave_rewards_internals.eventLogs",
                         base::Value(std::move(data)));
}

void RewardsInternalsDOMHandler::GetAdDiagnostics(
    const base::Value::List& args) {
  if (!ads_service_) {
    NOTREACHED();
    return;
  }

  AllowJavascript();

  ads_service_->GetDiagnostics(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetAdDiagnostics,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetAdDiagnostics(const bool success,
                                                    const std::string& json) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::List diagnostics;
  if (success && !json.empty()) {
    absl::optional<base::Value> serialized_json = base::JSONReader::Read(json);
    if (serialized_json && serialized_json->is_list() &&
        !serialized_json->GetList().empty()) {
      diagnostics = std::move(*serialized_json->GetIfList());
    }
  }

#if DCHECK_IS_ON()
  for (const auto& entry : diagnostics) {
    DCHECK(entry.is_dict()) << "Diagnostic entry must be a dictionary";
    DCHECK(entry.GetDict().Find("name"))
        << "Diagnostic entry missing 'name' key";
    DCHECK(entry.GetDict().Find("value"))
        << "Diagnostic entry missing 'value' key";
  }
#endif  // DCHECK_IS_ON()

  CallJavascriptFunction("brave_rewards_internals.adDiagnostics",
                         base::Value(std::move(diagnostics)));
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
