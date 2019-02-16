/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "confirmations_client_mock.h"
#include "bat-native-confirmations/src/confirmations_impl.h"
#include "bat-native-confirmations/src/redeem_payment_tokens_request.h"
#include "bat-native-confirmations/src/unblinded_tokens.h"
#include "bat-native-confirmations/include/bat/confirmations/wallet_info.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=Confirmations*

namespace confirmations {

class ConfirmationsRedeemPaymentTokensRequestTest : public ::testing::Test {
 protected:
  MockConfirmationsClient* mock_confirmations_client_;
  ConfirmationsImpl* confirmations_;

  UnblindedTokens* unblinded_tokens_;

  RedeemPaymentTokensRequest* request_;

  ConfirmationsRedeemPaymentTokensRequestTest() :
      mock_confirmations_client_(new MockConfirmationsClient()),
      confirmations_(new ConfirmationsImpl(mock_confirmations_client_)),
      unblinded_tokens_(new UnblindedTokens(confirmations_)),
      request_(new RedeemPaymentTokensRequest()) {
    // You can do set-up work for each test here
  }

  ~ConfirmationsRedeemPaymentTokensRequestTest() override {
    // You can do clean-up work that doesn't throw exceptions here
    delete request_;

    delete unblinded_tokens_;

    delete confirmations_;
    delete mock_confirmations_client_;
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
    std::vector<std::string> unblinded_tokens_base64 = {
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

    int modulo = unblinded_tokens_base64.size();

    std::vector<UnblindedToken> unblinded_tokens;
    for (int i = 0; i < count; i++) {
      auto unblinded_token_base64 = unblinded_tokens_base64.at(i % modulo);

      auto unblinded_token =
          UnblindedToken::decode_base64(unblinded_token_base64);

      unblinded_tokens.push_back(unblinded_token);
    }

    return unblinded_tokens;
  }
};

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, BuildUrl) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  // Act
  auto url = request_->BuildUrl(wallet_info);

  // Assert
  std::string expected_url = "https://ads-serve.bravesoftware.com/v1/confirmation/payment/e7fcf220-d3f4-4111-a0b2-6157d0347567";  // NOLINT
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
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  auto unblinded_tokens = GetUnblindedTokens(7);
  unblinded_tokens_->SetTokens(unblinded_tokens);

  auto tokens = unblinded_tokens_->GetAllTokens();

  auto payload = request_->CreatePayload(wallet_info);

  // Act
  auto body = request_->BuildBody(tokens, payload, wallet_info);

  // Assert
  std::string expected_body = R"({"payload":"{\"paymentId\":\"e7fcf220-d3f4-4111-a0b2-6157d0347567\"}","paymentCredentials":[{"credential":{"signature":"Vdt2I2razGwIiVaHsFomAZjJAJETqVwcFFd0iT+hsGiQu0HB/0ZRwgHcAkhJuVt0j7Dl5VfTwmy7BfA3arwmjA==","t":"gXMEnFFPTfgVA3MB11zNRP1ixWjkdw/qsW1RnuQlfkF+ugGxFLafpypS7OJ7mB1zTP775LXrO9vM48fAFNihCA=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"},{"credential":{"signature":"cMsDGegcXs2YgFzQTMkgi6KHhnLiehiY6cNi/8TWD5h+9JAecWYwGKCLz8DXbN7DIe5tNL8DjRu0tL9PCz92ZQ==","t":"nEHl6RxMncjw0NKaRxrdpa5iZ7bD+nvBm4yifAYrFgEPJ9DluocwsSS2JUy1nkkcPwWQC3wx5ekhL3Ca9xi7yQ=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"},{"credential":{"signature":"Iy6MUNwZ4pIGNIuOKyPJC2Pc+GfJQEgp0ImkB8ZBuIM4+opdxGBlMmY+oTz68/ovoZHi2Vcl3LEHL68dxPLLyQ==","t":"MNrshKuw6zUTsmlZ+w4WzlJknjV/m/ZYyWUhwSmzyW8Dm/VGpMrifyw5txpNu+SQyNcAR+EJ468ADS5qfNfH7w=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"},{"credential":{"signature":"oAzrUB6X14gDn/WGT2CGkhfCr2YtHSV5Yh/qQw6TkD3ESq8rIPnJLAZBCu82AzpCHpQD03KvApWq/ZidevAh8w==","t":"MRAbYbmnmjM5bqlbHsX9iuy1Jwc9GCGEA4idBt+PNaQONgbZaPbxHb2pOjw1H6sbgJ2eeIwtobrRRmy+diurWg=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"},{"credential":{"signature":"OWc9CLfJSwRIDGjbNZkd5frik39/meIjEJCmcHlDWsc6IOn2qr4iI1XI8SAAfhT9ncUWGThrGZyUC1PSGl4tGw==","t":"aMTJ5HnQot4p6lU5LuXMdYPt3q3Eg1pz5pB2q1c8ys6qVVHd1PyrtEVY+qGJrET3ay2E12Qft0UhNzVUkrgnZw=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"},{"credential":{"signature":"VgybmDGXgO9Z90KZ4zHf/cH0fFZrpJbIKMDVdK5sE726b6yVVvcQ6KhOhupwMOe8n71C3geNJb2Adf5CBpwClw==","t":"lv6mXcIzMFmBbK37U3SFRxgMiRcM4pGLfrdgp0TCevTJ+XbDlHGNIXxYU8CT8ztGwoJSxYjtBh/MGSpjaklJGw=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"},{"credential":{"signature":"ioKIMHG1JQnrbMA3PIvM2q8kzdi6KTKxFdyQB/LATqfD2mzihGG8lsic1CeyD2dhJg8ZqLuwfHwgyT51vD4NFA==","t":"f3v9XvsBKp7fdXwQSQHNpHN0MPDzGJ1obhc37pLLyv65/JbdMbsXSQ1dGP0+nD/ETvAFzWzro9s/8HQo0MPLBg=="},"publicKey":"3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf"}]})";  // NOLINT
  EXPECT_EQ(expected_body, body);
}

TEST_F(ConfirmationsRedeemPaymentTokensRequestTest, CreatePayload) {
  // Arrange
  WalletInfo wallet_info;
  wallet_info.payment_id = "e7fcf220-d3f4-4111-a0b2-6157d0347567";
  wallet_info.public_key = "3fc8ff3b121e7b7875750d26eaba6f06a3b06d96cf6b2fb898323917e7be9d16e255a4a6f7eb8647428f727c0d4e1958bd8e69a984eee38514d1e483aab27edf";  // NOLINT

  // Act
  auto payload = request_->CreatePayload(wallet_info);

  // Assert
  std::string expected_payload =
      R"({"paymentId":"e7fcf220-d3f4-4111-a0b2-6157d0347567"})";
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
