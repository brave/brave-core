// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/common/brave_shields_settings_values.h"

#include "base/check.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

using ShieldsSettingsValuesTest = ::testing::Test;

template <ContentSettingsType content_settings_type,
          typename SettingT,
          SettingT... values>
struct ValuesChecker {
  template <SettingT value>
  bool SuccessCheckValue(
      base::FunctionRef<bool(const base::Value&)> structure_checker) {
    using TypeToCheck =
        typename traits::BraveShieldsSetting<content_settings_type, SettingT,
                                             value>;
    if (TypeToCheck::kDefaultValue != value) {
      return false;
    }
    if (TypeToCheck::ToValue(value) != TypeToCheck::DefaultValue()) {
      return false;
    }
    if (TypeToCheck::FromValue(TypeToCheck::DefaultValue()) !=
        TypeToCheck::kDefaultValue) {
      return false;
    }
    if (TypeToCheck::FromValue(TypeToCheck::ToValue(value)) != value) {
      return false;
    }
    if (!structure_checker(TypeToCheck::ToValue(value))) {
      return false;
    }
    return true;
  }

  bool SuccessCheckValues(
      base::FunctionRef<bool(const base::Value&)> structure_checker) {
    return (SuccessCheckValue<values>(structure_checker) && ...);
  }
};

template <ContentSettingsType... content_settings_types>
struct ControlTypeShieldsSettingsValuesChecker {
  static_assert(kShieldsContentTypeNames.size() ==
                sizeof...(content_settings_types));

  template <ContentSettingsType content_settings_type>
  bool SuccessCheckOne() {
    ValuesChecker<content_settings_type, ControlType, ControlType::DEFAULT,
                  ControlType::ALLOW, ControlType::BLOCK,
                  ControlType::BLOCK_THIRD_PARTY>
        checker;
    return checker.SuccessCheckValues([](const base::Value& value) {
      return value.is_dict() &&
             value.GetDict().FindInt(
                 traits::GetShieldsContentTypeName(content_settings_type));
    });
  }

  bool SuccessCheck() {
    return (SuccessCheckOne<content_settings_types>() && ...);
  }
};

using ControlTypeAllShieldsSettingsValuesChecker =
    ControlTypeShieldsSettingsValuesChecker<
        ContentSettingsType::BRAVE_ADS,
        ContentSettingsType::BRAVE_COSMETIC_FILTERING,
        ContentSettingsType::BRAVE_TRACKERS,
        ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
        ContentSettingsType::BRAVE_HTTPS_UPGRADE,
        ContentSettingsType::JAVASCRIPT,
        ContentSettingsType::BRAVE_FINGERPRINTING_V2,
        ContentSettingsType::BRAVE_SHIELDS,
        ContentSettingsType::BRAVE_SHIELDS_METADATA,
        ContentSettingsType::BRAVE_REFERRERS,
        ContentSettingsType::BRAVE_COOKIES,
        ContentSettingsType::BRAVE_AUTO_SHRED>;

TEST_F(ShieldsSettingsValuesTest, ControlTypeSettingTypeSuccess) {
  ControlTypeAllShieldsSettingsValuesChecker checker;
  EXPECT_TRUE(checker.SuccessCheck());
}

#if !BUILDFLAG(IS_IOS)  // iOS doesn't support EXPECT_DEATH
TEST_F(ShieldsSettingsValuesTest, ControlTypeSettingTypeFailure) {
  using ControlTypeTrait = traits::SettingTraits<ControlType>;
  EXPECT_DEATH(ControlTypeTrait::To(static_cast<ControlType>(-1)), "");

  EXPECT_EQ(std::nullopt, ControlTypeTrait::From(-1));

  using Setting =
      traits::BraveShieldsSetting<ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                                  ControlType, ControlType::BLOCK_THIRD_PARTY>;

  // Structure is invalid.
  if constexpr (DCHECK_IS_ON()) {
    EXPECT_DEATH(Setting::FromValue(base::Value()), "");
  } else {
    EXPECT_EQ(Setting::kDefaultValue, Setting::FromValue(base::Value()));
  }

  // Structure is ok, but contains an invalid value.
  EXPECT_EQ(Setting::kDefaultValue,
            Setting::FromValue(base::Value(base::DictValue().Set(
                traits::GetShieldsContentTypeName(
                    ContentSettingsType::BRAVE_COSMETIC_FILTERING),
                -1))));
}
#endif

}  // namespace brave_shields
