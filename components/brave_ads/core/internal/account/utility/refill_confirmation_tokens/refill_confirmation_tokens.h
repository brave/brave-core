/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_H_

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens_delegate.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/blinded_token.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/token.h"
#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

class RefillConfirmationTokens final {
 public:
  RefillConfirmationTokens();

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
  void FailedToRefillAndRetry();
  void FailedToRefill();

  void Retry();
  void RetryCallback();
  void StopRetrying();

  void Reset();

  void NotifyWillRefillConfirmationTokens() const;
  void NotifyCaptchaRequiredToRefillConfirmationTokens(
      const std::string& captcha_id) const;
  void NotifyDidRefillConfirmationTokens() const;
  void NotifyFailedToRefillConfirmationTokens() const;
  void NotifyWillRetryRefillingConfirmationTokens(base::Time retry_at) const;
  void NotifyDidRetryRefillingConfirmationTokens() const;

  raw_ptr<RefillConfirmationTokensDelegate> delegate_ = nullptr;

  WalletInfo wallet_;

  std::optional<std::string> nonce_;

  std::optional<std::vector<cbr::Token>> tokens_;
  std::optional<std::vector<cbr::BlindedToken>> blinded_tokens_;

  bool is_refilling_ = false;

  BackoffTimer timer_;

  base::WeakPtrFactory<RefillConfirmationTokens> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REFILL_CONFIRMATION_TOKENS_REFILL_CONFIRMATION_TOKENS_H_
