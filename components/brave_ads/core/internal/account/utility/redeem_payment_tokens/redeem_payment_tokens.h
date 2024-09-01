/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_H_

#include <string>
#include <tuple>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

class RedeemPaymentTokens final {
 public:
  RedeemPaymentTokens();

  RedeemPaymentTokens(const RedeemPaymentTokens&) = delete;
  RedeemPaymentTokens& operator=(const RedeemPaymentTokens&) = delete;

  RedeemPaymentTokens(RedeemPaymentTokens&&) noexcept = delete;
  RedeemPaymentTokens& operator=(RedeemPaymentTokens&&) noexcept = delete;

  ~RedeemPaymentTokens();

  void SetDelegate(RedeemPaymentTokensDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeRedeemAfterDelay(const WalletInfo& wallet);

 private:
  void RedeemAfterDelay();
  void Redeem();
  void RedeemCallback(const PaymentTokenList& payment_tokens,
                      const mojom::UrlResponseInfo& mojom_url_response);

  static base::expected<void, std::tuple<std::string, bool>> HandleUrlResponse(
      const mojom::UrlResponseInfo& mojom_url_response);

  void SuccessfullyRedeemed(const PaymentTokenList& payment_tokens);
  void FailedToRedeem(bool should_retry);

  void ScheduleNextRedemption();

  void Retry();
  void RetryCallback();
  void StopRetrying();

  void NotifyDidRedeemPaymentTokens(
      const PaymentTokenList& payment_tokens) const;
  void NotifyFailedToRedeemPaymentTokens() const;
  void NotifyDidScheduleNextPaymentTokenRedemption(base::Time redeem_at) const;
  void NotifyWillRetryRedeemingPaymentTokens(base::Time retry_at) const;
  void NotifyDidRetryRedeemingPaymentTokens() const;

  raw_ptr<RedeemPaymentTokensDelegate> delegate_ = nullptr;

  WalletInfo wallet_;

  bool is_redeeming_ = false;

  BackoffTimer timer_;

  base::WeakPtrFactory<RedeemPaymentTokens> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_REDEEM_PAYMENT_TOKENS_H_
