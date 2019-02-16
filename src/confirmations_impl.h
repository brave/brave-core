/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_
#define BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "bat/confirmations/confirmations.h"
#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/notification_info.h"
#include "bat/confirmations/issuers_info.h"
#include "refill_tokens.h"
#include "redeem_token.h"
#include "payout_tokens.h"

#include "base/values.h"

namespace confirmations {

class UnblindedTokens;
class RefillTokens;
class RedeemToken;
class PayoutTokens;

class ConfirmationsImpl : public Confirmations {
 public:
  explicit ConfirmationsImpl(ConfirmationsClient* confirmations_client);
  ~ConfirmationsImpl() override;

  // Wallet
  void SetWalletInfo(std::unique_ptr<WalletInfo> info) override;

  // Catalog issuers
  void SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) override;
  std::map<std::string, std::string> GetCatalogIssuers() const;
  bool IsValidPublicKeyForCatalogIssuers(const std::string& public_key) const;

  // Scheduled events
  bool OnTimer(const uint32_t timer_id) override;

  // Refill tokens
  void RefillTokensIfNecessary() const;
  void StartRetryingToGetRefillSignedTokens(const uint64_t start_timer_in);

  // Redeem unblinded tokens
  void AdSustained(std::unique_ptr<NotificationInfo> info) override;

  // Payout redeemed tokens
  void StartPayingOutRedeemedTokens(const uint64_t start_timer_in);

  // State
  void SaveState();

 private:
  bool is_initialized_;
  void CheckReady();

  // Wallet
  WalletInfo wallet_info_;
  std::string public_key_;

  // Catalog issuers
  std::map<std::string, std::string> catalog_issuers_;

  // Unblinded tokens
  std::unique_ptr<UnblindedTokens> unblinded_tokens_;
  void NotifyAdsIfConfirmationsIsReady();

  std::unique_ptr<UnblindedTokens> unblinded_payment_tokens_;

  // Refill tokens
  uint32_t retry_getting_signed_tokens_timer_id_;
  void RetryGettingRefillSignedTokens() const;
  void StopRetryingToGetRefillSignedTokens();
  bool IsRetryingToGetRefillSignedTokens() const;
  std::unique_ptr<RefillTokens> refill_tokens_;

  // Redeem unblinded tokens
  std::unique_ptr<RedeemToken> redeem_token_;

  // Payout redeemed tokens
  uint32_t payout_redeemed_tokens_timer_id_;
  void PayoutRedeemedTokens() const;
  void StopPayingOutRedeemedTokens();
  bool IsPayingOutRedeemedTokens() const;
  std::unique_ptr<PayoutTokens> payout_tokens_;

  // State
  void OnStateSaved(const Result result);
  void LoadState();
  void OnStateLoaded(const Result result, const std::string& json);
  void ResetState();
  void OnStateReset(const Result result);

  std::string ToJSON() const;

  base::Value GetCatalogIssuersAsDictionary(
      const std::string& public_key,
      const std::map<std::string, std::string>& issuers) const;

  bool FromJSON(const std::string& json);

  bool GetCatalogIssuersFromDictionary(
      base::DictionaryValue* dictionary,
      std::string* public_key,
      std::map<std::string, std::string>* issuers) const;

  // Confirmations::Client
  ConfirmationsClient* confirmations_client_;  // NOT OWNED

  // Not copyable, not assignable
  ConfirmationsImpl(const ConfirmationsImpl&) = delete;
  ConfirmationsImpl& operator=(const ConfirmationsImpl&) = delete;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_
