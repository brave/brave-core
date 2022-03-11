/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/account/confirmations/confirmations_delegate.h"
#include "bat/ads/internal/account/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "bat/ads/internal/backoff_timer.h"

namespace base {
class Value;
}  // namespace base

namespace ads {

class AdType;
class ConfirmationType;
class RedeemUnblindedToken;
struct TransactionInfo;

namespace privacy {
class TokenGeneratorInterface;
struct UnblindedPaymentTokenInfo;
}  // namespace privacy

class Confirmations final : public RedeemUnblindedTokenDelegate {
 public:
  explicit Confirmations(privacy::TokenGeneratorInterface* token_generator);
  ~Confirmations() override;

  void set_delegate(ConfirmationsDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void Confirm(const TransactionInfo& transaction);

  void ProcessRetryQueue();

 private:
  raw_ptr<ConfirmationsDelegate> delegate_ = nullptr;

  raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  std::unique_ptr<RedeemUnblindedToken> redeem_unblinded_token_;

  ConfirmationInfo CreateConfirmation(const std::string& transaction_id,
                                      const std::string& creative_instance_id,
                                      const ConfirmationType& confirmation_type,
                                      const AdType& ad_type,
                                      const base::Value& user_data) const;

  BackoffTimer retry_timer_;
  void Retry();
  void OnRetry();
  void StopRetrying();

  void CreateNewConfirmationAndAppendToRetryQueue(
      const ConfirmationInfo& confirmation);
  void AppendToRetryQueue(const ConfirmationInfo& confirmation);
  void RemoveFromRetryQueue(const ConfirmationInfo& confirmation);

  // RedeemUnblindedTokenDelegate:
  void OnDidSendConfirmation(const ConfirmationInfo& confirmation) override;
  void OnFailedToSendConfirmation(const ConfirmationInfo& confirmation,
                                  const bool should_retry) override;
  void OnDidRedeemUnblindedToken(const ConfirmationInfo& confirmation,
                                 const privacy::UnblindedPaymentTokenInfo&
                                     unblinded_payment_token) override;
  void OnFailedToRedeemUnblindedToken(const ConfirmationInfo& confirmation,
                                      const bool should_retry) override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
