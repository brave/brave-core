/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/catalog_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/catalog/catalog_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {

namespace {

constexpr char kCatalogId[] = "04a13086-8fd8-4dce-a44f-afe86f14a662";

std::string GetCatalogAsJson() {
  const base::DictionaryValue user_data = GetCatalog();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

class BatAdsConfirmationCatalogDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationCatalogDtoUserDataTest() = default;

  ~BatAdsConfirmationCatalogDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationCatalogDtoUserDataTest, GetCatalog) {
  // Arrange
  SetCatalogId(kCatalogId);

  // Act
  const std::string json = GetCatalogAsJson();

  // Assert
  const std::string expected_json =
      R"({"catalog":[{"id":"04a13086-8fd8-4dce-a44f-afe86f14a662"}]})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace user_data
}  // namespace ads
