/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>
#include <memory>

#include "confirmations_client_mock.h"
#include "bat-native-confirmations/src/confirmations_impl.h"
#include "bat-native-confirmations/src/security_helper.h"
#include "bat-native-confirmations/src/unblinded_tokens.h"

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using ::testing::AtLeast;

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
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case
  std::vector<UnblindedToken> GetUnblindedTokens(const int count) {
    std::vector<std::string> tokens_base64 = {
      "gXMEnFFPTfgVA3MB11zNRP1ixWjkdw/qsW1RnuQlfkF+ugGxFLafpypS7OJ7mB1zTP775LXrO9vM48fAFNihCOYZS660ClZE/xfDFd930yb12+isTsk6KswtxR10Aogc",  // NOLINT
      "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6",  // NOLINT
      "MNrshKuw6zUTsmlZ+w4WzlJknjV/m/ZYyWUhwSmzyW8Dm/VGpMrifyw5txpNu+SQyNcAR+EJ468ADS5qfNfH7yS0kP9z1OJwMNfLiCTHOCiwd7PJkdv14T/vGS5AT1B5",  // NOLINT
      "MRAbYbmnmjM5bqlbHsX9iuy1Jwc9GCGEA4idBt+PNaQONgbZaPbxHb2pOjw1H6sbgJ2eeIwtobrRRmy+diurWoa0cJ8IG9oy3YtOj8bgc7hy/x5Ixu0kxylNxTKb5b9Z",  // NOLINT
      "aMTJ5HnQot4p6lU5LuXMdYPt3q3Eg1pz5pB2q1c8ys6qVVHd1PyrtEVY+qGJrET3ay2E12Qft0UhNzVUkrgnZ4Kh3mmpcm9wbYnmsid2GK3dBzuHC0ggnYoir1Oo+A8D",  // NOLINT
      "lv6mXcIzMFmBbK37U3SFRxgMiRcM4pGLfrdgp0TCevTJ+XbDlHGNIXxYU8CT8ztGwoJSxYjtBh/MGSpjaklJG37ttqDaMzMT0VhKgEvTHuY7qmyi55WtWVENispKe35M",  // NOLINT
      "f3v9XvsBKp7fdXwQSQHNpHN0MPDzGJ1obhc37pLLyv65/JbdMbsXSQ1dGP0+nD/ETvAFzWzro9s/8HQo0MPLBiKkzvAwnaWyM+TAXG5xwL70iICkNApiv57kUfzvnudp",  // NOLINT
      "uSczWJh99T9QKlsDGoRSBpjoMFf4nQj/A5AW72m9o6akR4BkzQ1M1ATIyZde5O4Q2iSV+KRjGPUheU7QmTQxDS6l79e8a+ro2uXZKbxjY+XAM7PO+iFOOAZuR4IUoJpF",  // NOLINT
      "2W8uYe1n6lFMiQFuD9wHLjr2qYhDB6AM3oXyetnsuR9fOxo8BXu28IzQbkCueWSyBEZ54Xf4AzPyPY2cB73Gh8LuyY4vChgP+E9LwI3yqWyD+RR4O6hCo2e7yKm9dTAm",  // NOLINT
      "tl+V73HJRK2g4TWlqRGxjXeMvhmOvrnLFMfEbUJuiMiByZOUuK4hffoXB5VmbiGLYvJr3shcFpmxMZSuLK3Q97QbP27wmoU+Lk8Jy+MGR+9OTn4MpyvSOfVvDhLypSMG"   // NOLINT
    };

    int modulo = tokens_base64.size();

    std::vector<UnblindedToken> tokens;
    for (int i = 0; i < count; i++) {
      auto token_base64 = tokens_base64.at(i % modulo);
      auto token = UnblindedToken::decode_base64(token_base64);

      tokens.push_back(token);
    }

    return tokens;
  }

  std::vector<UnblindedToken> GetRandomUnblindedTokens(const int count) {
    std::vector<UnblindedToken> unblinded_tokens;

    auto tokens = helper::Security::GenerateTokens(count);
    for (const auto& token : tokens) {
      auto token_base64 = token.encode_base64();
      auto unblinded_token = UnblindedToken::decode_base64(token_base64);

      unblinded_tokens.push_back(unblinded_token);
    }

    return unblinded_tokens;
  }

  base::ListValue GetUnblindedTokensAsList(const int count) {
    base::Value list(base::Value::Type::LIST);

    auto tokens = GetUnblindedTokens(count);
    for (auto& token : tokens) {
      auto token_base64 = token.encode_base64();
      auto token_value = base::Value(token_base64);
      list.GetList().push_back(std::move(token_value));
    }

    base::ListValue list_values(list.GetList());
    return list_values;
  }
};

