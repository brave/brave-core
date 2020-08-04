/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_tokens.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "base/files/scoped_temp_dir.h"
#include "brave/components/l10n/browser/locale_helper_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "wrapper.hpp"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/platform/platform_helper_mock.h"
#include "bat/ads/internal/privacy/privacy_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace ads {
namespace privacy {

using challenge_bypass_ristretto::UnblindedToken;
using challenge_bypass_ristretto::PublicKey;

class BatAdsUnblindedTokensTest : public ::testing::Test {
 protected:
  BatAdsUnblindedTokensTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        ads_client_mock_(std::make_unique<NiceMock<AdsClientMock>>()),
        ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
        locale_helper_mock_(std::make_unique<
            NiceMock<brave_l10n::LocaleHelperMock>>()),
        platform_helper_mock_(std::make_unique<
            NiceMock<PlatformHelperMock>>()) {
    // You can do set-up work for each test here

    brave_l10n::LocaleHelper::GetInstance()->set_for_testing(
        locale_helper_mock_.get());

    PlatformHelper::GetInstance()->set_for_testing(platform_helper_mock_.get());
  }

  ~BatAdsUnblindedTokensTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath path = temp_dir_.GetPath();

    ON_CALL(*ads_client_mock_, IsEnabled())
        .WillByDefault(Return(true));

    ON_CALL(*ads_client_mock_, ShouldAllowAdConversionTracking())
        .WillByDefault(Return(true));

    SetBuildChannel(false, "test");

    ON_CALL(*locale_helper_mock_, GetLocale())
        .WillByDefault(Return("en-US"));

    MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

    ads_->OnWalletUpdated("c387c2d8-a26d-4451-83e4-5c0c6fd942be",
        "5BEKM1Y7xcRSg/1q8in/+Lki2weFZQB+UMYZlRw8ql8=");

    MockLoad(ads_client_mock_);
    MockLoadUserModelForId(ads_client_mock_);
    MockLoadResourceForId(ads_client_mock_);
    MockSave(ads_client_mock_);

    database_ = std::make_unique<Database>(path.AppendASCII("database.sqlite"));
    MockRunDBTransaction(ads_client_mock_, database_);

    Initialize(ads_);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  UnblindedTokens* get_unblinded_tokens() {
    return ads_->get_confirmations()->get_unblinded_tokens();
  }

  UnblindedTokenInfo CreateUnblindedToken(
      const std::string& unblinded_token_base64) {
    UnblindedTokenInfo unblinded_token;

    unblinded_token.value =
        UnblindedToken::decode_base64(unblinded_token_base64);
    unblinded_token.public_key = PublicKey::decode_base64(
        "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");

    return unblinded_token;
  }

  UnblindedTokenList CreateUnblindedTokens(
      const std::vector<std::string>& unblinded_tokens_base64) {
    UnblindedTokenList unblinded_tokens;

    for (const auto& unblinded_token_base64 : unblinded_tokens_base64) {
      UnblindedTokenInfo unblinded_token =
          CreateUnblindedToken(unblinded_token_base64);

      unblinded_tokens.push_back(unblinded_token);
    }

    return unblinded_tokens;
  }

  UnblindedTokenList GetUnblindedTokens(
      const int count) {
    const std::vector<std::string> unblinded_tokens_base64 = {
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

    const int modulo = unblinded_tokens_base64.size();

    UnblindedTokenList unblinded_tokens;

    for (int i = 0; i < count; i++) {
      const std::string unblinded_token_base64 =
          unblinded_tokens_base64.at(i % modulo);
      const UnblindedTokenInfo unblinded_token =
          CreateUnblindedToken(unblinded_token_base64);

      unblinded_tokens.push_back(unblinded_token);
    }

    return unblinded_tokens;
  }

  UnblindedTokenList GetRandomUnblindedTokens(
      const int count) {
    UnblindedTokenList unblinded_tokens;

    const std::vector<Token> tokens = GenerateTokens(count);
    for (const auto& token : tokens) {
      const std::string token_base64 = token.encode_base64();
      const UnblindedTokenInfo unblinded_token =
          CreateUnblindedToken(token_base64);

      unblinded_tokens.push_back(unblinded_token);
    }

    return unblinded_tokens;
  }

  base::Value GetUnblindedTokensAsList(
      const int count) {
    base::Value list(base::Value::Type::LIST);

    const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(count);

    for (const auto& unblinded_token : unblinded_tokens) {
      base::Value dictionary(base::Value::Type::DICTIONARY);

      dictionary.SetKey("unblinded_token", base::Value(
          unblinded_token.value.encode_base64()));

      dictionary.SetKey("public_key",
          base::Value(unblinded_token.public_key.encode_base64()));

      list.Append(std::move(dictionary));
    }

    return list;
  }

  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<brave_l10n::LocaleHelperMock> locale_helper_mock_;
  std::unique_ptr<PlatformHelperMock> platform_helper_mock_;
  std::unique_ptr<Database> database_;
};

TEST_F(BatAdsUnblindedTokensTest,
    GetToken) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(10);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  const UnblindedTokenInfo unblinded_token = get_unblinded_tokens()->GetToken();

  // Assert
  const std::string expected_unblinded_token_base64 =
      "PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuY"
      "L5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY";
  UnblindedTokenInfo expected_unblinded_token =
      CreateUnblindedToken(expected_unblinded_token_base64);

