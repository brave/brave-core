/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_url_request_builder.h"

#include <vector>

#include "base/check.h"
#include "bat/ads/internal/account/utility/redeem_unblinded_payment_tokens/redeem_unblinded_payment_tokens_user_data_builder.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/flags/flag_manager_util.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/public_key.h"
#include "bat/ads/internal/privacy/challenge_bypass_ristretto/unblinded_token.h"
#include "bat/ads/sys_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

privacy::UnblindedPaymentTokenList GetUnblindedPaymentTokens(const int count) {
  const std::vector<std::string> unblinded_payment_tokens_base64 = {
      R"~(PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgC3B1+VD/uyHPfa/+bwYRrpVH5YwNSDEydVx8S4r+BYVY)~",
      R"~(hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfXbBA023DYvGMHoKm15NP/nWnZ1V3iLkgOOHZuk80Z4K)~",
      R"~(bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnGYUlA/6MMEOzmR67rZhMwdJHr14Fu+TCI9JscDlWepa)~",
      R"~(OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjR5oieuH4Bx5mHqTb/rAPI6RpaAXtfXYrCYbf7EPwHTMU)~",
      R"~(Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAxBfhtXKARFKtGH8WccB6NfCa85XHBmlcuv1+zcFPDJi)~",
      R"~(+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvVZGYXLeLn2HSAM7cDgqyW6SEuPzlDeZT6kkTNI7JcQm)~",
      R"~(CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkj5pW/rg7nl48EX6nmekgd3D2Hz8JgJnSarzP/8+3l+MW)~",
      R"~(hQ+6+jh5DUUBFhhGn7bPLDjqrUIKNi/T8QDt1x01bcW9PLADg6aS73dzrVBsHav44+4q1QhFE/93u0KHVtZ1RPKMqkt8MIiC6RG575102nGRTJDA2kSOgUM75hjDsI8z)~",
      R"~(6tKJHOtQqpNzFjLGT0gvXlCF0GGKrqQlK82e2tc7gJvQkorg60Y21jEAg8JHbU8D3mBK/riZCILoi1cPCiBDAdhWJNVm003mZ0ShjmbESnKhL/NxRv/0/PB3GQ5iydoc)~",
      R"~(ujGlRHnz+UF0h8i6gYDnfeZDUj7qZZz6o29ZJFa3XN2g+yVXgRTws1yv6RAtLCr39OQso6FAT12o8GAvHVEzmRqyzm2XU9gMK5WrNtT/fhr8gQ9RvupdznGKOqmVbuIc)~"};

  const int modulo = unblinded_payment_tokens_base64.size();

  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;
  for (int i = 0; i < count; i++) {
    privacy::UnblindedPaymentTokenInfo unblinded_payment_token;

    const std::string& unblinded_payment_token_base64 =
        unblinded_payment_tokens_base64.at(i % modulo);
    unblinded_payment_token.value =
        privacy::cbr::UnblindedToken(unblinded_payment_token_base64);
    CHECK(unblinded_payment_token.value.has_value());

    unblinded_payment_token.public_key =
        privacy::cbr::PublicKey("RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk=");
    CHECK(unblinded_payment_token.public_key.has_value());

    unblinded_payment_token.confirmation_type = ConfirmationType::kViewed;

    unblinded_payment_token.ad_type = AdType::kNotificationAd;

    unblinded_payment_tokens.push_back(unblinded_payment_token);
  }

  return unblinded_payment_tokens;
}

}  // namespace

class BatAdsRedeemUnblindedPaymentTokensUrlRequestBuilderTest
    : public UnitTestBase {};

