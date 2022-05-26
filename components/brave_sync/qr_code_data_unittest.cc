/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/qr_code_data.h"

#include <memory>
#include <string>

#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_sync {

TEST(QrCodeData, SerializationVersionAsString) {
  const std::string json_string =
      R"(
{
  "version": "2",
  "sync_code_hex" : "current hex code",
  "not_after": "1637080050"
}
)";
  auto qr_code_data1 = QrCodeData::FromJson(json_string);
  ASSERT_NE(qr_code_data1.get(), nullptr);
  EXPECT_EQ(qr_code_data1->version, 2);
}

TEST(QrCodeData, Serialization) {
  const std::string json_string1 =
      R"(
{
  "version": "2",
  "sync_code_hex" : "current hex code",
  "not_after": "1637080050"
}
)";
  auto qr_code_data1 = QrCodeData::FromJson(json_string1);
  ASSERT_NE(qr_code_data1.get(), nullptr);
  EXPECT_EQ(qr_code_data1->version, 2);
  EXPECT_EQ(qr_code_data1->sync_code_hex, "current hex code");
  base::Time::Exploded exploded;
  qr_code_data1->not_after.UTCExplode(&exploded);
  // 1637080050 is for Tuesday, 16 November 2021 16:27:30
  EXPECT_EQ(exploded.year, 2021);
  EXPECT_EQ(exploded.month, 11);
  EXPECT_EQ(exploded.day_of_month, 16);
  EXPECT_EQ(exploded.hour, 16);
  EXPECT_EQ(exploded.minute, 27);
  EXPECT_EQ(exploded.second, 30);
  EXPECT_EQ(exploded.millisecond, 0);

  const std::string json_string = qr_code_data1->ToJson();
  EXPECT_NE(json_string.length(), 0ul);

  auto qr_code_data2 = QrCodeData::FromJson(json_string);
  EXPECT_EQ(qr_code_data1->version, 2);
  EXPECT_EQ(qr_code_data1->sync_code_hex, qr_code_data2->sync_code_hex);
  EXPECT_EQ(qr_code_data1->not_after, qr_code_data2->not_after);

  auto qr_code_data3 = QrCodeData::FromJson("not a json");
  EXPECT_EQ(qr_code_data3.get(), nullptr);
}

TEST(QrCodeData, CreateWithActualDate) {
  auto qr_code_data1 = QrCodeData::CreateWithActualDate("sync_code_hex");
  EXPECT_GE(qr_code_data1->not_after, base::Time::Now());
  EXPECT_LE(qr_code_data1->not_after,
            base::Time::Now() +
                base::Minutes(QrCodeData::kMinutesFromNowForValidCode));
}

}  // namespace brave_sync
