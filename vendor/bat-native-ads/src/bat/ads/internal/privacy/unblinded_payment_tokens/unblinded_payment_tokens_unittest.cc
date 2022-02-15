/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {

class BatAdsUnblindedPaymentTokensTest : public UnitTestBase {
 protected:
  BatAdsUnblindedPaymentTokensTest() = default;

  ~BatAdsUnblindedPaymentTokensTest() override = default;
};

TEST_F(BatAdsUnblindedPaymentTokensTest, GetToken) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(10);

  // Act
  const UnblindedPaymentTokenInfo& unblinded_payment_token =
      get_unblinded_payment_tokens()->GetToken();

  // Assert
  const std::string expected_unblinded_payment_token_base64 =
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";
  UnblindedPaymentTokenInfo expected_unblinded_payment_token =
      CreateUnblindedPaymentToken(expected_unblinded_payment_token_base64);

  EXPECT_EQ(expected_unblinded_payment_token, unblinded_payment_token);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, GetAllTokens) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(8);

  // Act
  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      get_unblinded_payment_tokens()->GetAllTokens();

  // Assert
  const std::vector<std::string> expected_unblinded_payment_tokens_base64 = {
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY",
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K",
      "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2"
      "AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa",
      "OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEp"
      "BrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU",
      "Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbV"
      "T3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi",
      "+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKul"
      "bCvsRna7++MhbBz6iC0FvVZGYXLeLn2HSAM7cDgqyW6SEuPzlDeZT6kkTNI7JcQm",
      "CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqq"
      "fhIcLhD0f1WFod7JpuEkj5pW/rg7nl48EX6nmekgd3D2Hz8JgJnSarzP/8+3l+MW",
      "hQ+6+jh5DUUBFhhGn7bPLDjqrUIKNi/T8QDt1x01bcW9PLADg6aS73dzrVBsHav4"
      "4+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z"};
  UnblindedPaymentTokenList expected_unblinded_payment_tokens =
      CreateUnblindedPaymentTokens(expected_unblinded_payment_tokens_base64);

  EXPECT_EQ(expected_unblinded_payment_tokens, unblinded_payment_tokens);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, GetTokensAsList) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(8);

  // Act
  const base::Value& list = get_unblinded_payment_tokens()->GetTokensAsList();

  // Assert
  base::ListValue list_values(list.GetListDeprecated());

  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      get_unblinded_payment_tokens()->GetAllTokens();
  EXPECT_EQ(list_values.GetListDeprecated().size(),
            unblinded_payment_tokens.size());

  for (auto& value : list_values.GetListDeprecated()) {
    base::DictionaryValue* dictionary = nullptr;
    if (!value.GetAsDictionary(&dictionary)) {
      FAIL();
    }

    const std::string* unblinded_payment_token_value =
        dictionary->FindStringKey("unblinded_token");
    if (!unblinded_payment_token_value) {
      FAIL();
    }
    const std::string unblinded_payment_token_base64 =
        *unblinded_payment_token_value;

    const std::string* public_key_value =
        dictionary->FindStringKey("public_key");
    if (!public_key_value) {
      FAIL();
    }
    const std::string public_key_base64 = *public_key_value;

    UnblindedPaymentTokenInfo unblinded_payment_token;
    unblinded_payment_token.transaction_id =
        "0d9de7ce-b3f9-4158-8726-23d52b9457c6";
    unblinded_payment_token.value =
        UnblindedToken::decode_base64(unblinded_payment_token_base64);
    unblinded_payment_token.public_key =
        PublicKey::decode_base64(public_key_base64);
    unblinded_payment_token.confirmation_type = ConfirmationType::kViewed;
    unblinded_payment_token.ad_type = AdType::kAdNotification;

    if (!get_unblinded_payment_tokens()->TokenExists(unblinded_payment_token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatAdsUnblindedPaymentTokensTest, GetTokensAsListWithEmptyList) {
  // Arrange

  // Act
  const base::Value& list = get_unblinded_payment_tokens()->GetTokensAsList();

  // Assert
  EXPECT_TRUE(list.GetListDeprecated().empty());
}

TEST_F(BatAdsUnblindedPaymentTokensTest, SetTokens) {
  // Arrange
  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      GetUnblindedPaymentTokens(10);

  // Act
  get_unblinded_payment_tokens()->SetTokens(unblinded_payment_tokens);

  // Assert
  const UnblindedPaymentTokenList& expected_unblinded_payment_tokens =
      get_unblinded_payment_tokens()->GetAllTokens();

  EXPECT_EQ(expected_unblinded_payment_tokens, unblinded_payment_tokens);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, SetTokensWithEmptyList) {
  // Arrange
  const UnblindedPaymentTokenList& unblinded_payment_tokens = {};

  // Act
  get_unblinded_payment_tokens()->SetTokens(unblinded_payment_tokens);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, SetTokensFromList) {
  // Arrange
  const base::Value& list = GetUnblindedPaymentTokensAsList(5);

  // Act
  get_unblinded_payment_tokens()->SetTokensFromList(list);

  // Assert
  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      get_unblinded_payment_tokens()->GetAllTokens();

  const std::vector<std::string> expected_unblinded_payment_tokens_base64 = {
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY",
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K",
      "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2"
      "AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa",
      "OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEp"
      "BrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU",
      "Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbV"
      "T3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi"};

  UnblindedPaymentTokenList expected_unblinded_payment_tokens;
  for (const auto& unblinded_payment_token_base64 :
       expected_unblinded_payment_tokens_base64) {
    UnblindedPaymentTokenInfo unblinded_payment_token;

    unblinded_payment_token.transaction_id =
        "0d9de7ce-b3f9-4158-8726-23d52b9457c6";
    unblinded_payment_token.value =
        UnblindedToken::decode_base64(unblinded_payment_token_base64);
    unblinded_payment_token.public_key = PublicKey::decode_base64(
        "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");
    unblinded_payment_token.confirmation_type = ConfirmationType::kViewed;
    unblinded_payment_token.ad_type = AdType::kAdNotification;

    expected_unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  EXPECT_EQ(expected_unblinded_payment_tokens, unblinded_payment_tokens);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, SetTokensFromListWithEmptyList) {
  // Arrange
  const base::Value& list = GetUnblindedPaymentTokensAsList(0);

  // Act
  get_unblinded_payment_tokens()->SetTokensFromList(list);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, AddTokens) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  const UnblindedPaymentTokenList& unblinded_payment_tokens =
      GetRandomUnblindedPaymentTokens(5);
  get_unblinded_payment_tokens()->AddTokens(unblinded_payment_tokens);

  // Assert
  for (const auto& unblinded_payment_token : unblinded_payment_tokens) {
    if (!get_unblinded_payment_tokens()->TokenExists(unblinded_payment_token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatAdsUnblindedPaymentTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  const UnblindedPaymentTokenList& duplicate_unblinded_payment_tokens =
      GetUnblindedPaymentTokens(1);
  get_unblinded_payment_tokens()->AddTokens(duplicate_unblinded_payment_tokens);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, AddTokensCount) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(5);

  // Act
  const UnblindedPaymentTokenList& random_unblinded_payment_tokens =
      GetRandomUnblindedPaymentTokens(3);
  get_unblinded_payment_tokens()->AddTokens(random_unblinded_payment_tokens);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(8, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, AddTokensWithEmptyList) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  const UnblindedPaymentTokenList& empty_unblinded_payment_tokens = {};
  get_unblinded_payment_tokens()->AddTokens(empty_unblinded_payment_tokens);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveTokenCount) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  const std::string unblinded_payment_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedPaymentTokenInfo& unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  get_unblinded_payment_tokens()->RemoveToken(unblinded_payment_token);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveToken) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  std::string unblinded_payment_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedPaymentTokenInfo& unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  get_unblinded_payment_tokens()->RemoveToken(unblinded_payment_token);

  // Assert
  EXPECT_FALSE(
      get_unblinded_payment_tokens()->TokenExists(unblinded_payment_token));
}

TEST_F(BatAdsUnblindedPaymentTokensTest, DoNotRemoveTokensThatDoNotExist) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  std::string unblinded_payment_token_base64 =
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";
  UnblindedPaymentTokenInfo unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  get_unblinded_payment_tokens()->RemoveToken(unblinded_payment_token);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, DoNotRemoveTheSameTokenTwice) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  std::string unblinded_payment_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedPaymentTokenInfo& unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  get_unblinded_payment_tokens()->RemoveToken(unblinded_payment_token);
  get_unblinded_payment_tokens()->RemoveToken(unblinded_payment_token);

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveMatchingTokens) {
  // Arrange
  UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::SetUnblindedPaymentTokens(3);
  UnblindedPaymentTokenInfo unblinded_payment_token =
      unblinded_payment_tokens.back();
  unblinded_payment_tokens.pop_back();

  // Act
  get_unblinded_payment_tokens()->RemoveTokens(unblinded_payment_tokens);

  // Assert
  const std::vector<std::string> expected_unblinded_payment_tokens_base64 = {
      "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2"
      "AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa"};
  const UnblindedPaymentTokenList& expected_unblinded_payment_tokens =
      CreateUnblindedPaymentTokens(expected_unblinded_payment_tokens_base64);

  unblinded_payment_tokens = get_unblinded_payment_tokens()->GetAllTokens();

  EXPECT_EQ(expected_unblinded_payment_tokens, unblinded_payment_tokens);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveAllTokens) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(7);

  // Act
  get_unblinded_payment_tokens()->RemoveAllTokens();

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, RemoveAllTokensWithEmptyList) {
  // Arrange

  // Act
  get_unblinded_payment_tokens()->RemoveAllTokens();

  // Assert
  const int count = get_unblinded_payment_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, TokenExists) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  std::string unblinded_payment_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedPaymentTokenInfo& unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  // Assert
  const bool exists =
      get_unblinded_payment_tokens()->TokenExists(unblinded_payment_token);

  EXPECT_TRUE(exists);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, TokenDoesNotExist) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(3);

  // Act
  std::string unblinded_payment_token_base64 =
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";
  UnblindedPaymentTokenInfo unblinded_payment_token =
      CreateUnblindedPaymentToken(unblinded_payment_token_base64);

  // Assert
  const bool exists =
      get_unblinded_payment_tokens()->TokenExists(unblinded_payment_token);

  EXPECT_FALSE(exists);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, Count) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(6);

  // Act
  const int count = get_unblinded_payment_tokens()->Count();

  // Assert
  EXPECT_EQ(6, count);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, IsEmpty) {
  // Arrange

  // Act
  const bool is_empty = get_unblinded_payment_tokens()->IsEmpty();

  // Assert
  EXPECT_TRUE(is_empty);
}

TEST_F(BatAdsUnblindedPaymentTokensTest, IsNotEmpty) {
  // Arrange
  privacy::SetUnblindedPaymentTokens(9);

  // Act
  const bool is_empty = get_unblinded_payment_tokens()->IsEmpty();

  // Assert
  EXPECT_FALSE(is_empty);
}

}  // namespace privacy
}  // namespace ads
