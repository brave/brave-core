/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_CLIENT_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_rewards::core {

using FetchIconCallback = std::function<void(bool, const std::string&)>;

using LegacyLoadURLCallback = std::function<void(const mojom::UrlResponse&)>;

using LoadURLCallback = base::OnceCallback<void(const mojom::UrlResponse&)>;

using OnLoadCallback =
    std::function<void(const mojom::Result, const std::string&)>;

using LegacyRunDBTransactionCallback =
    std::function<void(mojom::DBCommandResponsePtr)>;

using RunDBTransactionCallback =
    base::OnceCallback<void(mojom::DBCommandResponsePtr)>;

using GetCreateScriptCallback =
    std::function<void(const std::string&, const int)>;

using LegacyResultCallback = std::function<void(mojom::Result)>;

using ResultCallback = base::OnceCallback<void(mojom::Result)>;

using GetPromotionListCallback =
    std::function<void(std::vector<mojom::PromotionPtr>)>;

using TransactionCallback =
    std::function<void(const mojom::Result, const std::string&)>;

using GetServerPublisherInfoCallback =
    std::function<void(mojom::ServerPublisherInfoPtr)>;

class LedgerClient {
 public:
  virtual ~LedgerClient() = default;

  virtual void OnReconcileComplete(const mojom::Result result,
                                   mojom::ContributionInfoPtr contribution) = 0;

  virtual void LoadLedgerState(OnLoadCallback callback) = 0;

  virtual void LoadPublisherState(OnLoadCallback callback) = 0;

  virtual void OnPanelPublisherInfo(mojom::Result result,
                                    mojom::PublisherInfoPtr publisher_info,
                                    uint64_t windowId) = 0;

  virtual void OnPublisherRegistryUpdated() = 0;

  virtual void OnPublisherUpdated(const std::string& publisher_id) = 0;

  virtual void FetchFavIcon(const std::string& url,
                            const std::string& favicon_key,
                            FetchIconCallback callback) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void LoadURL(mojom::UrlRequestPtr request,
                       LoadURLCallback callback) = 0;

  virtual void Log(const char* file,
                   const int line,
                   const int verbose_level,
                   const std::string& message) = 0;

  virtual void PublisherListNormalized(
      std::vector<mojom::PublisherInfoPtr> list) = 0;

  virtual void SetBooleanState(const std::string& name, bool value) = 0;

  virtual bool GetBooleanState(const std::string& name) const = 0;

  virtual void SetIntegerState(const std::string& name, int value) = 0;

  virtual int GetIntegerState(const std::string& name) const = 0;

  virtual void SetDoubleState(const std::string& name, double value) = 0;

  virtual double GetDoubleState(const std::string& name) const = 0;

  virtual void SetStringState(const std::string& name,
                              const std::string& value) = 0;

  virtual std::string GetStringState(const std::string& name) const = 0;

  virtual void SetInt64State(const std::string& name, int64_t value) = 0;

  virtual int64_t GetInt64State(const std::string& name) const = 0;

  virtual void SetUint64State(const std::string& name, uint64_t value) = 0;

  virtual uint64_t GetUint64State(const std::string& name) const = 0;

  virtual void SetValueState(const std::string& name, base::Value value) = 0;

  virtual base::Value GetValueState(const std::string& name) const = 0;

  virtual void SetTimeState(const std::string& name, base::Time time) = 0;

  virtual base::Time GetTimeState(const std::string& name) const = 0;

  virtual void ClearState(const std::string& name) = 0;

  virtual bool GetBooleanOption(const std::string& name) const = 0;

  virtual int GetIntegerOption(const std::string& name) const = 0;

  virtual double GetDoubleOption(const std::string& name) const = 0;

  virtual std::string GetStringOption(const std::string& name) const = 0;

  virtual int64_t GetInt64Option(const std::string& name) const = 0;

  virtual uint64_t GetUint64Option(const std::string& name) const = 0;

  virtual void OnContributeUnverifiedPublishers(
      mojom::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) = 0;

  // DEPRECATED
  virtual std::string GetLegacyWallet() = 0;

  virtual void ShowNotification(const std::string& type,
                                const std::vector<std::string>& args,
                                LegacyResultCallback callback) = 0;

  virtual mojom::ClientInfoPtr GetClientInfo() = 0;

  virtual void UnblindedTokensReady() = 0;

  virtual void ReconcileStampReset() = 0;

  virtual void RunDBTransaction(mojom::DBTransactionPtr transaction,
                                RunDBTransactionCallback callback) = 0;

  virtual void GetCreateScript(GetCreateScriptCallback callback) = 0;

  virtual void PendingContributionSaved(const mojom::Result result) = 0;

  virtual void ClearAllNotifications() = 0;

  virtual void ExternalWalletConnected() const = 0;

  virtual void ExternalWalletLoggedOut() const = 0;

  virtual void ExternalWalletReconnected() const = 0;

  virtual void DeleteLog(LegacyResultCallback callback) = 0;

  virtual absl::optional<std::string> EncryptString(
      const std::string& value) = 0;

  virtual absl::optional<std::string> DecryptString(
      const std::string& value) = 0;
};

}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEDGER_CLIENT_H_
