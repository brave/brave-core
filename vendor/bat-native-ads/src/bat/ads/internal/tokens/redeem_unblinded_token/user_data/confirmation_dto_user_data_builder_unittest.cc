/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_dto_user_data_builder.h"

#include <string>

#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/unittest_base.h"
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

bool IsValidUserDataDictionary(const base::Value user_data) {
  const base::DictionaryValue* user_data_dictionary = nullptr;
  user_data.GetAsDictionary(&user_data_dictionary);
  if (!user_data_dictionary->is_dict()) {
    return false;
  }

  const base::Value* envelope_dictionary =
      user_data_dictionary->FindDictKey("conversionEnvelope");

  if (!IsValidEnvelope(*envelope_dictionary)) {
    return false;
  }

  const std::string* build_channel =
      user_data_dictionary->FindStringKey("buildChannel");
  if (!build_channel || build_channel->empty()) {
    return false;
  }

  if ("release" != *build_channel) {
    return false;
  }

  const std::string* country_code =
      user_data_dictionary->FindStringKey("countryCode");
  if (!country_code || country_code->empty()) {
    return false;
  }

  if ("GB" != *country_code) {
    return false;
  }

  const std::string* platform = user_data_dictionary->FindStringKey("platform");
  if (!platform || platform->empty()) {
    return false;
  }

  if ("macos" != *platform) {
    return false;
  }

  const base::Value* studies = user_data_dictionary->FindListKey("studies");
  if (!studies || !studies->is_list()) {
    return false;
  }

  return true;
}

}  // namespace

class BatAdsConfirmationDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationDtoUserDataTest()
      : database_table_(std::make_unique<database::table::ConversionQueue>()) {}

  ~BatAdsConfirmationDtoUserDataTest() override = default;

  void Save(const ConversionQueueItemList& conversion_queue_items) {
    database_table_->Save(conversion_queue_items,
                          [](const bool success) { ASSERT_TRUE(success); });
  }

  std::unique_ptr<database::table::ConversionQueue> database_table_;
};

TEST_F(BatAdsConfirmationDtoUserDataTest, BuildWithoutConversion) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);
  SetBuildChannel(true, "release");
  MockLocaleHelper(locale_helper_mock_, "en-GB");

  // Act
  const std::string creative_instance_id =
      "465f10df-fbc4-4a92-8d43-4edf73734a60";
  const ConfirmationType confirmation_type = ConfirmationType::kViewed;

  // Assert
  dto::user_data::Build(
      creative_instance_id, confirmation_type,
      [=](const base::Value& user_data) {
        const base::DictionaryValue* user_data_dictionary = nullptr;
        user_data.GetAsDictionary(&user_data_dictionary);

        base::DictionaryValue expected_user_data;
        expected_user_data.SetKey("buildChannel", base::Value("release"));
        expected_user_data.SetKey("countryCode", base::Value("GB"));
        expected_user_data.SetKey("platform", base::Value("macos"));
        expected_user_data.SetKey("studies",
                                  base::Value(base::Value::Type::LIST));

        EXPECT_EQ(expected_user_data, *user_data_dictionary);
      });
}

TEST_F(BatAdsConfirmationDtoUserDataTest, BuildWithConversion) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);
  SetBuildChannel(true, "release");
  MockLocaleHelper(locale_helper_mock_, "en-GB");

  // Act
  const std::string creative_instance_id =
      "465f10df-fbc4-4a92-8d43-4edf73734a60";
  const ConfirmationType confirmation_type = ConfirmationType::kConversion;

  ConversionQueueItemList conversion_queue_items;

  ConversionQueueItemInfo info;
  info.creative_instance_id = creative_instance_id;
  info.creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  info.campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  info.advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  info.timestamp = Now();
  info.conversion_id = "smartbrownfoxes42";
  info.advertiser_public_key = "ofIveUY/bM7qlL9eIkAv/xbjDItFs1xRTTYKRZZsPHI=";
  conversion_queue_items.push_back(info);

  Save(conversion_queue_items);

  // Act

  // Assert
  dto::user_data::Build(
      creative_instance_id, confirmation_type,
      [=](const base::Value& user_data) {
        const base::DictionaryValue* user_data_dictionary = nullptr;
        user_data.GetAsDictionary(&user_data_dictionary);

        bool is_valid =
            IsValidUserDataDictionary(user_data_dictionary->Clone());

        EXPECT_TRUE(is_valid);
      });
}

}  // namespace ads
