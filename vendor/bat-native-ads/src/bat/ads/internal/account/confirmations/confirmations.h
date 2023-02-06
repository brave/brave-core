/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "bat/ads/internal/account/confirmations/confirmations_delegate.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "bat/ads/internal/common/timer/backoff_timer.h"

namespace ads {

namespace privacy {
class TokenGeneratorInterface;
struct UnblindedPaymentTokenInfo;
}  // namespace privacy

class RedeemUnblindedToken;
struct TransactionInfo;

class Confirmations final : public RedeemUnblindedTokenDelegate {
 public:
  explicit Confirmations(privacy::TokenGeneratorInterface* token_generator);

  Confirmations(const Confirmations& other) = delete;
  Confirmations& operator=(const Confirmations& other) = delete;

  Confirmations(Confirmations&& other) noexcept = delete;
  Confirmations& operator=(Confirmations&& other) noexcept = delete;

  ~Confirmations() override;

  void SetDelegate(ConfirmationsDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void Confirm(const TransactionInfo& transaction);

  void ProcessRetryQueue();

 private:
  void Retry();
  void OnRetry();
  void StopRetrying();

  void CreateConfirmationAndRedeemToken(const TransactionInfo& transaction,
                                        const base::Time& created_at,
                                        base::Value::Dict user_data);

  void CreateNewConfirmationAndAppendToRetryQueue(
      const ConfirmationInfo& confirmation,
      base::Value::Dict user_data);

  // RedeemUnblindedTokenDelegate:
  void OnDidSendConfirmation(const ConfirmationInfo& confirmation) override;
  void OnFailedToSendConfirmation(const ConfirmationInfo& confirmation,
                                  bool should_retry) override;
  void OnDidRedeemUnblindedToken(const ConfirmationInfo& confirmation,
                                 const privacy::UnblindedPaymentTokenInfo&
                                     unblinded_payment_token) override;
  void OnFailedToRedeemUnblindedToken(const ConfirmationInfo& confirmation,
                                      bool should_retry,
                                      bool should_backoff) override;

  raw_ptr<ConfirmationsDelegate> delegate_ = nullptr;

  const raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;

  BackoffTimer retry_timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
