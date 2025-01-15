/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards_internals_ui.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_internals_generated_map.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"

namespace {

constexpr int kPartialLogMaxLines = 5000;
constexpr size_t kAdDiagnosticIdMaxLength = 36;

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
  void OnGetRewardsInternalsInfo(
      brave_rewards::mojom::RewardsInternalsInfoPtr info);
  void GetBalance(const base::Value::List& args);
  void OnGetBalance(brave_rewards::mojom::BalancePtr balance);
  void GetContributions(const base::Value::List& args);
  void OnGetContributions(
      std::vector<brave_rewards::mojom::ContributionInfoPtr> contributions);
  void GetPartialLog(const base::Value::List& args);
  void OnGetPartialLog(const std::string& log);
  void GetFulllLog(const base::Value::List& args);
  void OnGetFulllLog(const std::string& log);
  void ClearLog(const base::Value::List& args);
  void OnClearLog(const bool success);
  void GetExternalWallet(const base::Value::List& args);
  void OnGetExternalWallet(brave_rewards::mojom::ExternalWalletPtr wallet);
  void GetEventLogs(const base::Value::List& args);
  void OnGetEventLogs(std::vector<brave_rewards::mojom::EventLogPtr> logs);
  void GetAdDiagnostics(const base::Value::List& args);
  void OnGetAdDiagnostics(std::optional<base::Value::List> diagnostics);
  void SetAdDiagnosticId(const base::Value::List& args);
  void GetEnvironment(const base::Value::List& args);
  void OnGetEnvironment(brave_rewards::mojom::Environment environment);

  raw_ptr<brave_rewards::RewardsService> rewards_service_ =
      nullptr;                                            // NOT OWNED
  raw_ptr<brave_ads::AdsService> ads_service_ = nullptr;  // NOT OWNED
  raw_ptr<Profile> profile_ = nullptr;
  base::WeakPtrFactory<RewardsInternalsDOMHandler> weak_ptr_factory_;
};

RewardsInternalsDOMHandler::RewardsInternalsDOMHandler()
    : rewards_service_(nullptr), profile_(nullptr), weak_ptr_factory_(this) {}

RewardsInternalsDOMHandler::~RewardsInternalsDOMHandler() = default;

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
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.setAdDiagnosticId",
      base::BindRepeating(&RewardsInternalsDOMHandler::SetAdDiagnosticId,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brave_rewards_internals.getEnvironment",
      base::BindRepeating(&RewardsInternalsDOMHandler::GetEnvironment,
                          base::Unretained(this)));
}

void RewardsInternalsDOMHandler::Init() {
  profile_ = Profile::FromWebUI(web_ui());
  rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
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
    brave_rewards::mojom::RewardsInternalsInfoPtr info) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict info_dict;
  if (info) {
    const auto* prefs = profile_->GetPrefs();
    const std::string declared_geo =
        prefs->GetString(::brave_rewards::prefs::kDeclaredGeo);
    const int wallet_creation_environment =
        prefs->GetInteger(::brave_rewards::prefs::kWalletCreationEnvironment);
    info_dict.Set("walletPaymentId", info->payment_id);
    info_dict.Set("isKeyInfoSeedValid", info->is_key_info_seed_valid);
    info_dict.Set("bootStamp", static_cast<double>(info->boot_stamp));
    info_dict.Set("declaredGeo", declared_geo);
    if (wallet_creation_environment != -1) {
      info_dict.Set("walletCreationEnvironment", wallet_creation_environment);
    }
  }
  CallJavascriptFunction("brave_rewards_internals.onGetRewardsInternalsInfo",
                         info_dict);
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
    brave_rewards::mojom::BalancePtr balance) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict data;
  if (balance) {
    data.Set("total", balance->total);
    data.Set("wallets",
             base::Value::Dict(std::move_iterator(balance->wallets.begin()),
                               std::move_iterator(balance->wallets.end())));
  } else {
    data.Set("total", 0.0);
    data.Set("wallets", base::Value::Dict());
  }

  CallJavascriptFunction("brave_rewards_internals.balance", std::move(data));
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
    std::vector<brave_rewards::mojom::ContributionInfoPtr> contributions) {
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

  CallJavascriptFunction("brave_rewards_internals.contributions", list);
}

