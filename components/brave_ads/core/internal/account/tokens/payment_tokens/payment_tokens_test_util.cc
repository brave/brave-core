/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_test_util.h"

#include <cstddef>
#include <string>
#include <vector>

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/public_key.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

namespace brave_ads::test {

namespace {

PaymentTokenInfo BuildPaymentToken(const std::string& payment_token_base64) {
  PaymentTokenInfo payment_token;

  payment_token.transaction_id = test::kTransactionId;

  payment_token.unblinded_token = cbr::UnblindedToken(payment_token_base64);
  CHECK(payment_token.unblinded_token.has_value());

  payment_token.public_key =
      cbr::PublicKey("RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");
  CHECK(payment_token.public_key.has_value());

  payment_token.confirmation_type = mojom::ConfirmationType::kViewedImpression;

  payment_token.ad_type = mojom::AdType::kNotificationAd;

  return payment_token;
}

}  // namespace

PaymentTokens& GetPaymentTokens() {
  return ConfirmationStateManager::GetInstance().GetPaymentTokens();
}

PaymentTokenList SetPaymentTokens(int count) {
  CHECK_GT(count, 0);

  PaymentTokenList payment_tokens = BuildPaymentTokens(count);
  GetPaymentTokens().SetTokens(payment_tokens);
  return payment_tokens;
}

PaymentTokenInfo BuildPaymentToken(
    mojom::ConfirmationType mojom_confirmation_type,
    mojom::AdType mojom_ad_type) {
  const std::string payment_token_base64 =
      R"(PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY)";
  PaymentTokenInfo payment_token = BuildPaymentToken(payment_token_base64);

  payment_token.confirmation_type = mojom_confirmation_type;
  payment_token.ad_type = mojom_ad_type;

  return payment_token;
}

PaymentTokenInfo BuildPaymentToken() {
  const PaymentTokenList payment_tokens = BuildPaymentTokens(/*count=*/1);
  CHECK(!payment_tokens.empty());
  return payment_tokens.front();
}

PaymentTokenList BuildPaymentTokens(int count) {
  CHECK_GT(count, 0);

  const std::vector<std::string> payment_tokens_base64 = {
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

  const size_t modulo = payment_tokens_base64.size();

  PaymentTokenList payment_tokens;

  for (int i = 0; i < count; ++i) {
    const std::string& payment_token_base64 =
        payment_tokens_base64.at(i % modulo);
    const PaymentTokenInfo payment_token =
        BuildPaymentToken(payment_token_base64);

    payment_tokens.push_back(payment_token);
  }

  return payment_tokens;
}

}  // namespace brave_ads::test
