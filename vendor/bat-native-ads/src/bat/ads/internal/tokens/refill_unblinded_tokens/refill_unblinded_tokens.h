/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/backoff_timer.h"
#include "bat/ads/internal/privacy/tokens/token_generator_interface.h"
#include "bat/ads/internal/tokens/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "wrapper.hpp"

namespace ads {

using challenge_bypass_ristretto::BlindedToken;
using challenge_bypass_ristretto::Token;

class RefillUnblindedTokens {
 public:
  explicit RefillUnblindedTokens(
      privacy::TokenGeneratorInterface* token_generator);

  ~RefillUnblindedTokens();

  void set_delegate(RefillUnblindedTokensDelegate* delegate);

  void MaybeRefill(const WalletInfo& wallet);

 private:
  WalletInfo wallet_;

  std::string public_key_;

  std::string nonce_;

  std::vector<Token> tokens_;
  std::vector<BlindedToken> blinded_tokens_;

  void Refill();

  void MaybeGetScheduledCaptcha();
  void GetScheduledCaptcha();
  void OnGetScheduledCaptcha(const std::string& captcha_id);

  void RequestSignedTokens();
  void OnRequestSignedTokens(const mojom::UrlResponse& url_response);

  void GetSignedTokens();
  void OnGetSignedTokens(const mojom::UrlResponse& url_response);

  void OnDidRefillUnblindedTokens();

  void OnFailedToRefillUnblindedTokens(const bool should_retry);

  BackoffTimer retry_timer_;
  void Retry();
  void OnRetry();

  bool ShouldRefillUnblindedTokens() const;

  int CalculateAmountOfTokensToRefill() const;

  bool is_processing_ = false;

  privacy::TokenGeneratorInterface* token_generator_;  // NOT OWNED

  RefillUnblindedTokensDelegate* delegate_ = nullptr;

  base::WeakPtrFactory<RefillUnblindedTokens> weak_ptr_factory_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_H_
