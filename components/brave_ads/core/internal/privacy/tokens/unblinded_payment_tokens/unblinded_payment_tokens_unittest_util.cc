/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"

namespace brave_ads::privacy {

UnblindedPaymentTokens& GetUnblindedPaymentTokens() {
  return ConfirmationStateManager::GetInstance().GetUnblindedPaymentTokens();
}

UnblindedPaymentTokenList SetUnblindedPaymentTokens(const int count) {
  CHECK_GT(count, 0);

  UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildUnblindedPaymentTokens(count);
  GetUnblindedPaymentTokens().SetTokens(unblinded_payment_tokens);
  return unblinded_payment_tokens;
}

UnblindedPaymentTokenInfo CreateUnblindedPaymentToken(
    const std::string& unblinded_payment_token_base64) {
  UnblindedPaymentTokenInfo unblinded_payment_token;

  unblinded_payment_token.transaction_id =
      "0d9de7ce-b3f9-4158-8726-23d52b9457c6";

  unblinded_payment_token.value =
      cbr::UnblindedToken(unblinded_payment_token_base64);
  CHECK(unblinded_payment_token.value.has_value());

  unblinded_payment_token.public_key =
      cbr::PublicKey("RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");
  CHECK(unblinded_payment_token.public_key.has_value());

  unblinded_payment_token.confirmation_type = ConfirmationType::kViewed;

  unblinded_payment_token.ad_type = AdType::kNotificationAd;

  return unblinded_payment_token;
}

UnblindedPaymentTokenInfo CreateUnblindedPaymentToken(
    const ConfirmationType& confirmation_type,
    const AdType& ad_type) {
  const std::string unblinded_payment_token_base64 =
      R"(PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY)";
  UnblindedPaymentTokenInfo unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  unblinded_payment_token.confirmation_type = confirmation_type;
  unblinded_payment_token.ad_type = ad_type;

  return unblinded_payment_token;
}

UnblindedPaymentTokenList CreateUnblindedPaymentTokens(
    const std::vector<std::string>& unblinded_payment_tokens_base64) {
  UnblindedPaymentTokenList unblinded_payment_tokens;

  for (const auto& unblinded_payment_token_base64 :
       unblinded_payment_tokens_base64) {
    const UnblindedPaymentTokenInfo unblinded_payment_token =
        CreateUnblindedPaymentToken(unblinded_payment_token_base64);

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

UnblindedPaymentTokenList BuildUnblindedPaymentTokens(const int count) {
  CHECK_GT(count, 0);

  const std::vector<std::string> unblinded_payment_tokens_base64 = {
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

  const size_t modulo = unblinded_payment_tokens_base64.size();

  UnblindedPaymentTokenList unblinded_payment_tokens;

  for (int i = 0; i < count; i++) {
    const std::string& unblinded_payment_token_base64 =
        unblinded_payment_tokens_base64.at(i % modulo);
    const UnblindedPaymentTokenInfo unblinded_payment_token =
        CreateUnblindedPaymentToken(unblinded_payment_token_base64);

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

UnblindedPaymentTokenInfo BuildUnblindedPaymentToken() {
  const UnblindedPaymentTokenList unblinded_payment_tokens =
      BuildUnblindedPaymentTokens(/*count*/ 1);
  CHECK(!unblinded_payment_tokens.empty());
  return unblinded_payment_tokens.front();
}

}  // namespace brave_ads::privacy
