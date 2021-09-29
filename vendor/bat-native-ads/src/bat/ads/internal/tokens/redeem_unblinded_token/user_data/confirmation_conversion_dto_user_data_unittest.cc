/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_conversion_dto_user_data.h"

#include <string>

#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

bool IsValidEnvelope(const base::Value& envelope) {
  if (!envelope.is_dict()) {
    return false;
  }

  const std::string* alg = envelope.FindStringKey("alg");
  if (!alg || alg->empty()) {
    return false;
  }

  const std::string* ciphertext = envelope.FindStringKey("ciphertext");
  if (!ciphertext || ciphertext->empty()) {
    return false;
  }

  const std::string* epk = envelope.FindStringKey("epk");
  if (!epk || epk->empty()) {
    return false;
  }

  const std::string* nonce = envelope.FindStringKey("nonce");
  if (!nonce || nonce->empty()) {
    return false;
  }

  return true;
}

}  // namespace

class BatAdsConfirmationConversionDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationConversionDtoUserDataTest() = default;

  ~BatAdsConfirmationConversionDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationConversionDtoUserDataTest, GetInvalidConversion) {
  // Arrange
  ConversionQueueItemInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.conversion_id = "";
  info.advertiser_public_key = "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  info.confirm_at = Now();

  // Act
  const base::DictionaryValue user_data = dto::user_data::GetConversion(info);

  // Assert
  const base::Value* envelope_dictionary =
      user_data.FindDictKey("conversionEnvelope");

  EXPECT_FALSE(envelope_dictionary);
}

TEST_F(BatAdsConfirmationConversionDtoUserDataTest, GetValidConversion) {
  // Arrange
  ConversionQueueItemInfo info;
  info.creative_instance_id = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.conversion_id = "smartbrownfoxes42";
  info.advertiser_public_key = "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  info.confirm_at = Now();

  // Act
  const base::DictionaryValue user_data = dto::user_data::GetConversion(info);

  // Assert
  const base::Value* envelope_dictionary =
      user_data.FindDictKey("conversionEnvelope");
  bool is_valid = IsValidEnvelope(*envelope_dictionary);

  EXPECT_TRUE(is_valid);
}

}  // namespace ads
