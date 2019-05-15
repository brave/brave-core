/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/security_helper.h"
#include "bat/confirmations/internal/unblinded_tokens.h"

#include "base/files/file_path.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using ::testing::_;
using ::testing::Invoke;

namespace confirmations {

class ConfirmationsUnblindedTokensTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<UnblindedTokens> unblinded_tokens_;

  ConfirmationsUnblindedTokensTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      unblinded_tokens_(std::make_unique<UnblindedTokens>(
          confirmations_.get())) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsUnblindedTokensTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)
    EXPECT_CALL(*mock_confirmations_client_, LoadState(_, _))
        .WillRepeatedly(
            Invoke([this](
                const std::string& name,
                OnLoadCallback callback) {
              auto path = GetTestDataPath();
              path = path.AppendASCII(name);

              std::string value;
              if (!Load(path, &value)) {
                callback(FAILED, value);
                return;
              }

              callback(SUCCESS, value);
            }));

    ON_CALL(*mock_confirmations_client_, SaveState(_, _, _))
        .WillByDefault(
            Invoke([](
                const std::string& name,
                const std::string& value,
                OnSaveCallback callback) {
              callback(SUCCESS);
            }));

    confirmations_->Initialize();
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  base::FilePath GetTestDataPath() {
    return base::FilePath(FILE_PATH_LITERAL(
        "brave/vendor/bat-native-confirmations/test/data"));
  }

  bool Load(const base::FilePath path, std::string* value) {
    if (!value) {
      return false;
    }

    std::ifstream ifs{path.value().c_str()};
    if (ifs.fail()) {
      *value = "";
      return false;
    }

    std::stringstream stream;
    stream << ifs.rdbuf();
    *value = stream.str();
    return true;
  }

  std::vector<TokenInfo> GetUnblindedTokens(const int count) {
    std::vector<std::string> tokens_base64 = {
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY",  // NOLINT
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K",  // NOLINT
      "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa",  // NOLINT
      "OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU",  // NOLINT
      "Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi",  // NOLINT
      "+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvVZGYXLeLn2HSAM7cDgqyW6SEuPzlDeZT6kkTNI7JcQm",  // NOLINT
      "CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkj5pW/rg7nl48EX6nmekgd3D2Hz8JgJnSarzP/8+3l+MW",  // NOLINT
      "hQ+6+jh5DUUBFhhGn7bPLDjqrUIKNi/T8QDt1x01bcW9PLADg6aS73dzrVBsHav44+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z",  // NOLINT
      "6tKJHOtQqpNzFjLGT0gvXlCF0GGKrqQlK82e2tc7gJvQkorg60Y21jEAg8JHbU8D3mBK/riZCILoi1cPCiBDAdhWJNVm003mZ0ShjmbESnKhL/NxRv/0/PB3GQ5iydoc",  // NOLINT
      "ujGlRHnz+UF0h8i6gYDnfeZDUj7qZZz6o29ZJFa3XN2g+yVXgRTws1yv6RAtLCr39OQso6FAT12o8GAvHVEzmRqyzm2XU9gMK5WrNtT/fhr8gQ9RvupdznGKOqmVbuIc"   // NOLINT
    };

    int modulo = tokens_base64.size();

    std::vector<TokenInfo> unblinded_tokens;
    for (int i = 0; i < count; i++) {
      TokenInfo token_info;
      auto token_base64 = tokens_base64.at(i % modulo);
      token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
      token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

      unblinded_tokens.push_back(token_info);
    }

    return unblinded_tokens;
  }

  std::vector<TokenInfo> GetRandomUnblindedTokens(const int count) {
    std::vector<TokenInfo> unblinded_tokens;

    auto tokens = helper::Security::GenerateTokens(count);
    for (const auto& token : tokens) {
      TokenInfo token_info;
      auto token_base64 = token.encode_base64();
      token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
      token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

      unblinded_tokens.push_back(token_info);
    }

    return unblinded_tokens;
  }

  base::Value GetUnblindedTokensAsList(const int count) {
    base::Value list(base::Value::Type::LIST);

    auto tokens = GetUnblindedTokens(count);

    for (const auto& token : tokens) {
      base::Value dictionary(base::Value::Type::DICTIONARY);
      dictionary.SetKey("unblinded_token", base::Value(
          token.unblinded_token.encode_base64()));
      dictionary.SetKey("public_key", base::Value(token.public_key));

      list.GetList().push_back(std::move(dictionary));
    }

    return list;
  }
};

