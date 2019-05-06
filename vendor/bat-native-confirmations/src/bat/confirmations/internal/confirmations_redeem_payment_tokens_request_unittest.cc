/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <sstream>

#include "bat/confirmations/wallet_info.h"

#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/redeem_payment_tokens_request.h"
#include "bat/confirmations/internal/unblinded_tokens.h"

#include "base/files/file_path.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

using ::testing::_;
using ::testing::Invoke;

namespace confirmations {

class ConfirmationsRedeemPaymentTokensRequestTest : public ::testing::Test {
 protected:
  std::unique_ptr<MockConfirmationsClient> mock_confirmations_client_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<UnblindedTokens> unblinded_tokens_;

  std::unique_ptr<RedeemPaymentTokensRequest> request_;

  ConfirmationsRedeemPaymentTokensRequestTest() :
      mock_confirmations_client_(std::make_unique<MockConfirmationsClient>()),
      confirmations_(std::make_unique<ConfirmationsImpl>(
          mock_confirmations_client_.get())),
      unblinded_tokens_(std::make_unique<UnblindedTokens>(
          confirmations_.get())),
      request_(std::make_unique<RedeemPaymentTokensRequest>()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsRedeemPaymentTokensRequestTest() override {
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

    std::vector<TokenInfo> tokens;
    for (int i = 0; i < count; i++) {
      TokenInfo token_info;
      auto token_base64 = tokens_base64.at(i % modulo);
      token_info.unblinded_token = UnblindedToken::decode_base64(token_base64);
      token_info.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

      tokens.push_back(token_info);
    }

    return tokens;
  }
};

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, BuildUrl) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.public_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  // Act
  auto url = request_->BuildUrl(wallet_info);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/payment/d4ed0af0-bfa9-464b-abd7-67b29d891b8b";  // NOLINT
  EXPECT_EQ(expected_url, url);
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, GetMethod) {
  // Arrange

  // Act
  auto method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::PUT, method);
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, BuildBody) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.public_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  auto unblinded_tokens = GetUnblindedTokens(7);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  auto tokens = unblinded_tokens_->GetAllTokens();

  auto payload = request_->CreatePayload(wallet_info);

  // Act
  auto body = request_->BuildBody(tokens, payload);

  // Assert
  std::string expected_body = R"({"payload":"{\"paymentId\":\"d4ed0af0-bfa9-464b-abd7-67b29d891b8b\"}","paymentCredentials":[{"credential":{"signature":"wQXvy7chZlrrVCe/RYIiL/siGUFYF0tCxx7M0xIOPvThR4TCBwmH9IDWQKyqQy9g2wUw5jcKszqBHEhPyidrlA==","t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"AemGBdoUXbp25pGZJuWv6yiImtfXC4AtboJMGR1Z6nQm178ier7hLJDVCJ11HWEO1UdlAYFRrJqyuD5uUBxgug==","t":"hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"krVZeadk/ElvsaYiUE4Ma/hkicRDjvS8O7QVkrWl0n2zsGYyAa/hodVb1aDn8tT3CMOV/l1JZdTVSXHrSHBHGg==","t":"bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnA=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"1HwlWbhUewzvEWfGlOhmEo8x4FR3w82iRan+ZyBl1h3laOiXTVHXe5EraDiUd3G6bZlLJ+x9snDXPcd4wI5tpA==","t":"OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjRw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"c9wbOwh7xT3Fx89HKh6D4isUU8ki9vTq+1MR81bRyPWCv0lDHYchd7Kk9EFtz3qNip4nZpSDUDDqV5Gu3ac2DA==","t":"Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"PW8G57q6/hoj0GzBoiRPilmPyWSYrFfOpJJ9I0tLsNfNF+DNOASnBoRpUy6nGJLX1vWcJnUQGGVr9hfwBNTGfg==","t":"+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"Rn9mRKy6B0Sysx6+y3scWE+ZE6EWVA/pYTp1XqOLFZH3IVVh+WnIVP/FNA7GuexDmVaq8/an8+9Gv7puKpQPWA==","t":"CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkjw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}]})";  // NOLINT
  EXPECT_EQ(expected_body, body);
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, CreatePayload) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet_info.public_key = "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a5661565033cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";  // NOLINT

  // Act
  auto payload = request_->CreatePayload(wallet_info);

  // Assert
  std::string expected_payload =
      R"({"paymentId":"d4ed0af0-bfa9-464b-abd7-67b29d891b8b"})";
  EXPECT_EQ(expected_payload, payload);
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, HeadersCount) {
  // Arrange

  // Act
  auto headers = request_->BuildHeaders();

  // Assert
  auto count = headers.size();
  EXPECT_EQ(1UL, count);
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, GetAcceptHeaderValue) {
  // Arrange

  // Act
  auto accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ(accept_header_value, "application/json");
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, GetContentType) {
  // Arrange

  // Act
  auto content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ(content_type, "application/json");
}

}  // namespace confirmations
