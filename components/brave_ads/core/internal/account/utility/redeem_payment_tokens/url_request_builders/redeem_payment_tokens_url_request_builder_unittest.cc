/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/url_request_builders/redeem_payment_tokens_url_request_builder.h"

#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/user_data/redeem_payment_tokens_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kExpectedUrlRequestContent[] =
    R"({"payload":"{\"paymentId\":\"27a39b2f-9b2e-4eb0-bbb2-2f84447496e7\"}","paymentCredentials":[{"confirmationType":"view","credential":{"signature":"H9HPNdEVJBvc9d8RZig/Gihlrcgug/n/rRaAJzeZI20gKPCivIj9Ig8StvqMSc5GfgLrBaJDibwBghnhRhqYRQ==","t":"PLowz2WF2eGD5zfwZjk9p76HXBLDKMq/3EAZHeG/fE2XGQ48jyte+Ve50ZlasOuYL5mwA8CU2aFMlJrt3DDgCw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"mfv+HJP5K/q9ogcGwD4uqOd98sb2fx96h+QnsdtGwJ4wdZfvrukbP4whyz46Ro3gm2FIMhPWZ5wM2Hhg9OGPtg==","t":"hfrMEltWLuzbKQ02Qixh5C/DWiJbdOoaGaidKZ7Mv+cRq5fyxJqemE/MPlARPhl6NgXPHUeyaxzd6/Lk6YHlfQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"acj3J7aaf/rN/uleCnaGTOzNceo9m9Dz/2D1+NDIVN4MnZG2BF1hYl5qHY/VRZNh5/uhnRbqgsuPn6SXvaTXVA==","t":"bbpQ1DcxfDA+ycNg9WZvIwinjO0GKnCon1UFxDLoDOLZVnKG3ufruNZi/n8dO+G2AkTiWkUKbi78xCyKsqsXnA=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"zETc7kPZQhzNaufsUiBHiBtnwEhk3oQYUh5HZeNbTOiHvm5pAzNl/TuR/fjQcusN5EqH7e1B+ycsO5CeF5FmAw==","t":"OlDIXpWRR1/B+1pjPbLyc5sx0V+d7QzQb4NDGUI6F676jy8tL++u57SF4DQhvdEpBrKID+j27RLrbjsecXSjRw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"d1oVe9XnwRs7bNcMzxp+/q7zL5hDHLD13f4tazz79a+ImszJfVRYIcf1fxvc7d2Pstciu1i11jRYGUWIDttYVg==","t":"Y579V5BUcCzAFj6qNX7YnIr+DvH0mugb/nnY5UINdjxziyDJlejJwi0kPaRGmqbVT3+B51lpErt8e66z0jTbAw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"rQkn3xdJu/GFfkGUREqvV69lQNQWsa5a9RQIXaig7CQtlS/PVLsUvM6plG4IeboK0E6gR8Do5Gg40OK59/ZyUQ==","t":"+MPQfSo6UcaZNWtfmbd5je9UIr+FVrCWHl6I5C1ZFD7y7bjP/yz7flTjV+l5mKulbCvsRna7++MhbBz6iC0FvQ=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="},{"confirmationType":"view","credential":{"signature":"GohmRsNFp8Vd47PB5MzgFvM7o/8XjkY+9gjCOJY/MAcpmLBgoS6v373A0Tvkq3T/t0z9GYM0yrs6LikJggwnmg==","t":"CRXUzo7S0X//u0RGsO534vCoIbrsXgbzLfWw8CLML0CkgMltEGxM6XwBTICl4dqqfhIcLhD0f1WFod7JpuEkjw=="},"publicKey":"RJ2i/o/pZkrH+i0aGEMY1G9FXtd7Q7gfRi3YdNRnDDk="}],"platform":"windows","totals":[{"ad_format":"ad_notification","view":7}]})";

}  // namespace

class BraveAdsRedeemPaymentTokensUrlRequestBuilderTest : public test::TestBase {
};

TEST_F(BraveAdsRedeemPaymentTokensUrlRequestBuilderTest, BuildUrl) {
  // Arrange
  const PaymentTokenList payment_tokens = test::BuildPaymentTokens(/*count=*/7);

  RedeemPaymentTokensUrlRequestBuilder url_request_builder(
      test::Wallet(), payment_tokens,
      BuildRedeemPaymentTokensUserData(payment_tokens));

  // Act
  const mojom::UrlRequestInfoPtr url_request = url_request_builder.Build();

  // Assert
  const mojom::UrlRequestInfoPtr expected_url_request =
      mojom::UrlRequestInfo::New();
  expected_url_request->url = GURL(
      R"(https://mywallet.ads.bravesoftware.com/v3/confirmation/payment/27a39b2f-9b2e-4eb0-bbb2-2f84447496e7)");
  expected_url_request->headers = {"accept: application/json"};
  expected_url_request->content = kExpectedUrlRequestContent;
  expected_url_request->content_type = "application/json";
  expected_url_request->method = mojom::UrlRequestMethodType::kPut;
  EXPECT_EQ(expected_url_request, url_request);
}

}  // namespace brave_ads
