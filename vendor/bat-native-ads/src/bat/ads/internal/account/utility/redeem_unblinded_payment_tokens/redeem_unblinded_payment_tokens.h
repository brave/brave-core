/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/base/backoff_timer.h"
#include "bat/ads/internal/base/timer.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace ads {

class RedeemUnblindedPaymentTokens final {
 public:
  RedeemUnblindedPaymentTokens();
  ~RedeemUnblindedPaymentTokens();

  void set_delegate(RedeemUnblindedPaymentTokensDelegate* delegate) {
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeRedeemAfterDelay(const WalletInfo& wallet);

 private:
  void Redeem();
  void OnRedeem(
      const mojom::UrlResponse& url_response,
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens);

  void OnDidRedeemUnblindedPaymentTokens(
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens);

  void OnFailedToRedeemUnblindedPaymentTokens();

  void ScheduleNextTokenRedemption();

  void Retry();
  void OnRetry();

  base::TimeDelta CalculateTokenRedemptionDelay();
  base::Time CalculateNextTokenRedemptionDate();

  raw_ptr<RedeemUnblindedPaymentTokensDelegate> delegate_ = nullptr;

  WalletInfo wallet_;

  bool is_processing_ = false;

  Timer timer_;
  BackoffTimer retry_timer_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
