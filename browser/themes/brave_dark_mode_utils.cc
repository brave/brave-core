/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "brave/browser/themes/brave_dark_mode_utils_internal.h"
#include "brave/components/constants/brave_switches.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/l10n/common/localization_util.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/channel_info.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/channel.h"
#include "ui/native_theme/native_theme.h"

namespace {

bool g_is_test_ = false;
bool g_system_dark_mode_enabled_in_test_ = false;

dark_mode::BraveDarkModeType GetDarkModeTypeBasedOnChannel() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
    case version_info::Channel::BETA:
      return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
    case version_info::Channel::DEV:
    case version_info::Channel::CANARY:
    case version_info::Channel::UNKNOWN:
    default:
      return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }
}

dark_mode::BraveDarkModeType GetDarkModeSwitchValue(
    const base::CommandLine& command_line) {
  DCHECK(command_line.HasSwitch(switches::kDarkMode));

  std::string requested_dark_mode_value =
      command_line.GetSwitchValueASCII(switches::kDarkMode);
  std::string requested_dark_mode_value_lower =
      base::ToLowerASCII(requested_dark_mode_value);
  DCHECK(requested_dark_mode_value_lower == "dark" ||
         requested_dark_mode_value == "light");

  if (requested_dark_mode_value_lower == "light") {
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }
  if (requested_dark_mode_value_lower == "dark") {
    return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }

  return dark_mode::BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
}

}  // namespace

namespace dark_mode {

void RegisterBraveDarkModeLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      kBraveDarkMode,
      static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT));
}

bool SystemDarkModeEnabled() {
  if (g_is_test_) {
    return g_system_dark_mode_enabled_in_test_;
  }

#if BUILDFLAG(IS_LINUX)
  return HasCachedSystemDarkModeType();
#else
  return ui::NativeTheme::GetInstanceForNativeUi()->SystemDarkModeSupported();
#endif
}

void SetUseSystemDarkModeEnabledForTest(bool enabled) {
  g_is_test_ = true;
  g_system_dark_mode_enabled_in_test_ = enabled;
}

std::string GetStringFromBraveDarkModeType(BraveDarkModeType type) {
  DCHECK_NE(type, BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT)
      << "Didn't expect to encounter the default theme mode here - this was "
         "previously a NOTREACHED";
  switch (type) {
    case BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT:
      return "Light";
    case BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK:
      return "Dark";
    default:
      return "Default";
  }
}

void SetBraveDarkModeType(const std::string& type) {
  BraveDarkModeType parsed_type =
      BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT;

  if (type == "Light") {
    parsed_type = BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  } else if (type == "Dark") {
    parsed_type = BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK;
  }
  SetBraveDarkModeType(parsed_type);
}

void SetBraveDarkModeType(BraveDarkModeType type) {
  g_browser_process->local_state()->SetInteger(kBraveDarkMode,
                                               static_cast<int>(type));
}

BraveDarkModeType GetActiveBraveDarkModeType() {
  // allow override via cli flag
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDarkMode)) {
    return GetDarkModeSwitchValue(command_line);
  }

  if (!g_browser_process || !g_browser_process->local_state()) {
    // In unittest, local_state() could not be initialzed.
    CHECK_IS_TEST();
    return BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }

  BraveDarkModeType type = static_cast<BraveDarkModeType>(
      g_browser_process->local_state()->GetInteger(kBraveDarkMode));
  if (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    if (!SystemDarkModeEnabled()) {
      return GetDarkModeTypeBasedOnChannel();
    }

    return ui::NativeTheme::GetInstanceForNativeUi()->ShouldUseDarkColors()
               ? BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK
               : BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }
  return type;
}

BraveDarkModeType GetBraveDarkModeType() {
  // allow override via cli flag
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kDarkMode)) {
    return GetDarkModeSwitchValue(command_line);
  }

  if (!g_browser_process || !g_browser_process->local_state()) {
    // In unittest, local_state() could not be initialzed.
    CHECK_IS_TEST();
    return BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT;
  }

  BraveDarkModeType type = static_cast<BraveDarkModeType>(
      g_browser_process->local_state()->GetInteger(kBraveDarkMode));
  if (type == BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT) {
    if (!SystemDarkModeEnabled()) {
      return GetDarkModeTypeBasedOnChannel();
    }
    return type;
  }
  return type;
}

base::Value::List GetBraveDarkModeTypeList() {
  base::Value::List list;

  if (SystemDarkModeEnabled()) {
    base::Value::Dict system_type;
    system_type.Set(
        "value",
        static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT));
    system_type.Set("name", brave_l10n::GetLocalizedResourceUTF16String(
                                IDS_BRAVE_THEME_TYPE_SYSTEM));
    list.Append(std::move(system_type));
  }

  base::Value::Dict dark_type;
  dark_type.Set("value",
                static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DARK));
  dark_type.Set("name", brave_l10n::GetLocalizedResourceUTF16String(
                            IDS_BRAVE_THEME_TYPE_DARK));
  list.Append(std::move(dark_type));

  base::Value::Dict light_type;
  light_type.Set(
      "value", static_cast<int>(BraveDarkModeType::BRAVE_DARK_MODE_TYPE_LIGHT));
  light_type.Set("name", brave_l10n::GetLocalizedResourceUTF16String(
                             IDS_BRAVE_THEME_TYPE_LIGHT));
  list.Append(std::move(light_type));

  return list;
}

}  // namespace dark_mode
