/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/brave_dark_mode_utils.h"

namespace dark_mode {

void RegisterBraveDarkModeLocalStatePrefs(PrefRegistrySimple* registry) {
}

void RegisterBraveDarkModePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
}

bool SystemDarkModeEnabled() {
  return false;
}

void SetUseSystemDarkModeEnabledForTest(bool enabled) {
}

std::string GetStringFromBraveDarkModeType(BraveDarkModeType type) {
  return "Default";
}

void SetBraveDarkModeType(const std::string& type) {
}

void SetBraveDarkModeType(BraveDarkModeType type) {
}

BraveDarkModeType GetActiveBraveDarkModeType() {
  return BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT;
}

BraveDarkModeType GetBraveDarkModeType() {
  return BraveDarkModeType::BRAVE_DARK_MODE_TYPE_DEFAULT;
}

base::Value::List GetBraveDarkModeTypeList() {
  return base::Value::List();
}

void SetSystemDarkMode(BraveDarkModeType type) {
}

}  // namespace dark_mode