TEST_F(ConfirmationsUnblindedTokensTest, GetToken) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(10);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto token = unblinded_tokens_->GetToken();
  auto token_base64 = token.encode_base64();

  // Assert
  std::string expected_token_base64 = "gXMEnFFPTfgVA3MB11zNRP1ixWjkdw/qsW1RnuQlfkF+ugGxFLafpypS7OJ7mB1zTP775LXrO9vM48fAFNihCOYZS660ClZE/xfDFd930yb12+isTsk6KswtxR10Aogc";  // NOLINT
  EXPECT_EQ(expected_token_base64, token_base64);
}

TEST_F(ConfirmationsUnblindedTokensTest, GetAllTokens_Exist) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(8);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  auto tokens = unblinded_tokens_->GetAllTokens();

  // Assert
  std::vector<std::string> expected_tokens_base64 = {
    "gXMEnFFPTfgVA3MB11zNRP1ixWjkdw/qsW1RnuQlfkF+ugGxFLafpypS7OJ7mB1zTP775LXrO9vM48fAFNihCOYZS660ClZE/xfDFd930yb12+isTsk6KswtxR10Aogc",  // NOLINT
    "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6",  // NOLINT
    "MNrshKuw6zUTsmlZ+w4WzlJknjV/m/ZYyWUhwSmzyW8Dm/VGpMrifyw5txpNu+SQyNcAR+EJ468ADS5qfNfH7yS0kP9z1OJwMNfLiCTHOCiwd7PJkdv14T/vGS5AT1B5",  // NOLINT
    "MRAbYbmnmjM5bqlbHsX9iuy1Jwc9GCGEA4idBt+PNaQONgbZaPbxHb2pOjw1H6sbgJ2eeIwtobrRRmy+diurWoa0cJ8IG9oy3YtOj8bgc7hy/x5Ixu0kxylNxTKb5b9Z",  // NOLINT
    "aMTJ5HnQot4p6lU5LuXMdYPt3q3Eg1pz5pB2q1c8ys6qVVHd1PyrtEVY+qGJrET3ay2E12Qft0UhNzVUkrgnZ4Kh3mmpcm9wbYnmsid2GK3dBzuHC0ggnYoir1Oo+A8D",  // NOLINT
    "lv6mXcIzMFmBbK37U3SFRxgMiRcM4pGLfrdgp0TCevTJ+XbDlHGNIXxYU8CT8ztGwoJSxYjtBh/MGSpjaklJG37ttqDaMzMT0VhKgEvTHuY7qmyi55WtWVENispKe35M",  // NOLINT
    "f3v9XvsBKp7fdXwQSQHNpHN0MPDzGJ1obhc37pLLyv65/JbdMbsXSQ1dGP0+nD/ETvAFzWzro9s/8HQo0MPLBiKkzvAwnaWyM+TAXG5xwL70iICkNApiv57kUfzvnudp",  // NOLINT
    "uSczWJh99T9QKlsDGoRSBpjoMFf4nQj/A5AW72m9o6akR4BkzQ1M1ATIyZde5O4Q2iSV+KRjGPUheU7QmTQxDS6l79e8a+ro2uXZKbxjY+XAM7PO+iFOOAZuR4IUoJpF"   // NOLINT
  };

  unsigned int index = 0;
  for (auto& token : tokens) {
    auto expected_token_base64 = expected_tokens_base64.at(index);
    auto expected_token = UnblindedToken::decode_base64(expected_token_base64);
    if (token != expected_token) {
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
  for (auto& value : list_values) {
    auto token_base64 = value.GetString();
    auto token = UnblindedToken::decode_base64(token_base64);

    if (!unblinded_tokens_->TokenExists(token)) {
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(1);

  auto unblinded_tokens = GetUnblindedTokens(10);

  // Act
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Assert
  unsigned int index = 0;
  auto tokens = unblinded_tokens_->GetAllTokens();
  for (auto& token : tokens) {
    auto unblinded_token = unblinded_tokens.at(index);
    if (token != unblinded_token) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokens_Count) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(1);

  auto list = GetUnblindedTokensAsList(5);

  // Act
  unblinded_tokens_->SetTokensFromList(list);

  // Assert
  std::vector<std::string> expected_tokens_base64 = {
    "gXMEnFFPTfgVA3MB11zNRP1ixWjkdw/qsW1RnuQlfkF+ugGxFLafpypS7OJ7mB1zTP775LXrO9vM48fAFNihCOYZS660ClZE/xfDFd930yb12+isTsk6KswtxR10Aogc",  // NOLINT
    "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6",  // NOLINT
    "MNrshKuw6zUTsmlZ+w4WzlJknjV/m/ZYyWUhwSmzyW8Dm/VGpMrifyw5txpNu+SQyNcAR+EJ468ADS5qfNfH7yS0kP9z1OJwMNfLiCTHOCiwd7PJkdv14T/vGS5AT1B5",  // NOLINT
    "MRAbYbmnmjM5bqlbHsX9iuy1Jwc9GCGEA4idBt+PNaQONgbZaPbxHb2pOjw1H6sbgJ2eeIwtobrRRmy+diurWoa0cJ8IG9oy3YtOj8bgc7hy/x5Ixu0kxylNxTKb5b9Z",  // NOLINT
    "aMTJ5HnQot4p6lU5LuXMdYPt3q3Eg1pz5pB2q1c8ys6qVVHd1PyrtEVY+qGJrET3ay2E12Qft0UhNzVUkrgnZ4Kh3mmpcm9wbYnmsid2GK3dBzuHC0ggnYoir1Oo+A8D"   // NOLINT
  };

  auto tokens = unblinded_tokens_->GetAllTokens();

  unsigned int index = 0;
  for (auto& token : tokens) {
    auto expected_token_base64 = expected_tokens_base64.at(index);
    auto expected_token = UnblindedToken::decode_base64(expected_token_base64);
    if (token != expected_token) {
      FAIL();
    }

    index++;
  }

  SUCCEED();
}

TEST_F(ConfirmationsUnblindedTokensTest, SetTokensFromList_EmptyList) {
  // Arrange
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(1);

  auto tokens = GetRandomUnblindedTokens(5);
  unblinded_tokens_->AddTokens(tokens);

  // Assert
  for (auto& token : tokens) {
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(1);

  std::string token_base64 = "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6";  // NOLINT
  auto token = UnblindedToken::decode_base64(token_base64);
  unblinded_tokens_->RemoveToken(token);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(2, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_Removed) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(1);

  std::string token_base64 = "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6";  // NOLINT
  auto token = UnblindedToken::decode_base64(token_base64);
  unblinded_tokens_->RemoveToken(token);

  // Assert
  EXPECT_FALSE(unblinded_tokens_->TokenExists(token));
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_UnknownToken) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(0);

  std::string token_base64 = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";  // NOLINT
  auto token = UnblindedToken::decode_base64(token_base64);
  unblinded_tokens_->RemoveToken(token);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(3, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveToken_SameTokenTwice) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
      .Times(1);

  std::string token_base64 = "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6";  // NOLINT
  auto token = UnblindedToken::decode_base64(token_base64);

  unblinded_tokens_->RemoveToken(token);
  unblinded_tokens_->RemoveToken(token);

  // Assert
  auto count = unblinded_tokens_->Count();
  EXPECT_EQ(2, count);
}

TEST_F(ConfirmationsUnblindedTokensTest, RemoveAllTokens) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(7);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  EXPECT_CALL(*mock_confirmations_client_, SaveState)
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
  std::string token_base64 = "nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yYBCAPsup2JFSbp5iYUaeWiCxF6w8I1MKrjPj6trywQ6";  // NOLINT
  auto token = UnblindedToken::decode_base64(token_base64);
  auto exists = unblinded_tokens_->TokenExists(token);

  // Assert
  EXPECT_TRUE(exists);
}

TEST_F(ConfirmationsUnblindedTokensTest, TokenExists_UnknownToken) {
  // Arrange
  auto unblinded_tokens = GetUnblindedTokens(3);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  // Act
  std::string token_base64 = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";  // NOLINT
  auto token = UnblindedToken::decode_base64(token_base64);
  auto exists = unblinded_tokens_->TokenExists(token);

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