  EXPECT_EQ(expected_unblinded_token, unblinded_token);
}

TEST_F(BatAdsUnblindedTokensTest,
    GetAllTokens) {
  // Arrange
  UnblindedTokenList unblinded_tokens = GetUnblindedTokens(8);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  unblinded_tokens = get_unblinded_tokens()->GetAllTokens();

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

  UnblindedTokenList expected_unblinded_tokens =
      CreateUnblindedTokens(expected_unblinded_tokens_base64);

  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokensTest,
    GetTokensAsList) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(8);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  const base::Value list = get_unblinded_tokens()->GetTokensAsList();

  // Assert
  base::ListValue list_values(list.GetList());

  EXPECT_EQ(list_values.GetSize(), unblinded_tokens.size());

  for (auto& value : list_values) {
    base::DictionaryValue* dictionary = nullptr;
    if (!value.GetAsDictionary(&dictionary)) {
      FAIL();
    }

    const std::string* unblinded_token_value =
        dictionary->FindStringKey("unblinded_token");
    if (!unblinded_token_value) {
      FAIL();
    }
    const std::string unblinded_token_base64 = *unblinded_token_value;

    const std::string* public_key_value =
        dictionary->FindStringKey("public_key");
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

TEST_F(BatAdsUnblindedTokensTest,
    GetTokensAsListWithEmptyList) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = {};
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  const base::Value list = get_unblinded_tokens()->GetTokensAsList();

  // Assert
  EXPECT_TRUE(list.GetList().empty());
}

TEST_F(BatAdsUnblindedTokensTest,
    SetTokens) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(10);

  // Act
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Assert
  const UnblindedTokenList expected_unblinded_tokens =
      get_unblinded_tokens()->GetAllTokens();

  EXPECT_EQ(expected_unblinded_tokens, unblinded_tokens);
}

TEST_F(BatAdsUnblindedTokensTest,
    SetTokensWithEmptyList) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const UnblindedTokenList unblinded_tokens = {};

  // Act
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    SetTokensFromList) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const base::Value list = GetUnblindedTokensAsList(5);

  // Act
  get_unblinded_tokens()->SetTokensFromList(list);

  // Assert
  const UnblindedTokenList unblinded_tokens =
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
    "T3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi"
  };

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

TEST_F(BatAdsUnblindedTokensTest,
    SetTokensFromListWithEmptyList) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const base::Value list = GetUnblindedTokensAsList(0);

  // Act
  get_unblinded_tokens()->SetTokensFromList(list);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    AddTokens) {
  // Arrange
  UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  unblinded_tokens = GetRandomUnblindedTokens(5);
  get_unblinded_tokens()->AddTokens(unblinded_tokens);

  // Assert
  for (const auto& unblinded_token : unblinded_tokens) {
    if (!get_unblinded_tokens()->TokenExists(unblinded_token)) {
      FAIL();
    }
  }

  SUCCEED();
}

TEST_F(BatAdsUnblindedTokensTest,
    DoNotAddDuplicateTokens) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const UnblindedTokenList duplicate_unblinded_tokens = GetUnblindedTokens(1);
  get_unblinded_tokens()->AddTokens(duplicate_unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    AddTokensCount) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(5);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const UnblindedTokenList random_unblinded_tokens =
      GetRandomUnblindedTokens(3);
  get_unblinded_tokens()->AddTokens(random_unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(8, count);
}

TEST_F(BatAdsUnblindedTokensTest,
     AddTokensWithEmptyList) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const UnblindedTokenList empty_unblinded_tokens = {};
  get_unblinded_tokens()->AddTokens(empty_unblinded_tokens);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(3, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    RemoveTokenCount) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  const std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    RemoveToken) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  EXPECT_FALSE(get_unblinded_tokens()->TokenExists(unblinded_token));
}

TEST_F(BatAdsUnblindedTokensTest,
    DoNotRemoveTokensThatDoNotExist) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(0);

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

TEST_F(BatAdsUnblindedTokensTest,
    DoNotRemoveTheSameTokenTwice) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  get_unblinded_tokens()->RemoveToken(unblinded_token);
  get_unblinded_tokens()->RemoveToken(unblinded_token);

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(2, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    RemoveAllTokens) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(7);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  get_unblinded_tokens()->RemoveAllTokens();

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    RemoveAllTokensWithEmptyList) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = {};
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  EXPECT_CALL(*ads_client_mock_, Save(_, _, _))
      .Times(1);

  get_unblinded_tokens()->RemoveAllTokens();

  // Assert
  const int count = get_unblinded_tokens()->Count();
  EXPECT_EQ(0, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    TokenExists) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  std::string unblinded_token_base64 =
      "hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6"
      "NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K";

  const UnblindedTokenInfo unblinded_token =
      CreateUnblindedToken(unblinded_token_base64);

  // Assert
  const bool exists = get_unblinded_tokens()->TokenExists(unblinded_token);

  EXPECT_TRUE(exists);
}

TEST_F(BatAdsUnblindedTokensTest,
    TokenDoesNotExist) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(3);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

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

TEST_F(BatAdsUnblindedTokensTest,
    Count) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(6);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  const int count = get_unblinded_tokens()->Count();

  // Assert
  EXPECT_EQ(6, count);
}

TEST_F(BatAdsUnblindedTokensTest,
    IsEmpty) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = {};
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  const bool is_empty = get_unblinded_tokens()->IsEmpty();

  // Assert
  EXPECT_TRUE(is_empty);
}

TEST_F(BatAdsUnblindedTokensTest,
    IsNotEmpty) {
  // Arrange
  const UnblindedTokenList unblinded_tokens = GetUnblindedTokens(9);
  get_unblinded_tokens()->SetTokens(unblinded_tokens);

  // Act
  const bool is_empty = get_unblinded_tokens()->IsEmpty();

  // Assert
  EXPECT_FALSE(is_empty);
}

}  // namespace privacy
}  // namespace ads
