/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
#define BAT_ADS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_

#include "base/time/time.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/internal/timer.h"
#include "bat/ads/internal/tokens/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_delegate.h"
#include "bat/ads/mojom.h"

namespace ads {

class RedeemUnblindedPaymentTokens {
 public:
  RedeemUnblindedPaymentTokens();

  ~RedeemUnblindedPaymentTokens();

  void set_delegate(
      RedeemUnblindedPaymentTokensDelegate* delegate);

  void MaybeRedeemAfterDelay(
      const WalletInfo& wallet);

 private:
  WalletInfo wallet_;

  Timer timer_;

  void Redeem();
  void OnRedeem(
      const UrlResponse& url_response);

  void OnDidRedeemUnblindedPaymentTokens();

  void OnFailedToRedeemUnblindedPaymentTokens();

  void ScheduleNextTokenRedemption();

  BackoffTimer retry_timer_;
  void Retry();
  void OnRetry();

  bool is_processing_ = false;

  base::TimeDelta CalculateTokenRedemptionDelay();
  base::Time CalculateNextTokenRedemptionDate();

  RedeemUnblindedPaymentTokensDelegate* delegate_ = nullptr;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_REDEEM_UNBLINDED_PAYMENT_TOKENS_H_
