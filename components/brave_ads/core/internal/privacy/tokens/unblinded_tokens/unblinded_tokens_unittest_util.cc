/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens.h"

namespace brave_ads::privacy {

UnblindedTokens& GetUnblindedTokens() {
  return ConfirmationStateManager::GetInstance().GetUnblindedTokens();
}

UnblindedTokenList SetUnblindedTokens(const int count) {
  CHECK_GT(count, 0);

  UnblindedTokenList unblinded_tokens = BuildUnblindedTokens(count);
  GetUnblindedTokens().SetTokens(unblinded_tokens);
  return unblinded_tokens;
}

UnblindedTokenInfo CreateUnblindedToken(
    const std::string& unblinded_token_base64,
    const WalletInfo& wallet) {
  UnblindedTokenInfo unblinded_token;

  unblinded_token.value = cbr::UnblindedToken(unblinded_token_base64);

  unblinded_token.public_key =
      cbr::PublicKey("RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");

  const absl::optional<std::string> signature =
      crypto::Sign(unblinded_token_base64, wallet.secret_key);
  CHECK(signature);
  unblinded_token.signature = *signature;

  CHECK(IsValid(unblinded_token));

  return unblinded_token;
}

UnblindedTokenList CreateUnblindedTokens(
    const std::vector<std::string>& unblinded_tokens_base64,
    const WalletInfo& wallet) {
  UnblindedTokenList unblinded_tokens;

  for (const auto& unblinded_token_base64 : unblinded_tokens_base64) {
    const UnblindedTokenInfo unblinded_token =
        CreateUnblindedToken(unblinded_token_base64, wallet);

    unblinded_tokens.push_back(unblinded_token);
  }

  return unblinded_tokens;
}

UnblindedTokenList BuildUnblindedTokens(const int count) {
  CHECK_GT(count, 0);

  const WalletInfo& wallet = GetWalletForTesting();

  const std::vector<std::string> unblinded_tokens_base64 = {
      R"(PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY)",
      R"(hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K)",
      R"(bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa)",
      R"(OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU)",
      R"(Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi)",
      R"(+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvVZGYXLeLn2HSAM7cDgqyW6SEuPzlDeZT6kkTNI7JcQm)",
      R"(CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkj5pW/rg7nl48EX6nmekgd3D2Hz8JgJnSarzP/8+3l+MW)",
      R"(hQ+6+jh5DUUBFhhGn7bPLDjqrUIKNi/T8QDt1x01bcW9PLADg6aS73dzrVBsHav44+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z)",
      R"(6tKJHOtQqpNzFjLGT0gvXlCF0GGKrqQlK82e2tc7gJvQkorg60Y21jEAg8JHbU8D3mBK/riZCILoi1cPCiBDAdhWJNVm003mZ0ShjmbESnKhL/NxRv/0/PB3GQ5iydoc)",
      R"(ujGlRHnz+UF0h8i6gYDnfeZDUj7qZZz6o29ZJFa3XN2g+yVXgRTws1yv6RAtLCr39OQso6FAT12o8GAvHVEzmRqyzm2XU9gMK5WrNtT/fhr8gQ9RvupdznGKOqmVbuIc)"};

  const size_t modulo = unblinded_tokens_base64.size();

  UnblindedTokenList unblinded_tokens;

  for (int i = 0; i < count; i++) {
    const std::string& unblinded_token_base64 =
        unblinded_tokens_base64.at(i % modulo);
    const UnblindedTokenInfo unblinded_token =
        CreateUnblindedToken(unblinded_token_base64, wallet);

    unblinded_tokens.push_back(unblinded_token);
  }

  return unblinded_tokens;
}

UnblindedTokenInfo BuildUnblindedToken() {
  const UnblindedTokenList unblinded_tokens = BuildUnblindedTokens(/*count*/ 1);
  CHECK(!unblinded_tokens.empty());
  return unblinded_tokens.front();
}

}  // namespace brave_ads::privacy
