/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>

namespace state {

struct SETTINGS_STATE {
  SETTINGS_STATE();
  explicit SETTINGS_STATE(const SETTINGS_STATE& state);
  ~SETTINGS_STATE();

  bool LoadFromJson(const std::string& json);

  bool ads_enabled;
  std::string ads_locale;
  std::string ads_per_hour;
  std::string ads_per_day;
};

}  // namespace state