void RewardsInternalsDOMHandler::GetPartialLog(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->LoadDiagnosticLog(
      kPartialLogMaxLines,
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
    brave_rewards::mojom::ExternalWalletPtr wallet) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  if (!wallet) {
    CallJavascriptFunction("brave_rewards_internals.onGetExternalWallet");
    return;
  }

  base::Value::Dict data;
  data.Set("address", wallet->address);
  data.Set("memberId", wallet->member_id);
  data.Set("status", static_cast<int>(wallet->status));
  data.Set("type", wallet->type);

  CallJavascriptFunction("brave_rewards_internals.onGetExternalWallet", data);
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

void RewardsInternalsDOMHandler::OnGetEventLogs(
    std::vector<brave_rewards::mojom::EventLogPtr> logs) {
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

  CallJavascriptFunction("brave_rewards_internals.eventLogs", data);
}

void RewardsInternalsDOMHandler::GetAdDiagnostics(
    const base::Value::List& args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  ads_service_->GetDiagnostics(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetAdDiagnostics,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetAdDiagnostics(
    std::optional<base::Value::List> diagnosticsEntries) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  base::Value::Dict diagnostics;
  const PrefService* prefs = profile_->GetPrefs();
  const std::string& diagnostic_id =
      prefs->GetString(brave_ads::prefs::kDiagnosticId);
  diagnostics.Set("diagnosticId", diagnostic_id);

  if (diagnosticsEntries) {
#if DCHECK_IS_ON()
    for (const auto& entry : *diagnosticsEntries) {
      DCHECK(entry.is_dict()) << "Diagnostic entry must be a dictionary";
      DCHECK(entry.GetDict().Find("name"))
          << "Diagnostic entry missing 'name' key";
      DCHECK(entry.GetDict().Find("value"))
          << "Diagnostic entry missing 'value' key";
    }
#endif  // DCHECK_IS_ON()

    diagnostics.Set("entries", std::move(*diagnosticsEntries));
  }

  CallJavascriptFunction("brave_rewards_internals.adDiagnostics", diagnostics);
}

void RewardsInternalsDOMHandler::SetAdDiagnosticId(
    const base::Value::List& args) {
  if (args.empty() || !args[0].is_string() ||
      args[0].GetString().size() > kAdDiagnosticIdMaxLength) {
    return;
  }

  PrefService* prefs = profile_->GetPrefs();
  prefs->SetString(brave_ads::prefs::kDiagnosticId, args[0].GetString());
}

void RewardsInternalsDOMHandler::GetEnvironment(const base::Value::List& args) {
  if (!rewards_service_) {
    return;
  }

  AllowJavascript();

  rewards_service_->GetEnvironment(
      base::BindOnce(&RewardsInternalsDOMHandler::OnGetEnvironment,
                     weak_ptr_factory_.GetWeakPtr()));
}

void RewardsInternalsDOMHandler::OnGetEnvironment(
    brave_rewards::mojom::Environment environment) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  CallJavascriptFunction("brave_rewards_internals.environment",
                         base::Value(static_cast<int>(environment)));
}

}  // namespace

BraveRewardsInternalsUI::BraveRewardsInternalsUI(content::WebUI* web_ui,
                                                 const std::string& name)
    : WebUIController(web_ui) {
  CreateAndAddWebUIDataSource(web_ui, name, kBraveRewardsInternalsGenerated,
                              IDR_BRAVE_REWARDS_INTERNALS_HTML);

  auto handler_owner = std::make_unique<RewardsInternalsDOMHandler>();
  RewardsInternalsDOMHandler* handler = handler_owner.get();
  web_ui->AddMessageHandler(std::move(handler_owner));
  handler->Init();
}

BraveRewardsInternalsUI::~BraveRewardsInternalsUI() = default;
