/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {

class BatAdsUnblindedTokensTest : public UnitTestBase {
 protected:
  BatAdsUnblindedTokensTest() = default;

  ~BatAdsUnblindedTokensTest() override = default;
};

TEST_F(BatAdsUnblindedTokensTest, GetToken) {
  // Arrange
  privacy::SetUnblindedTokens(10);

  // Act
  const UnblindedTokenInfo& unblinded_token =
      get_unblinded_tokens()->GetToken();

  // Assert
  const std::string expected_unblinded_token_base64 =
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";
  UnblindedTokenInfo expected_unblinded_token =
      CreateUnblindedToken(expected_unblinded_token_base64);

  EXPECT_EQ(expected_unblinded_token, unblinded_token);
}

TEST_F(BatAdsUnblindedTokensTest, GetAllTokens) {
  // Arrange
  privacy::SetUnblindedTokens(8);

  // Act
  const UnblindedTokenList& unblinded_tokens =
      get_unblinded_tokens()->GetAllTokens();

  // Assert
  const std::vector<std::string> expected_unblinded_tokens_base64 = {
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
  UnblindedTokenList expected_unblinded_tokens =
      CreateUnblindedTokens(expected_unblinded_tokens_base64);

  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokensTest, GetTokensAsList) {
  // Arrange
  privacy::SetUnblindedTokens(8);

  // Act
  const base::Value& list = get_unblinded_tokens()->GetTokensAsList();

  // Assert
  const base::Value::List& list_values = list.GetList();

  const UnblindedTokenList& unblinded_tokens =
      get_unblinded_tokens()->GetAllTokens();
  EXPECT_EQ(list_values.size(), unblinded_tokens.size());

  for (auto& value : list_values) {
    const base::Value::Dict* dictionary = value.GetIfDict();
    if (!dictionary) {
      FAIL();
    }

    const std::string* unblinded_token_value =
        dictionary->FindString("unblinded_token");
    if (!unblinded_token_value) {
      FAIL();
    }
    const std::string unblinded_token_base64 = *unblinded_token_value;

    const std::string* public_key_value = dictionary->FindString("public_key");
    if (!public_key_value) {
      FAIL();
    }
    const std::string public_key_base64 = *public_key_value;

    UnblindedTokenInfo unblinded_token;
    unblinded_token.value =
        UnblindedToken::decode_base64(unblinded_token_base64);
    unblinded_token.public_key = PublicKey::decode_base64(public_key_base64);

    if (!get_unblinded_tokens()->TokenExists(unblinded_token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatAdsUnblindedTokensTest, GetTokensAsListWithEmptyList) {
  // Arrange

  // Act
  const base::Value& list = get_unblinded_tokens()->GetTokensAsList();

  // Assert
  EXPECT_TRUE(list.GetList().empty());
}

TEST_F(BatAdsUnblindedTokensTest, SetTokens) {
  // Arrange
  const UnblindedTokenList& unblinded_tokens = GetUnblindedTokens(10);

  // Act
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Assert
  const UnblindedTokenList& expected_unblinded_tokens =
      get_unblinded_tokens()->GetAllTokens();

  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokensTest, SetTokensWithEmptyList) {
  // Arrange
  const UnblindedTokenList& unblinded_tokens = {};

  // Act
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest, SetTokensFromList) {
  // Arrange
  const base::Value& list = GetUnblindedTokensAsList(5);

  // Act
  get_unblinded_tokens()->SetTokensFromList(list);

  // Assert
  const UnblindedTokenList& unblinded_tokens =
      get_unblinded_tokens()->GetAllTokens();

  const std::vector<std::string> expected_unblinded_tokens_base64 = {
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

  UnblindedTokenList expected_unblinded_tokens;
  for (const auto& unblinded_token_base64 : expected_unblinded_tokens_base64) {
    UnblindedTokenInfo unblinded_token;
    unblinded_token.value =
        UnblindedToken::decode_base64(unblinded_token_base64);
    unblinded_token.public_key = PublicKey::decode_base64(
        "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");

    expected_unblinded_tokens.push_back(unblinded_token);
  }

  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokensTest, SetTokensFromListWithEmptyList) {
  // Arrange
  const base::Value& list = GetUnblindedTokensAsList(0);

  // Act
  get_unblinded_tokens()->SetTokensFromList(list);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest, AddTokens) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  const UnblindedTokenList& unblinded_tokens = GetRandomUnblindedTokens(5);
  get_unblinded_tokens()->AddTokens(unblinded_tokens);

  // Assert
  for (const auto& unblinded_token : unblinded_tokens) {
    if (!get_unblinded_tokens()->TokenExists(unblinded_token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatAdsUnblindedTokensTest, DoNotAddDuplicateTokens) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  const UnblindedTokenList& duplicate_unblinded_tokens = GetUnblindedTokens(1);
  get_unblinded_tokens()->AddTokens(duplicate_unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedTokensTest, AddTokensCount) {
  // Arrange
  privacy::SetUnblindedTokens(5);

  // Act
  const UnblindedTokenList& random_unblinded_tokens =
      GetRandomUnblindedTokens(3);
  get_unblinded_tokens()->AddTokens(random_unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(8, count);
}

TEST_F(BatAdsUnblindedTokensTest, AddTokensWithEmptyList) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  const UnblindedTokenList& empty_unblinded_tokens = {};
  get_unblinded_tokens()->AddTokens(empty_unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedTokensTest, RemoveTokenCount) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  const std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo& unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatAdsUnblindedTokensTest, RemoveToken) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo& unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  EXPECT_FALSE(get_unblinded_tokens()->TokenExists(unblinded_token));
}

TEST_F(BatAdsUnblindedTokensTest, DoNotRemoveTokensThatDoNotExist) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  std::string unblinded_token_base64 =
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";
  UnblindedTokenInfo unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedTokensTest, DoNotRemoveTheSameTokenTwice) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo& unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);
  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatAdsUnblindedTokensTest, RemoveMatchingTokens) {
  // Arrange
  UnblindedTokenList unblinded_tokens = privacy::SetUnblindedTokens(3);
  UnblindedTokenInfo unblinded_token = unblinded_tokens.back();
  unblinded_tokens.pop_back();

  // Act
  get_unblinded_tokens()->RemoveTokens(unblinded_tokens);

  // Assert
  const std::vector<std::string> expected_unblinded_tokens_base64 = {
      "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2"
      "AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa"};
  const UnblindedTokenList& expected_unblinded_tokens =
      CreateUnblindedTokens(expected_unblinded_tokens_base64);

  unblinded_tokens = get_unblinded_tokens()->GetAllTokens();

  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokensTest, RemoveAllTokens) {
  // Arrange
  privacy::SetUnblindedTokens(7);

  // Act
  get_unblinded_tokens()->RemoveAllTokens();

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest, RemoveAllTokensWithEmptyList) {
  // Arrange

  // Act
  get_unblinded_tokens()->RemoveAllTokens();

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest, TokenExists) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo& unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  // Assert
  const bool exists = get_unblinded_tokens()->TokenExists(unblinded_token);

  EXPECT_TRUE(exists);
}

TEST_F(BatAdsUnblindedTokensTest, TokenDoesNotExist) {
  // Arrange
  privacy::SetUnblindedTokens(3);

  // Act
  std::string unblinded_token_base64 =
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";
  UnblindedTokenInfo unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  // Assert
  const bool exists = get_unblinded_tokens()->TokenExists(unblinded_token);

  EXPECT_FALSE(exists);
}

TEST_F(BatAdsUnblindedTokensTest, Count) {
  // Arrange
  privacy::SetUnblindedTokens(6);

  // Act
  const int count = get_unblinded_tokens()->Count();

  // Assert
  EXPECT_EQ(6, count);
}

TEST_F(BatAdsUnblindedTokensTest, IsEmpty) {
  // Arrange

  // Act
  const bool is_empty = get_unblinded_tokens()->IsEmpty();

  // Assert
  EXPECT_TRUE(is_empty);
}

TEST_F(BatAdsUnblindedTokensTest, IsNotEmpty) {
  // Arrange
  privacy::SetUnblindedTokens(9);

  // Act
  const bool is_empty = get_unblinded_tokens()->IsEmpty();

  // Assert
  EXPECT_FALSE(is_empty);
}

}  // namespace privacy
}  // namespace ads
