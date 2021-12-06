/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"

#include <utility>

#include "base/check.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto_util.h"
#include "bat/ads/internal/privacy/tokens/token_generator.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_token_info.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "wrapper.hpp"

namespace ads {
namespace privacy {

using challenge_bypass_ristretto::PublicKey;
using challenge_bypass_ristretto::UnblindedToken;

UnblindedPaymentTokens* get_unblinded_payment_tokens() {
  return ConfirmationsState::Get()->get_unblinded_payment_tokens();
}

UnblindedPaymentTokenList SetUnblindedPaymentTokens(const int count) {
  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      GetUnblindedPaymentTokens(count);
  get_unblinded_payment_tokens()->SetTokens(unblinded_payment_tokens);
  return unblinded_payment_tokens;
}

UnblindedPaymentTokenInfo CreateUnblindedPaymentToken(
    const std::string& unblinded_payment_token_base64) {
  UnblindedPaymentTokenInfo unblinded_payment_token;

  unblinded_payment_token.transaction_id =
      "0d9de7ce-b3f9-4158-8726-23d52b9457c6";

  unblinded_payment_token.value =
      UnblindedToken::decode_base64(unblinded_payment_token_base64);
  DCHECK(!ExceptionOccurred());

  unblinded_payment_token.public_key =
      PublicKey::decode_base64("RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");
  DCHECK(!ExceptionOccurred());

  unblinded_payment_token.confirmation_type = ConfirmationType::kViewed;

  unblinded_payment_token.ad_type = AdType::kAdNotification;

  return unblinded_payment_token;
}

UnblindedPaymentTokenList CreateUnblindedPaymentTokens(
    const std::vector<std::string>& unblinded_payment_tokens_base64) {
  UnblindedPaymentTokenList unblinded_payment_tokens;

  for (const auto& unblinded_payment_token_base64 :
       unblinded_payment_tokens_base64) {
    const UnblindedPaymentTokenInfo& unblinded_payment_token =
        CreateUnblindedPaymentToken(unblinded_payment_token_base64);

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

UnblindedPaymentTokenList GetUnblindedPaymentTokens(const int count) {
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

  const int modulo = unblinded_payment_tokens_base64.size();

  UnblindedPaymentTokenList unblinded_payment_tokens;

  for (int i = 0; i < count; i++) {
    const std::string& unblinded_payment_token_base64 =
        unblinded_payment_tokens_base64.at(i % modulo);
    const UnblindedPaymentTokenInfo& unblinded_payment_token =
        CreateUnblindedPaymentToken(unblinded_payment_token_base64);

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

UnblindedPaymentTokenList GetRandomUnblindedPaymentTokens(const int count) {
  UnblindedPaymentTokenList unblinded_payment_tokens;

  TokenGenerator token_generator;
  const std::vector<Token>& tokens = token_generator.Generate(count);
  for (const auto& token : tokens) {
    const std::string& token_base64 = token.encode_base64();
    const UnblindedPaymentTokenInfo& unblinded_payment_token =
        CreateUnblindedPaymentToken(token_base64);

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

base::Value GetUnblindedPaymentTokensAsList(const int count) {
  base::Value list(base::Value::Type::LIST);

  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      GetUnblindedPaymentTokens(count);

  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    base::Value dictionary(base::Value::Type::DICTIONARY);

    dictionary.SetKey("transaction_id",
                      base::Value(unblinded_payment_token.transaction_id));

    dictionary.SetKey(
        "unblinded_token",
        base::Value(unblinded_payment_token.value.encode_base64()));

    dictionary.SetKey(
        "public_key",
        base::Value(unblinded_payment_token.public_key.encode_base64()));

    dictionary.SetKey(
        "confirmation_type",
        base::Value(std::string(unblinded_payment_token.confirmation_type)));

    dictionary.SetKey(
        "ad_type", base::Value(std::string(unblinded_payment_token.ad_type)));

    list.Append(std::move(dictionary));
  }

  return list;
}

}  // namespace privacy
}  // namespace ads
