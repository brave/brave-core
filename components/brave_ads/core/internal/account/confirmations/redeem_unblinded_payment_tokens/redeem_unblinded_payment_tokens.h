/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/common/timer/timer.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"

namespace brave_ads {

class RedeemUnblindedPaymentTokens final {
 public:
  RedeemUnblindedPaymentTokens();

  RedeemUnblindedPaymentTokens(const RedeemUnblindedPaymentTokens&) = delete;
  RedeemUnblindedPaymentTokens& operator=(const RedeemUnblindedPaymentTokens&) =
      delete;

  RedeemUnblindedPaymentTokens(RedeemUnblindedPaymentTokens&&) noexcept =
      delete;
  RedeemUnblindedPaymentTokens& operator=(
      RedeemUnblindedPaymentTokens&&) noexcept = delete;

  ~RedeemUnblindedPaymentTokens();

  void SetDelegate(RedeemUnblindedPaymentTokensDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeRedeemAfterDelay(const WalletInfo& wallet);

 private:
  void Redeem();
  void OnRedeemUnblindedPaymentTokensUserDataBuilt(base::Value::Dict user_data);
  void OnRedeem(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
      const mojom::UrlResponseInfo& url_response);

  void SuccessfullyRedeemedUnblindedPaymentTokens(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens);
  void FailedToRedeemUnblindedPaymentTokens(bool should_retry);

  void ScheduleNextTokenRedemption();

  void Retry();
  void OnRetry();

  raw_ptr<RedeemUnblindedPaymentTokensDelegate> delegate_ = nullptr;

  WalletInfo wallet_;

  bool is_processing_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;

  base::WeakPtrFactory<RedeemUnblindedPaymentTokens> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
