// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELDS_SETTINGS_VALUES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELDS_SETTINGS_VALUES_H_

#include <optional>

#include "base/types/cxx23_to_underlying.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace brave_shields {

enum ControlType {
  ALLOW = 0,
  BLOCK,
  BLOCK_THIRD_PARTY,
  DEFAULT,
};

namespace traits {

template <typename Setting>
struct SettingTraits;

template <>
struct SettingTraits<ControlType> {
  static std::optional<ControlType> From(
      std::underlying_type_t<ControlType> v) {
    if (v >= ControlType::ALLOW && v <= ControlType::DEFAULT) {
      return static_cast<ControlType>(v);
    }
    return std::nullopt;
  }

  static int To(ControlType setting) {
    CHECK(setting >= ControlType::ALLOW && setting <= ControlType::DEFAULT)
        << "Invalid setting.";
    return base::to_underlying(setting);
  }
};

template <>
struct SettingTraits<mojom::AutoShredMode> {
  static std::optional<mojom::AutoShredMode> From(
      std::underlying_type_t<mojom::AutoShredMode> v) {
    if (v >= static_cast<int>(mojom::AutoShredMode::NEVER) &&
        v <= static_cast<int>(mojom::AutoShredMode::kMaxValue)) {
      return static_cast<mojom::AutoShredMode>(v);
    }
    return std::nullopt;
  }

  static int To(mojom::AutoShredMode setting) {
    return base::to_underlying(setting);
  }
};

namespace internal {
const char* NotShieldContentTypeFailure();
}

consteval const char* GetShieldsContentTypeName(
    ContentSettingsType content_type) {
  // Linear, because `find` isn't constexpr/consteval.
  for (const auto& v : kShieldsContentTypeNames) {
    if (v.first == content_type) {
      return v.second;
    }
  }
  return internal::NotShieldContentTypeFailure();
}

template <ContentSettingsType content_settings_type, typename SettingT>
struct BraveShieldsSettingSettingType {
  static constexpr ContentSettingsType kContentSettingsType =
      content_settings_type;
  static constexpr const char* kName =
      GetShieldsContentTypeName(content_settings_type);

  using SettingType = SettingT;
  using SettingTraits = SettingTraits<SettingType>;
};

template <ContentSettingsType content_settings_type,
          typename SettingType,
          SettingType default_value>
struct BraveShieldsSetting
    : BraveShieldsSettingSettingType<content_settings_type, SettingType> {
  static constexpr SettingType kDefaultValue = default_value;

  static base::Value DefaultValue() { return ToValue(kDefaultValue); }

  static base::Value ToValue(SettingType setting) {
    return base::Value(base::Value::Dict().Set(
        BraveShieldsSetting::kName,
        traits::SettingTraits<SettingType>::To(setting)));
  }

  static SettingType FromValue(const base::Value& value) {
    if (const auto* dict = value.GetIfDict()) {
      if (const auto v = dict->FindInt(BraveShieldsSetting::kName);
          v.has_value()) {
        return traits::SettingTraits<SettingType>::From(*v).value_or(
            kDefaultValue);
      }
    }
    LOG(ERROR) << "ShieldSetting " << BraveShieldsSetting::kName
               << " failed to parse value: " << value.DebugString();
    DCHECK(false) << "Invalid value.";
    return kDefaultValue;
  }
};

}  // namespace traits

using CosmeticFilteringSetting = traits::BraveShieldsSetting<
    /*content_settings_type=*/ContentSettingsType::BRAVE_COSMETIC_FILTERING,
    /*SettingType=*/ControlType,
    /*default_value=*/ControlType::BLOCK_THIRD_PARTY>;

using AutoShredSetting = traits::BraveShieldsSetting<
    /*content_settings_type=*/ContentSettingsType::BRAVE_AUTO_SHRED,
    /*SettingType=*/mojom::AutoShredMode,
    /*default_value=*/mojom::AutoShredMode::NEVER>;

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELDS_SETTINGS_VALUES_H_
