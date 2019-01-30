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

class RefillTokens;
class RedeemToken;
class PayoutTokens;

class ConfirmationsImpl : public Confirmations {
 public:
  explicit ConfirmationsImpl(ConfirmationsClient* confirmations_client);
  ~ConfirmationsImpl() override;

  void UpdateConfirmationsIsReadyStatus();

  // Wallet
  void SetWalletInfo(std::unique_ptr<WalletInfo> info) override;

  // Catalog issuers
  void SetCatalogIssuers(std::unique_ptr<IssuersInfo> info) override;
  std::map<std::string, std::string> GetCatalogIssuers() const;

  // Scheduled events
  void OnTimer(const uint32_t timer_id) override;

  // Refill tokens
  void StartRefillingConfirmations(const uint64_t start_timer_in);
  void StartRetryGettingSignedTokens(const uint64_t start_timer_in);
  std::vector<UnblindedToken> GetUnblindedTokens() const;
  void SetUnblindedTokens(const std::vector<UnblindedToken>& tokens);

  // Redeem token
  void AdSustained(std::unique_ptr<NotificationInfo> info) override;
  std::vector<UnblindedToken> GetUnblindedPaymentTokens() const;
  void SetUnblindedPaymentTokens(const std::vector<UnblindedToken>& tokens);

  // Payout tokens
  void StartPayingOutConfirmations(const uint64_t start_timer_in);

  // State
  void SaveState();

 private:
  bool is_initialized_;

  // Wallet
  bool is_wallet_initialized_;
  WalletInfo wallet_info_;
  std::string public_key_;

  // Catalog issuers
  bool is_catalog_issuers_initialized_;
  std::map<std::string, std::string> catalog_issuers_;

  // Refill tokens
  uint32_t refill_confirmations_timer_id_;
  void RefillConfirmations();
  void StopRefillingConfirmations();
  bool IsRefillingConfirmations() const;
  std::unique_ptr<RefillTokens> refill_tokens_;

  uint32_t retry_getting_signed_tokens_timer_id_;
  void RetryGettingSignedTokens();
  void StopRetryGettingSignedTokens();
  bool IsRetryingToGetSignedTokens() const;
  std::vector<UnblindedToken> unblinded_tokens_;

  // Redeem token
  std::unique_ptr<RedeemToken> redeem_token_;
  std::vector<UnblindedToken> unblinded_payment_tokens_;

  // Payout tokens
  uint32_t payout_confirmations_timer_id_;
  void PayoutConfirmations();
  void StopPayingOutConfirmations();
  bool IsPayingOutConfirmations() const;
  std::unique_ptr<PayoutTokens> payout_tokens_;

  // State
  void OnStateSaved(const Result result);
  void LoadState();
  void OnStateLoaded(const Result result, const std::string& json);
  void ResetState();
  void OnStateReset(const Result result);

  std::string ToJSON();
  bool FromJSON(const std::string& json);

  std::unique_ptr<base::ListValue> Munge(const std::vector<std::string>& v);
  std::vector<std::string> Unmunge(base::Value *value);

  // Confirmations::Client
  ConfirmationsClient* confirmations_client_;  // NOT OWNED

  // Not copyable, not assignable
  ConfirmationsImpl(const ConfirmationsImpl&) = delete;
  ConfirmationsImpl& operator=(const ConfirmationsImpl&) = delete;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_CONFIRMATIONS_IMPL_H_
