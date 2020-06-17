/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/redeem_unblinded_payment_tokens_request.h"

#include <memory>
#include <string>
#include <vector>

#include "base/test/task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "bat/confirmations/internal/confirmations_client_mock.h"
#include "bat/confirmations/internal/confirmations_impl.h"
#include "bat/confirmations/internal/unblinded_tokens.h"
#include "bat/confirmations/internal/unittest_utils.h"
#include "bat/confirmations/wallet_info.h"

// npm run test -- brave_unit_tests --filter=BatConfirmations*

using ::testing::NiceMock;

namespace confirmations {

class BatConfirmationsRedeemUnblindedPaymentTokensRequestTest
    : public ::testing::Test {
 protected:
  BatConfirmationsRedeemUnblindedPaymentTokensRequestTest()
      : confirmations_client_mock_(std::make_unique<
            NiceMock<ConfirmationsClientMock>>()),
        confirmations_(std::make_unique<ConfirmationsImpl>(
            confirmations_client_mock_.get())),
        unblinded_tokens_(std::make_unique<UnblindedTokens>(
            confirmations_.get())),
        request_(std::make_unique<RedeemUnblindedPaymentTokensRequest>()) {
    // You can do set-up work for each test here
  }

  ~BatConfirmationsRedeemUnblindedPaymentTokensRequestTest() override {
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

    TokenList tokens;
    for (int i = 0; i < count; i++) {
      TokenInfo token;
      const std::string token_base64 = tokens_base64.at(i % modulo);
      token.unblinded_token = UnblindedToken::decode_base64(token_base64);
      token.public_key = "RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=";

      tokens.push_back(token);
    }

    return tokens;
  }

  base::test::TaskEnvironment task_environment_;

  std::unique_ptr<ConfirmationsClientMock> confirmations_client_mock_;
  std::unique_ptr<ConfirmationsImpl> confirmations_;

  std::unique_ptr<UnblindedTokens> unblinded_tokens_;

  std::unique_ptr<RedeemUnblindedPaymentTokensRequest> request_;
};

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    BuildUrl) {
  // Arrange
  WalletInfo wallet;
  wallet.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.private_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  // Act
  const std::string url = request_->BuildUrl(wallet);

  // Assert
  const std::string expected_url = R"(https://ads-serve.bravesoftware.com/v1/confirmation/payment/d4ed0af0-bfa9-464b-abd7-67b29d891b8b)";

  EXPECT_EQ(expected_url, url);
}

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    GetMethod) {
  // Arrange

  // Act
  const URLRequestMethod method = request_->GetMethod();

  // Assert
  EXPECT_EQ(URLRequestMethod::PUT, method);
}

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    BuildBody) {
  // Arrange
  WalletInfo wallet;
  wallet.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.private_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  const TokenList unblinded_tokens = GetUnblindedTokens(7);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  const TokenList tokens = unblinded_tokens_->GetAllTokens();

  const std::string payload = request_->CreatePayload(wallet);

  // Act
  const std::string body = request_->BuildBody(tokens, payload);

  // Assert
  const std::string expected_body = R"({"payload":"{\"paymentId\":\"d4ed0af0-bfa9-464b-abd7-67b29d891b8b\"}","paymentCredentials":[{"credential":{"signature":"wQXvy7chZlrrVCe/RYIiL/siGUFYF0tCxx7M0xIOPvThR4TCBwmH9IDWQKyqQy9g2wUw5jcKszqBHEhPyidrlA==","t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"AemGBdoUXbp25pGZJuWv6yiImtfXC4AtboJMGR1Z6nQm178ier7hLJDVCJ11HWEO1UdlAYFRrJqyuD5uUBxgug==","t":"hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"krVZeadk/ElvsaYiUE4Ma/hkicRDjvS8O7QVkrWl0n2zsGYyAa/hodVb1aDn8tT3CMOV/l1JZdTVSXHrSHBHGg==","t":"bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnA=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"1HwlWbhUewzvEWfGlOhmEo8x4FR3w82iRan+ZyBl1h3laOiXTVHXe5EraDiUd3G6bZlLJ+x9snDXPcd4wI5tpA==","t":"OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjRw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"c9wbOwh7xT3Fx89HKh6D4isUU8ki9vTq+1MR81bRyPWCv0lDHYchd7Kk9EFtz3qNip4nZpSDUDDqV5Gu3ac2DA==","t":"Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"PW8G57q6/hoj0GzBoiRPilmPyWSYrFfOpJJ9I0tLsNfNF+DNOASnBoRpUy6nGJLX1vWcJnUQGGVr9hfwBNTGfg==","t":"+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"credential":{"signature":"Rn9mRKy6B0Sysx6+y3scWE+ZE6EWVA/pYTp1XqOLFZH3IVVh+WnIVP/FNA7GuexDmVaq8/an8+9Gv7puKpQPWA==","t":"CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkjw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}]})";

  EXPECT_EQ(expected_body, body);
}

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    CreatePayload) {
  // Arrange
  WalletInfo wallet;
  wallet.payment_id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.private_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  // Act
  const std::string payload = request_->CreatePayload(wallet);

  // Assert
  std::string expected_payload =
      R"({"paymentId":"d4ed0af0-bfa9-464b-abd7-67b29d891b8b"})";

  EXPECT_EQ(expected_payload, payload);
}

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    HeadersCount) {
  // Arrange

  // Act
  const std::vector<std::string> headers = request_->BuildHeaders();

  // Assert
  const size_t count = headers.size();
  EXPECT_EQ(1UL, count);
}

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    GetAcceptHeaderValue) {
  // Arrange

  // Act
  const std::string accept_header_value = request_->GetAcceptHeaderValue();

  // Assert
  EXPECT_EQ(accept_header_value, "application/json");
}

TEST_F(BatConfirmationsRedeemUnblindedPaymentTokensRequestTest,
    GetContentType) {
  // Arrange

  // Act
  const std::string content_type = request_->GetContentType();

  // Assert
  EXPECT_EQ(content_type, "application/json");
}

}  // namespace confirmations
