/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
#define BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class PrefRegistrySimple;
class PrefService;

namespace speedreader {

class SpeedreaderService : public KeyedService {
 public:
  explicit SpeedreaderService(PrefService* prefs);
  ~SpeedreaderService() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void ToggleSpeedreader();
  void DisableSpeedreaderForTest();
  bool IsEnabled();
  bool ShouldPromptUserToEnable() const;
  void IncrementPromptCount();
  void SelectedTheme(const std::string& theme_name);
  std::string GetSelectedTheme() const;

  SpeedreaderService(const SpeedreaderService&) = delete;
  SpeedreaderService& operator=(const SpeedreaderService&) = delete;

 private:
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace speedreader

#endif  // BRAVE_COMPONENTS_SPEEDREADER_SPEEDREADER_SERVICE_H_
