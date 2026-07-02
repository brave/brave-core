// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELDS_SETTINGS_VALUES_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CORE_COMMON_BRAVE_SHIELDS_SETTINGS_VALUES_H_

#include <optional>
#include <string>
#include <utility>

#include "base/logging.h"
#include "base/notreached.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/common/brave_shield_constants.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

namespace brave_shields {

// List of possible blocking modes when accessing blocked websites.
enum class DomainBlockingType {
  // Don't block a website, open as is.
  kNone,
  // Proceed to a website, but use Ephemeral Storage for privacy-sensitive data
  // (cookies, etc.).
  k1PES,
  // Show an interstitial before proceeding to as website.
  kAggressive,
};

enum ControlType {
  ALLOW = 0,
  BLOCK,
  BLOCK_THIRD_PARTY,
  DEFAULT,
};

inline std::string ControlTypeToString(ControlType type) {
  switch (type) {
    case ControlType::ALLOW:
      return "allow";
    case ControlType::BLOCK:
      return "block";
    case ControlType::BLOCK_THIRD_PARTY:
      return "block_third_party";
    case ControlType::DEFAULT:
      return "default";
  }
  NOTREACHED() << "Unexpected value for ControlType: "
               << std::to_underlying(type);
}

inline ControlType ControlTypeFromString(const std::string& value) {
  if (value == "allow") {
    return ControlType::ALLOW;
  } else if (value == "block") {
    return ControlType::BLOCK;
  } else if (value == "block_third_party") {
    return ControlType::BLOCK_THIRD_PARTY;
  } else if (value == "default") {
    return ControlType::DEFAULT;
  }
  NOTREACHED();
}

inline ContentSetting GetDefaultBlockFromControlType(ControlType type) {
  if (type == ControlType::DEFAULT) {
    return CONTENT_SETTING_DEFAULT;
  }

  return type == ControlType::ALLOW ? CONTENT_SETTING_ALLOW
                                    : CONTENT_SETTING_BLOCK;
}

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
    return std::to_underlying(setting);
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
    return std::to_underlying(setting);
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
    return base::Value(
        base::DictValue().Set(BraveShieldsSetting::kName,
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
