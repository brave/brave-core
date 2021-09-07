/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_H_

#include "bat/ads/internal/tokens/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

class ConfirmationType;
struct ConfirmationInfo;

namespace privacy {
struct UnblindedTokenInfo;
}  // namespace privacy

class RedeemUnblindedToken {
 public:
  RedeemUnblindedToken();

  ~RedeemUnblindedToken();

  void set_delegate(RedeemUnblindedTokenDelegate* delegate);

  void Redeem(const ConfirmationInfo& confirmation);

 private:
  void CreateConfirmation(const ConfirmationInfo& confirmation);
  void OnCreateConfirmation(const mojom::UrlResponse& url_response,
                            const ConfirmationInfo& confirmation);

  void FetchPaymentToken(const ConfirmationInfo& confirmation);
  void OnFetchPaymentToken(const mojom::UrlResponse& url_response,
                           const ConfirmationInfo& confirmation);

  void OnDidRedeemUnblindedToken(
      const ConfirmationInfo& confirmation,
      const privacy::UnblindedTokenInfo& unblinded_payment_token);

  void OnFailedToRedeemUnblindedToken(const ConfirmationInfo& confirmation,
                                      const bool should_retry);

  RedeemUnblindedTokenDelegate* delegate_ = nullptr;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_H_
