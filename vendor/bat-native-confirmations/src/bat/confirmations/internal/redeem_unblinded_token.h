/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_TOKEN_H_
#define BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_TOKEN_H_

#include <map>
#include <string>

#include "bat/confirmations/confirmations_client.h"
#include "bat/confirmations/internal/redeem_unblinded_token_delegate.h"

namespace confirmations {

class ConfirmationsImpl;
class ConfirmationType;
class UnblindedTokens;
struct AdInfo;
struct ConfirmationInfo;
struct TokenInfo;

class RedeemUnblindedToken {
 public:
  RedeemUnblindedToken(
      ConfirmationsImpl* confirmations,
      UnblindedTokens* unblinded_tokens,
      UnblindedTokens* unblinded_payment_tokens);

  ~RedeemUnblindedToken();

  void set_delegate(
      RedeemUnblindedTokenDelegate* delegate);

  void Redeem(
      const AdInfo& ad,
      const ConfirmationType confirmation_type);
  void Redeem(
      const std::string& creative_instance_id,
      const std::string& creative_set_id,
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
      const TokenInfo& token);

  bool Verify(
     const ConfirmationInfo& confirmation) const;

  ConfirmationsImpl* confirmations_;  // NOT OWNED

  UnblindedTokens* unblinded_tokens_;  // NOT OWNED
  UnblindedTokens* unblinded_payment_tokens_;  // NOT OWNED

  RedeemUnblindedTokenDelegate* delegate_ = nullptr;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_INTERNAL_REDEEM_UNBLINDED_TOKEN_H_
