/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_CLIENT_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_CLIENT_H_

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <map>

#include "bat/ledger/export.h"
#include "bat/ledger/mojom_structs.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {
namespace client {

using FetchIconCallback = std::function<void(bool, const std::string&)>;

using LoadURLCallback = std::function<void(const type::UrlResponse&)>;

using OnLoadCallback =
    std::function<void(const type::Result, const std::string&)>;

using RunDBTransactionCallback =
    std::function<void(type::DBCommandResponsePtr)>;

using GetCreateScriptCallback =
    std::function<void(const std::string&, const int)>;

using ResultCallback =
    std::function<void(const type::Result)>;

using GetPromotionListCallback = std::function<void(type::PromotionList)>;

using TransactionCallback =
    std::function<void(const type::Result, const std::string&)>;

using GetServerPublisherInfoCallback =
    std::function<void(type::ServerPublisherInfoPtr)>;

}  // namespace client

class LEDGER_EXPORT LedgerClient {
 public:
  virtual ~LedgerClient() = default;

  virtual void OnReconcileComplete(
      const type::Result result,
      type::ContributionInfoPtr contribution) = 0;

  virtual void LoadLedgerState(client::OnLoadCallback callback) = 0;

  virtual void LoadPublisherState(client::OnLoadCallback callback) = 0;

  virtual void OnPanelPublisherInfo(
      type::Result result,
      type::PublisherInfoPtr publisher_info,
      uint64_t windowId) = 0;

  virtual void FetchFavIcon(
      const std::string& url,
      const std::string& favicon_key,
      client::FetchIconCallback callback) = 0;

  virtual std::string URIEncode(const std::string& value) = 0;

  virtual void LoadURL(
      type::UrlRequestPtr request,
      client::LoadURLCallback callback) = 0;

  virtual void Log(
      const char* file,
      const int line,
      const int verbose_level,
      const std::string& message) = 0;

  virtual void PublisherListNormalized(type::PublisherInfoList list) = 0;

  virtual void SetBooleanState(const std::string& name, bool value) = 0;

  virtual bool GetBooleanState(const std::string& name) const = 0;

  virtual void SetIntegerState(const std::string& name, int value) = 0;

  virtual int GetIntegerState(const std::string& name) const = 0;

  virtual void SetDoubleState(const std::string& name, double value) = 0;

  virtual double GetDoubleState(const std::string& name) const = 0;

  virtual void SetStringState(
      const std::string& name,
      const std::string& value) = 0;

  virtual std::string GetStringState(const std::string& name) const = 0;

  virtual void SetInt64State(const std::string& name, int64_t value) = 0;

  virtual int64_t GetInt64State(const std::string& name) const = 0;

  virtual void SetUint64State(const std::string& name, uint64_t value) = 0;

  virtual uint64_t GetUint64State(const std::string& name) const = 0;

  virtual void ClearState(const std::string& name) = 0;

  virtual bool GetBooleanOption(const std::string& name) const = 0;

  virtual int GetIntegerOption(const std::string& name) const = 0;

  virtual double GetDoubleOption(const std::string& name) const = 0;

  virtual std::string GetStringOption(const std::string& name) const = 0;

  virtual int64_t GetInt64Option(const std::string& name) const = 0;

  virtual uint64_t GetUint64Option(const std::string& name) const = 0;

  virtual void OnContributeUnverifiedPublishers(
      type::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) = 0;

  // DEPRECATED
  virtual std::string GetLegacyWallet() = 0;

  virtual void ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      client::ResultCallback callback) = 0;

  virtual type::ClientInfoPtr GetClientInfo() = 0;

  virtual void UnblindedTokensReady() = 0;

  virtual void ReconcileStampReset() = 0;

  virtual void RunDBTransaction(
      type::DBTransactionPtr transaction,
      client::RunDBTransactionCallback callback) = 0;

  virtual void GetCreateScript(client::GetCreateScriptCallback callback) = 0;

  virtual void PendingContributionSaved(const type::Result result) = 0;

  virtual void ClearAllNotifications() = 0;

  virtual void WalletDisconnected(const std::string& wallet_type) = 0;

  virtual void DeleteLog(client::ResultCallback callback) = 0;

  virtual absl::optional<std::string> EncryptString(
      const std::string& value) = 0;

  virtual absl::optional<std::string> DecryptString(
      const std::string& value) = 0;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_INCLUDE_BAT_LEDGER_LEDGER_CLIENT_H_
