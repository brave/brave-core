/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/unblinded_tokens.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/privacy_utils.h"
#include "bat/confirmations/internal/unittest_utils.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

using ::testing::_;
using ::testing::NiceMock;

namespace confirmations {

class BatConfirmationsUnblindedTokensTest : public ::testing::Test {
 protected:
  BatConfirmationsUnblindedTokensTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<ConfirmationsClientMock>>()),
        confirmations_(std::make_unique<ConfirmationsImpl>(
            confirmations_client_mock_.get())),
        unblinded_tokens_(std::make_unique<UnblindedTokens>(
            confirmations_.get())) {
    // You can do set-up work for each test here
  }

  ~BatConfirmationsUnblindedTokensTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    MockLoadState(confirmations_client_mock_);
    MockSaveState(confirmations_client_mock_);

    Initialize(confirmations_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  TokenInfo CreateToken(
      const std::string& token_base64) {
    TokenInfo token;
    token.unblinded_token = UnblindedToken::decode_base64(token_base64);
    token.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

    return token;
  }

  TokenList GetUnblindedTokens(
      const int count) {
    const std::vector<std::string> tokens_base64 = {
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
      "4+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z",
      "6tKJHOtQqpNzFjLGT0gvXlCF0GGKrqQlK82e2tc7gJvQkorg60Y21jEAg8JHbU8D"
      "3mBK/riZCILoi1cPCiBDAdhWJNVm003mZ0ShjmbESnKhL/NxRv/0/PB3GQ5iydoc",
      "ujGlRHnz+UF0h8i6gYDnfeZDUj7qZZz6o29ZJFa3XN2g+yVXgRTws1yv6RAtLCr3"
      "9OQso6FAT12o8GAvHVEzmRqyzm2XU9gMK5WrNtT/fhr8gQ9RvupdznGKOqmVbuIc"
    };

    const int modulo = tokens_base64.size();

    TokenList unblinded_tokens;
    for (int i = 0; i < count; i++) {
      const std::string token_base64 = tokens_base64.at(i % modulo);
      const TokenInfo token = CreateToken(token_base64);

      unblinded_tokens.push_back(token);
    }

    return unblinded_tokens;
  }

  TokenList GetRandomUnblindedTokens(
      const int count) {
    TokenList unblinded_tokens;

    const std::vector<Token> tokens = privacy::GenerateTokens(count);
    for (const auto& token : tokens) {
      const std::string token_base64 = token.encode_base64();
      const TokenInfo unblinded_token = CreateToken(token_base64);

      unblinded_tokens.push_back(unblinded_token);
    }

    return unblinded_tokens;
  }

  base::Value GetUnblindedTokensAsList(
      const int count) {
    base::Value list(base::Value::Type::LIST);

    const TokenList tokens = GetUnblindedTokens(count);

    for (const auto& token : tokens) {
      base::Value dictionary(base::Value::Type::DICTIONARY);
      dictionary.SetKey("unblinded_token", base::Value(
          token.unblinded_token.encode_base64()));
      dictionary.SetKey("public_key", base::Value(token.public_key));

      list.Append(std::move(dictionary));
    }

    return list;
  }

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<ConfirmationsClientMock> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<UnblindedTokens> unblinded_tokens_;
};

TEST_F(BatConfirmationsUnblindedTokensTest,
    GetToken) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(10);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const TokenInfo token = unblinded_tokens_->GetToken();

  // Assert
  const std::string expected_unblinded_token_base64 =
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";

