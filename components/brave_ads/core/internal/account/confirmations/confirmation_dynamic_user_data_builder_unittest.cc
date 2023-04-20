/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_dynamic_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationDynamicUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest, Build) {
  // Arrange
  ads_client_mock_->SetStringPref(prefs::kDiagnosticId,
                                  "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

  const base::Time time =
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false);
  AdvanceClockTo(time);

  // Act

  // Assert
  const ConfirmationDynamicUserDataBuilder user_data_builder;
  user_data_builder.Build(base::BindOnce([](base::Value::Dict user_data) {
    std::string json;
    ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

    const std::string expected_json =
        R"~({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","systemTimestamp":"2020-11-18T12:00:00.000Z"})~";
    EXPECT_EQ(expected_json, json);
  }));
}

}  // namespace brave_ads
