/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/redeem_confirmation_delegate.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"

namespace brave_ads {

namespace privacy {
class TokenGeneratorInterface;
struct UnblindedPaymentTokenInfo;
}  // namespace privacy

struct TransactionInfo;

class Confirmations final : public RedeemConfirmationDelegate {
 public:
  explicit Confirmations(privacy::TokenGeneratorInterface* token_generator);

  Confirmations(const Confirmations&) = delete;
  Confirmations& operator=(const Confirmations&) = delete;

  Confirmations(Confirmations&&) noexcept = delete;
  Confirmations& operator=(Confirmations&&) noexcept = delete;

  ~Confirmations() override;

  void SetDelegate(ConfirmationsDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void Confirm(const TransactionInfo& transaction);

  void ProcessRetryQueue();

 private:
  void Retry();
  void RetryCallback();
  void StopRetrying();

  void ConfirmTransaction(const TransactionInfo& transaction);
  void BuildDynamicUserData(const TransactionInfo& transaction);
  void BuildFixedUserData(const TransactionInfo& transaction,
                          base::Value::Dict dynamic_opted_in_user_data);
  void CreateAndRedeem(const TransactionInfo& transaction,
                       base::Value::Dict dynamic_opted_in_user_data,
                       base::Value::Dict fixed_opted_in_user_data);

  void RecreateOptedInDynamicUserDataAndRedeem(
      const ConfirmationInfo& confirmation);
  void RecreateOptedInDynamicUserDataAndRedeemCallback(
      const ConfirmationInfo& confirmation,
      base::Value::Dict dynamic_opted_in_user_data);

  void Redeem(const ConfirmationInfo& confirmation);

  // RedeemConfirmationDelegate:
  void OnDidRedeemOptedInConfirmation(const ConfirmationInfo& confirmation,
                                      const privacy::UnblindedPaymentTokenInfo&
                                          unblinded_payment_token) override;
  void OnDidRedeemOptedOutConfirmation(
      const ConfirmationInfo& confirmation) override;
  void OnFailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                    bool should_retry,
                                    bool should_backoff) override;

  raw_ptr<ConfirmationsDelegate> delegate_ = nullptr;

  const raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  BackoffTimer retry_timer_;

  base::WeakPtrFactory<Confirmations> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
