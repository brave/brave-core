/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/url_requests/get_signed_tokens/get_signed_tokens_url_request_util.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

namespace {

constexpr char kCaptchaIdKey[] = "captcha_id";

std::optional<std::string> Sign(const cbr::UnblindedToken& unblinded_token,
                                const WalletInfo& wallet) {
  const std::optional<std::string> unblinded_token_base64 =
      unblinded_token.EncodeBase64();
  if (!unblinded_token_base64) {
    return std::nullopt;
  }

  return crypto::Sign(*unblinded_token_base64, wallet.secret_key_base64);
}

ConfirmationTokenList BuildConfirmationTokens(
    const std::vector<cbr::UnblindedToken>& unblinded_tokens,
    const cbr::PublicKey& public_key,
    const WalletInfo& wallet) {
  ConfirmationTokenList confirmation_tokens;
  confirmation_tokens.reserve(unblinded_tokens.size());

  for (const cbr::UnblindedToken& unblinded_token : unblinded_tokens) {
    const std::optional<std::string> signature = Sign(unblinded_token, wallet);
    CHECK(signature);

    confirmation_tokens.emplace_back(unblinded_token, public_key, *signature);
  }

  return confirmation_tokens;
}

}  // namespace

std::optional<std::string> ParseCaptchaId(const base::Value::Dict& dict) {
  const std::string* const captcha_id = dict.FindString(kCaptchaIdKey);
  if (!captcha_id || captcha_id->empty()) {
    return std::nullopt;
  }

  return *captcha_id;
}

void BuildAndAddConfirmationTokens(
    const std::vector<cbr::UnblindedToken>& unblinded_tokens,
    const cbr::PublicKey& public_key,
    const WalletInfo& wallet) {
  const ConfirmationTokenList confirmation_tokens =
      BuildConfirmationTokens(unblinded_tokens, public_key, wallet);

  AddConfirmationTokens(confirmation_tokens);

  BLOG(1, "Added " << confirmation_tokens.size()
                   << " confirmation tokens, you now have "
                   << ConfirmationTokenCount() << " confirmation tokens");
}

}  // namespace brave_ads