TEST_F(ConfirmationsUnblindedTokensTest, GetToken) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(10);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto token_info = unblinded_tokens_->GetToken();
  auto token_base64 = token_info.unblinded_token.encode_base64();
  auto public_key = token_info.public_key;

  // Assert
  std::string expected_token_base64 = "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";  // NOLINT
  EXPECT_EQ(expected_token_base64, token_base64);

  std::string expected_public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";  // NOLINT
  EXPECT_EQ(expected_public_key, public_key);
}

TEST_F(ConfirmationsUnblindedTokensTest, GetAllTokens_Exist) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(8);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto tokens = unblinded_tokens_->GetAllTokens();

  // Assert
  std::vector<std::string> expected_unblinded_tokens_base64 = {
    "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY",  // NOLINT
    "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K",  // NOLINT
    "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa",  // NOLINT
    "OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU",  // NOLINT
    "Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi",  // NOLINT
    "+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvVZGYXLeLn2HSAM7cDgqyW6SEuPzlDeZT6kkTNI7JcQm",  // NOLINT
    "CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkj5pW/rg7nl48EX6nmekgd3D2Hz8JgJnSarzP/8+3l+MW",  // NOLINT
    "hQ+6+jh5DUUBFhhGn7bPLDjqrUIKNi/T8QDt1x01bcW9PLADg6aS73dzrVBsHav44+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z"   // NOLINT
  };

  std::string expected_public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";  // NOLINT

  EXPECT_EQ(tokens.size(), expected_unblinded_tokens_base64.size());

  unsigned int index = 0;
  for (const auto& token_info : tokens) {
    auto expected_unblinded_token_base64 =
        expected_unblinded_tokens_base64.at(index);

    auto expected_unblinded_token =
        UnblindedToken::decode_base64(expected_unblinded_token_base64);

    if (token_info.unblinded_token != expected_unblinded_token ||
        token_info.public_key != expected_public_key) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, GetAllTokens_Count) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(8);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto tokens = unblinded_tokens_->GetAllTokens();

  // Assert
  auto count = tokens.size();
  EXPECT_EQ(8UL, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, GetTokensAsList_Exist) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(8);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto list = unblinded_tokens_->GetTokensAsList();

  // Assert
  base::ListValue list_values(list.GetList());
  EXPECT_EQ(list_values.GetSize(), unblinded_tokens.size());
  for (auto& value : list_values) {
    base::DictionaryValue* dictionary;
    if (!value.GetAsDictionary(&dictionary)) {
      FAIL();
    }

    // Unblinded token
    auto* unblinded_token_value = dictionary->FindKey("unblinded_token");
    if (!unblinded_token_value) {
      FAIL();
    }
    auto unblinded_token_base64 = unblinded_token_value->GetString();
    auto unblinded_token =
        UnblindedToken::decode_base64(unblinded_token_base64);

    // Public key
    auto* public_key_value = dictionary->FindKey("public_key");
    if (!public_key_value) {
      FAIL();
    }
    auto public_key = public_key_value->GetString();

    TokenInfo token_info;
    token_info.unblinded_token = unblinded_token;
    token_info.public_key = public_key;

    if (!unblinded_tokens_->TokenExists(token_info)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, GetTokensAsList_Count) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(11);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto list = unblinded_tokens_->GetTokensAsList();

  // Assert
  auto count = list.GetList().size();
  EXPECT_EQ(11UL, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, GetTokensAsList_EmptyList) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(0);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto list = unblinded_tokens_->GetTokensAsList();

  // Assert
  auto count = list.GetList().size();
  EXPECT_EQ(0UL, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokens_Exist) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto unblinded_tokens = GetUnblindedTokens(10);

  // Act
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Assert
  unsigned int index = 0;
  auto tokens = unblinded_tokens_->GetAllTokens();
  EXPECT_EQ(tokens.size(), unblinded_tokens.size());
  for (const auto& token_info : tokens) {
    auto expected_token_info = unblinded_tokens.at(index);
    if (token_info.unblinded_token != expected_token_info.unblinded_token) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokens_Count) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto unblinded_tokens = GetUnblindedTokens(4);

  // Act
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(4, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokens_NoTokens) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto unblinded_tokens = GetUnblindedTokens(0);

  // Act
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokensFromList) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto list = GetUnblindedTokensAsList(5);

  // Act
  unblinded_tokens_->SetTokensFromList(list);

  // Assert
  std::vector<std::string> expected_unblinded_tokens_base64 = {
    "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY",  // NOLINT
    "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K",  // NOLINT
    "bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa",  // NOLINT
    "OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU",  // NOLINT
    "Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi"   // NOLINT
  };

  auto tokens = unblinded_tokens_->GetAllTokens();

  EXPECT_EQ(tokens.size(), expected_unblinded_tokens_base64.size());

  unsigned int index = 0;
  for (const auto& token_info : tokens) {
    auto expected_unblinded_token_base64 =
        expected_unblinded_tokens_base64.at(index);

    auto expected_unblinded_token =
        UnblindedToken::decode_base64(expected_unblinded_token_base64);

    if (token_info.unblinded_token != expected_unblinded_token) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokensFromList_EmptyList) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto list = GetUnblindedTokensAsList(0);

  // Act
  unblinded_tokens_->SetTokensFromList(list);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, AddTokens_Added) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto tokens = GetRandomUnblindedTokens(5);
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  for (const auto& token : tokens) {
    if (!unblinded_tokens_->TokenExists(token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, AddTokens_ShouldNotAddDuplicates) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto duplicate_unblinded_tokens = GetUnblindedTokens(1);
  unblinded_tokens_->AddTokens(duplicate_unblinded_tokens);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, AddTokens_Count) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(5);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto tokens = GetRandomUnblindedTokens(3);
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(8, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, AddTokens_NoTokens) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  auto tokens = GetUnblindedTokens(0);
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_Count) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  std::string token_base64 = "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  unblinded_tokens_->RemoveToken(token_info);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(2, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_Removed) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  std::string token_base64 = "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  unblinded_tokens_->RemoveToken(token_info);

  // Assert
  EXPECT_FALSE(unblinded_tokens_->TokenExists(token_info));
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_UnknownToken) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(0);

  std::string token_base64 = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token_info.public_key = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEAD";

  unblinded_tokens_->RemoveToken(token_info);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_SameTokenTwice) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  std::string token_base64 = "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  unblinded_tokens_->RemoveToken(token_info);
  unblinded_tokens_->RemoveToken(token_info);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(2, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveAllTokens) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(7);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  unblinded_tokens_->RemoveAllTokens();

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveAllTokens_NoTokens) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(0);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState(_, _, _))
      .Times(1);

  unblinded_tokens_->RemoveAllTokens();

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(0, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, TokenExists) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  std::string token_base64 = "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

  auto exists = unblinded_tokens_->TokenExists(token_info);

  // Assert
  EXPECT_TRUE(exists);
}

TEST_F(ConfirmationsUnblindedTokensTest, TokenExists_UnknownToken) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  std::string token_base64 = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";  // NOLINT

  TokenInfo token_info;
  token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
  token_info.public_key = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEAD";

  auto exists = unblinded_tokens_->TokenExists(token_info);

  // Assert
  EXPECT_FALSE(exists);
}

TEST_F(ConfirmationsUnblindedTokensTest, Count) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(6);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto count = unblinded_tokens_->Count();

  // Assert
  EXPECT_EQ(6, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, IsEmpty) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(0);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto empty = unblinded_tokens_->IsEmpty();

  // Assert
  EXPECT_TRUE(empty);
}

TEST_F(ConfirmationsUnblindedTokensTest, IsNotEmpty) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(9);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto empty = unblinded_tokens_->IsEmpty();

  // Assert
  EXPECT_FALSE(empty);
}

}  // namespace confirmations
