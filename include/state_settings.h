/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

#include "../include/json_bat_helper.h"

namespace ads_bat_client {

struct SETTINGS_STATE_ST {
  SETTINGS_STATE_ST();
  SETTINGS_STATE_ST(const SETTINGS_STATE_ST& state);
  ~SETTINGS_STATE_ST();

  bool LoadFromJson(const std::string& json);

  bool ads_enabled;
  std::string ads_amount_day;
  std::string ads_amount_hour;
};

}  // namespace ads_bat_client
