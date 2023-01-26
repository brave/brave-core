/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/common/timer/backoff_timer.h"
#include "bat/ads/internal/common/timer/timer.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads {

class RedeemUnblindedPaymentTokens final {
 public:
  RedeemUnblindedPaymentTokens();

  RedeemUnblindedPaymentTokens(const RedeemUnblindedPaymentTokens& other) =
      delete;
  RedeemUnblindedPaymentTokens& operator=(const RedeemUnblindedPaymentTokens&) =
      delete;

  RedeemUnblindedPaymentTokens(RedeemUnblindedPaymentTokens&& other) noexcept =
      delete;
  RedeemUnblindedPaymentTokens& operator=(
      RedeemUnblindedPaymentTokens&& other) noexcept = delete;

  ~RedeemUnblindedPaymentTokens();

  void SetDelegate(RedeemUnblindedPaymentTokensDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
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
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
