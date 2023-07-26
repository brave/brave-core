/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_H_

#include <string>
#include <tuple>
#include <vector>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/account/tokens/refill_confirmation_tokens/refill_confirmation_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/token.h"

namespace brave_ads {

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class RefillConfirmationTokens final {
 public:
  explicit RefillConfirmationTokens(
      privacy::TokenGeneratorInterface* token_generator);

  RefillConfirmationTokens(const RefillConfirmationTokens&) = delete;
  RefillConfirmationTokens& operator=(const RefillConfirmationTokens&) = delete;

  RefillConfirmationTokens(RefillConfirmationTokens&&) noexcept = delete;
  RefillConfirmationTokens& operator=(RefillConfirmationTokens&&) noexcept =
      delete;

  ~RefillConfirmationTokens();

  void SetDelegate(RefillConfirmationTokensDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void MaybeRefill(const WalletInfo& wallet);

 private:
  void Refill();

  void GenerateTokens();

  bool ShouldRequestSignedTokens() const;
  void RequestSignedTokens();
  void RequestSignedTokensCallback(const mojom::UrlResponseInfo& url_response);
  base::expected<void, std::tuple<std::string, /*should_retry*/ bool>>
  HandleRequestSignedTokensUrlResponse(
      const mojom::UrlResponseInfo& url_response);

  void GetSignedTokens();
  void GetSignedTokensCallback(const mojom::UrlResponseInfo& url_response);
  base::expected<void, std::tuple<std::string, /*should_retry*/ bool>>
  HandleGetSignedTokensUrlResponse(const mojom::UrlResponseInfo& url_response);
  void ParseAndRequireCaptcha(const base::Value::Dict& dict) const;

  void SuccessfullyRefilled();
  void FailedToRefill(bool should_retry);

  void Retry();
  void RetryCallback();
  void StopRetrying();

  void Reset();

  const raw_ptr<privacy::TokenGeneratorInterface> token_generator_ =
      nullptr;  // NOT OWNED

  raw_ptr<RefillConfirmationTokensDelegate> delegate_ = nullptr;

  WalletInfo wallet_;

  absl::optional<std::string> nonce_;

  absl::optional<std::vector<privacy::cbr::Token>> tokens_;
  absl::optional<std::vector<privacy::cbr::BlindedToken>> blinded_tokens_;

  bool is_processing_ = false;

  BackoffTimer retry_timer_;

  base::WeakPtrFactory<RefillConfirmationTokens> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_H_
