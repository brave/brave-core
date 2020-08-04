/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_H_
#define BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_H_

#include <string>

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"
#include "bat/ads/internal/server/redeem_unblinded_token/redeem_unblinded_token_delegate.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

class AdsImpl;
class ConfirmationType;
struct AdInfo;
struct ConfirmationInfo;
struct TokenInfo;

class RedeemUnblindedToken {
 public:
  RedeemUnblindedToken(
      AdsImpl* ads);

  ~RedeemUnblindedToken();

  void set_delegate(
      RedeemUnblindedTokenDelegate* delegate);

  void Redeem(
      const AdInfo& ad,
      const ConfirmationType confirmation_type);
  void Redeem(
      const ConfirmationInfo& confirmation);

 private:
  void CreateConfirmation(
      const ConfirmationInfo& confirmation);
  void OnCreateConfirmation(
      const UrlResponse& url_response,
      const ConfirmationInfo& confirmation);

  void FetchPaymentToken(
      const ConfirmationInfo& confirmation);
  void OnFetchPaymentToken(
      const UrlResponse& url_response,
      const ConfirmationInfo& confirmation);

  void OnRedeem(
      const Result result,
      const ConfirmationInfo& confirmation,
      const bool should_retry);

  void CreateAndAppendNewConfirmationToRetryQueue(
      const ConfirmationInfo& confirmation);
  void AppendConfirmationToRetryQueue(
      const ConfirmationInfo& confirmation);

  ConfirmationInfo CreateConfirmationInfo(
      const AdInfo& ad,
      const ConfirmationType confirmation_type,
      const privacy::UnblindedTokenInfo& unblinded_token);

  AdsImpl* ads_;  // NOT OWNED

  RedeemUnblindedTokenDelegate* delegate_ = nullptr;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_SERVER_REDEEM_UNBLINDED_TOKEN_REDEEM_UNBLINDED_TOKEN_H_  // NOLINT
