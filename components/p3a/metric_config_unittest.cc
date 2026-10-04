/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/metric_config.h"

#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a {

namespace {

RemoteMetricConfig ConvertFromDict(base::DictValue dict) {
  base::JSONValueConverter<RemoteMetricConfig> converter;
  RemoteMetricConfig config;
  converter.Convert(base::Value(std::move(dict)), &config);
  return config;
}

}  // namespace

TEST(P3AMetricConfigTest, CustomAttributesValidList) {
  auto dict = base::DictValue().Set(
      "custom_attributes", base::ListValue().Append("attr_a").Append("attr_b"));
  auto config = ConvertFromDict(std::move(dict));
  ASSERT_TRUE(config.custom_attributes.has_value());
  EXPECT_EQ((*config.custom_attributes)[0], "attr_a");
  EXPECT_EQ((*config.custom_attributes)[1], "attr_b");
}

TEST(P3AMetricConfigTest, CustomAttributesNonList) {
  // custom_attributes must be a list; a non-list value should leave the field
  // unset.
  auto dict = base::DictValue().Set("custom_attributes", "not_a_list");
  auto config = ConvertFromDict(std::move(dict));
  EXPECT_FALSE(config.custom_attributes.has_value());
}

TEST(P3AMetricConfigTest, CustomAttributesListWithNonStringElement) {
  // A list element that is not a string should cause parsing to fail and leave
  // the field unset.
  auto dict = base::DictValue().Set(
      "custom_attributes", base::ListValue().Append("valid_attr").Append(42));
  auto config = ConvertFromDict(std::move(dict));
  EXPECT_FALSE(config.custom_attributes.has_value());
}

}  // namespace p3a
