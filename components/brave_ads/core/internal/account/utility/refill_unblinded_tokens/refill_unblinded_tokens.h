/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_H_

#include <string>
#include <vector>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_unblinded_tokens/refill_unblinded_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"

namespace brave_ads {

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class RefillUnblindedTokens final {
 public:
  explicit RefillUnblindedTokens(
      privacy::TokenGeneratorInterface* token_generator);

  RefillUnblindedTokens(const RefillUnblindedTokens&) = delete;
  RefillUnblindedTokens& operator=(const RefillUnblindedTokens&) = delete;

  RefillUnblindedTokens(RefillUnblindedTokens&&) noexcept = delete;
  RefillUnblindedTokens& operator=(RefillUnblindedTokens&&) noexcept = delete;

  ~RefillUnblindedTokens();

  void SetDelegate(RefillUnblindedTokensDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeRefill(const WalletInfo& wallet);

 private:
  void Refill();

  void RequestSignedTokens();
  void OnRequestSignedTokens(const mojom::UrlResponseInfo& url_response);

  void GetSignedTokens();
  void OnGetSignedTokens(const mojom::UrlResponseInfo& url_response);

  void SuccessfullyRefilledUnblindedTokens();

  void FailedToRefillUnblindedTokens(bool should_retry);

  void Retry();
  void OnRetry();

  const raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  raw_ptr<RefillUnblindedTokensDelegate> delegate_ = nullptr;

  WalletInfo wallet_;

  std::string nonce_;

  std::vector<privacy::cbr::Token> tokens_;
  std::vector<privacy::cbr::BlindedToken> blinded_tokens_;

  bool is_processing_ = false;

  BackoffTimer retry_timer_;

  base::WeakPtrFactory<RefillUnblindedTokens> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_UNBLINDED_TOKENS_REFILL_UNBLINDED_TOKENS_H_