TEST_F(BatAdsRedeemUnblindedPaymentTokensUrlRequestBuilderTest,
       BuildUrlForRPill) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SysInfo().is_uncertain_future = true;

  SetEnvironmentTypeForTesting(EnvironmentType::kStaging);

  WalletInfo wallet;
  wallet.id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.secret_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      GetUnblindedPaymentTokens(7);

  const RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      unblinded_payment_tokens);

  // Act
  user_data_builder.Build([&wallet, &unblinded_payment_tokens](
                              const base::Value::Dict& user_data) {
    RedeemUnblindedPaymentTokensUrlRequestBuilder url_request_builder(
        wallet, unblinded_payment_tokens, user_data);

    const mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

    mojom::UrlRequestInfoPtr expected_url_request =
        mojom::UrlRequestInfo::New();
    expected_url_request->url = GURL(
        R"(https://mywallet.ads.bravesoftware.com/v2/confirmation/payment/d4ed0af0-bfa9-464b-abd7-67b29d891b8b)");
    expected_url_request->headers = {
        R"(Via: 1.1 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
        R"(accept: application/json)"};
    expected_url_request->content =
        R"({"odyssey":"guest","payload":"{\"paymentId\":\"d4ed0af0-bfa9-464b-abd7-67b29d891b8b\"}","paymentCredentials":[{"confirmationType":"view","credential":{"signature":"wQXvy7chZlrrVCe/RYIiL/siGUFYF0tCxx7M0xIOPvThR4TCBwmH9IDWQKyqQy9g2wUw5jcKszqBHEhPyidrlA==","t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"AemGBdoUXbp25pGZJuWv6yiImtfXC4AtboJMGR1Z6nQm178ier7hLJDVCJ11HWEO1UdlAYFRrJqyuD5uUBxgug==","t":"hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"krVZeadk/ElvsaYiUE4Ma/hkicRDjvS8O7QVkrWl0n2zsGYyAa/hodVb1aDn8tT3CMOV/l1JZdTVSXHrSHBHGg==","t":"bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnA=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"1HwlWbhUewzvEWfGlOhmEo8x4FR3w82iRan+ZyBl1h3laOiXTVHXe5EraDiUd3G6bZlLJ+x9snDXPcd4wI5tpA==","t":"OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjRw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"c9wbOwh7xT3Fx89HKh6D4isUU8ki9vTq+1MR81bRyPWCv0lDHYchd7Kk9EFtz3qNip4nZpSDUDDqV5Gu3ac2DA==","t":"Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"PW8G57q6/hoj0GzBoiRPilmPyWSYrFfOpJJ9I0tLsNfNF+DNOASnBoRpUy6nGJLX1vWcJnUQGGVr9hfwBNTGfg==","t":"+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"Rn9mRKy6B0Sysx6+y3scWE+ZE6EWVA/pYTp1XqOLFZH3IVVh+WnIVP/FNA7GuexDmVaq8/an8+9Gv7puKpQPWA==","t":"CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkjw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}],"platform":"windows","totals":[{"ad_format":"ad_notification","view":"7"}]})";
    expected_url_request->content_type = "application/json";
    expected_url_request->method = mojom::UrlRequestMethodType::kPut;

    EXPECT_EQ(url_request, expected_url_request);
  });

  // Assert
}

TEST_F(BatAdsRedeemUnblindedPaymentTokensUrlRequestBuilderTest,
       BuildUrlForBPill) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  SysInfo().is_uncertain_future = false;

  SetEnvironmentTypeForTesting(EnvironmentType::kStaging);

  WalletInfo wallet;
  wallet.id = "d4ed0af0-bfa9-464b-abd7-67b29d891b8b";
  wallet.secret_key =
      "e9b1ab4f44d39eb04323411eed0b5a2ceedff01264474f86e29c707a56615650"
      "33cea0085cfd551faa170c1dd7f6daaa903cdd3138d61ed5ab2845e224d58144";

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      GetUnblindedPaymentTokens(7);

  const RedeemUnblindedPaymentTokensUserDataBuilder user_data_builder(
      unblinded_payment_tokens);

  // Act
  user_data_builder.Build([&wallet, &unblinded_payment_tokens](
                              const base::Value::Dict& user_data) {
    RedeemUnblindedPaymentTokensUrlRequestBuilder url_request_builder(
        wallet, unblinded_payment_tokens, user_data);

    const mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

    mojom::UrlRequestInfoPtr expected_url_request =
        mojom::UrlRequestInfo::New();
    expected_url_request->url = GURL(
        R"(https://mywallet.ads.bravesoftware.com/v2/confirmation/payment/d4ed0af0-bfa9-464b-abd7-67b29d891b8b)");
    expected_url_request->headers = {
        R"(Via: 1.0 brave, 1.1 ads-serve.brave.com (Apache/1.1))",
        R"(accept: application/json)"};
    expected_url_request->content =
        R"({"odyssey":"host","payload":"{\"paymentId\":\"d4ed0af0-bfa9-464b-abd7-67b29d891b8b\"}","paymentCredentials":[{"confirmationType":"view","credential":{"signature":"wQXvy7chZlrrVCe/RYIiL/siGUFYF0tCxx7M0xIOPvThR4TCBwmH9IDWQKyqQy9g2wUw5jcKszqBHEhPyidrlA==","t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"AemGBdoUXbp25pGZJuWv6yiImtfXC4AtboJMGR1Z6nQm178ier7hLJDVCJ11HWEO1UdlAYFRrJqyuD5uUBxgug==","t":"hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"krVZeadk/ElvsaYiUE4Ma/hkicRDjvS8O7QVkrWl0n2zsGYyAa/hodVb1aDn8tT3CMOV/l1JZdTVSXHrSHBHGg==","t":"bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnA=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"1HwlWbhUewzvEWfGlOhmEo8x4FR3w82iRan+ZyBl1h3laOiXTVHXe5EraDiUd3G6bZlLJ+x9snDXPcd4wI5tpA==","t":"OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjRw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"c9wbOwh7xT3Fx89HKh6D4isUU8ki9vTq+1MR81bRyPWCv0lDHYchd7Kk9EFtz3qNip4nZpSDUDDqV5Gu3ac2DA==","t":"Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"PW8G57q6/hoj0GzBoiRPilmPyWSYrFfOpJJ9I0tLsNfNF+DNOASnBoRpUy6nGJLX1vWcJnUQGGVr9hfwBNTGfg==","t":"+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"Rn9mRKy6B0Sysx6+y3scWE+ZE6EWVA/pYTp1XqOLFZH3IVVh+WnIVP/FNA7GuexDmVaq8/an8+9Gv7puKpQPWA==","t":"CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkjw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}],"platform":"windows","totals":[{"ad_format":"ad_notification","view":"7"}]})";
    expected_url_request->content_type = "application/json";
    expected_url_request->method = mojom::UrlRequestMethodType::kPut;

    EXPECT_EQ(url_request, expected_url_request);
  });

  // Assert
}

}  // namespace ads