  TokenInfo expected_token;
  expected_token.unblinded_token =
      UnblindedToken::decode_base64(expected_unblinded_token_base64);
  expected_token.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  EXPECT_EQ(expected_token, token);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    GetAllTokens) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(8);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const TokenList tokens = unblinded_tokens_->GetAllTokens();

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
    "4+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z"
  };

  const std::string expected_public_key =
      "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  EXPECT_EQ(tokens.size(), expected_unblinded_tokens_base64.size());

  unsigned int index = 0;
  for (const auto& token_info : tokens) {
    const std::string expected_unblinded_token_base64 =
        expected_unblinded_tokens_base64.at(index);

    const UnblindedToken expected_unblinded_token =
        UnblindedToken::decode_base64(expected_unblinded_token_base64);

    if (token_info.unblinded_token != expected_unblinded_token ||
        token_info.public_key != expected_public_key) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    GetTokensAsList) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(8);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const base::Value list = unblinded_tokens_->GetTokensAsList();

  // Assert
  base::ListValue list_values(list.GetList());

  EXPECT_EQ(list_values.GetSize(), unblinded_tokens.size());

  for (auto& value : list_values) {
    base::DictionaryValue* dictionary;
    if (!value.GetAsDictionary(&dictionary)) {
      FAIL();
    }

    const std::string* unblinded_token_value =
        dictionary->FindStringKey("unblinded_token");
    if (!unblinded_token_value) {
      FAIL();
    }
    const std::string unblinded_token_base64 = *unblinded_token_value;
    const UnblindedToken unblinded_token =
        UnblindedToken::decode_base64(unblinded_token_base64);

    const std::string* public_key_value =
        dictionary->FindStringKey("public_key");
    if (!public_key_value) {
      FAIL();
    }
    const std::string public_key = *public_key_value;

    TokenInfo token;
    token.unblinded_token = unblinded_token;
    token.public_key = public_key;

    if (!unblinded_tokens_->TokenExists(token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    GetTokensAsListWithEmptyList) {
  // Arrange
  const TokenList unblinded_tokens = {};
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const base::Value list = unblinded_tokens_->GetTokensAsList();

  // Assert
  EXPECT_TRUE(list.GetList().empty());
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    SetTokens) {
  // Arrange
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const TokenList unblinded_tokens = GetUnblindedTokens(10);

  // Act
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Assert
  unsigned int index = 0;
  const TokenList tokens = unblinded_tokens_->GetAllTokens();

  EXPECT_EQ(tokens.size(), unblinded_tokens.size());

  for (const auto& token : tokens) {
    const TokenInfo expected_token = unblinded_tokens.at(index);
    if (token.unblinded_token != expected_token.unblinded_token) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    SetTokensWithEmptyList) {
  // Arrange
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const TokenList unblinded_tokens = {};

  // Act
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    SetTokensFromList) {
  // Arrange
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const base::Value list = GetUnblindedTokensAsList(5);

  // Act
  unblinded_tokens_->SetTokensFromList(list);

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
    "T3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi"
  };

  const TokenList tokens = unblinded_tokens_->GetAllTokens();

  EXPECT_EQ(tokens.size(), expected_unblinded_tokens_base64.size());

  unsigned int index = 0;
  for (const auto& token_info : tokens) {
    const std::string expected_unblinded_token_base64 =
        expected_unblinded_tokens_base64.at(index);

    const UnblindedToken expected_unblinded_token =
        UnblindedToken::decode_base64(expected_unblinded_token_base64);

    if (token_info.unblinded_token != expected_unblinded_token) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    SetTokensFromListWithEmptyList) {
  // Arrange
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const base::Value list = GetUnblindedTokensAsList(0);

  // Act
  unblinded_tokens_->SetTokensFromList(list);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    AddTokens) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const TokenList tokens = GetRandomUnblindedTokens(5);
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  for (const auto& token : tokens) {
    if (!unblinded_tokens_->TokenExists(token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    DoNotAddDuplicateTokens) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const TokenList duplicate_unblinded_tokens = GetUnblindedTokens(1);
  unblinded_tokens_->AddTokens(duplicate_unblinded_tokens);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    AddTokensCount) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(5);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const TokenList tokens = GetRandomUnblindedTokens(3);
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(8, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
     AddTokensWithEmptyList) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const TokenList tokens = {};
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    RemoveTokenCount) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  const std::string token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const TokenInfo token = CreateToken(token_base64);

  unblinded_tokens_->RemoveToken(token);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    RemoveToken) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  std::string token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const TokenInfo token = CreateToken(token_base64);

  unblinded_tokens_->RemoveToken(token);

  // Assert
  EXPECT_FALSE(unblinded_tokens_->TokenExists(token));
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    DoNotRemoveTokensThatDoNotExist) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(0);

  std::string token_base64 =
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";

  TokenInfo token;
  token.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token.public_key = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEAD";

  unblinded_tokens_->RemoveToken(token);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    DoNotRemoveTheSameTokenTwice) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  std::string token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const TokenInfo token = CreateToken(token_base64);

  unblinded_tokens_->RemoveToken(token);
  unblinded_tokens_->RemoveToken(token);

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    RemoveAllTokens) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(7);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  unblinded_tokens_->RemoveAllTokens();

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    RemoveAllTokensWithEmptyList) {
  // Arrange
  const TokenList unblinded_tokens = {};
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*confirmations_client_mock_, SaveState(_, _, _))
      .Times(1);

  unblinded_tokens_->RemoveAllTokens();

  // Assert
  const int count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    TokenExists) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  std::string token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const TokenInfo token = CreateToken(token_base64);

  const bool exists = unblinded_tokens_->TokenExists(token);

  // Assert
  EXPECT_TRUE(exists);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    TokenDoesNotExist) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  std::string token_base64 =
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF"
      "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";

  TokenInfo token;
  token.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token.public_key = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEAD";

  const bool exists = unblinded_tokens_->TokenExists(token);

  // Assert
  EXPECT_FALSE(exists);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    Count) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(6);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const int count = unblinded_tokens_->Count();

  // Assert
  EXPECT_EQ(6, count);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    IsEmpty) {
  // Arrange
  const TokenList unblinded_tokens = {};
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const bool is_empty = unblinded_tokens_->IsEmpty();

  // Assert
  EXPECT_TRUE(is_empty);
}

TEST_F(BatConfirmationsUnblindedTokensTest,
    IsNotEmpty) {
  // Arrange
  const TokenList unblinded_tokens = GetUnblindedTokens(9);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  const bool is_empty = unblinded_tokens_->IsEmpty();

  // Assert
  EXPECT_FALSE(is_empty);
}

}  // namespace confirmations
