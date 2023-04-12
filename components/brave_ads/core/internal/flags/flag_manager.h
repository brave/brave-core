/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_FLAG_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_FLAG_MANAGER_H_

#include "brave/components/brave_ads/core/internal/flags/environment/environment_types.h"

namespace brave_ads {

class FlagManager final {
 public:
  FlagManager();

  FlagManager(const FlagManager&) = delete;
  FlagManager& operator=(const FlagManager&) = delete;

  FlagManager(FlagManager&&) noexcept = delete;
  FlagManager& operator=(FlagManager&&) noexcept = delete;

  ~FlagManager();

  static FlagManager* GetInstance();

  static bool HasInstance();

  bool ShouldDebug() const { return should_debug_; }
  void SetShouldDebugForTesting(const bool should_debug) {
    should_debug_ = should_debug;
  }

  bool DidOverrideFromCommandLine() const {
    return did_override_from_command_line_;
  }
  void SetDidOverrideFromCommandLineForTesting(
      const bool did_override_from_command_line) {
    did_override_from_command_line_ = did_override_from_command_line;
  }

  EnvironmentType GetEnvironmentType() const { return environment_type_; }
  void SetEnvironmentTypeForTesting(const EnvironmentType environment_type) {
    environment_type_ = environment_type;
  }

 private:
  void Initialize();

  bool should_debug_ = false;

  bool did_override_from_command_line_ = false;

  EnvironmentType environment_type_ = EnvironmentType::kStaging;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_FLAGS_FLAG_MANAGER_H_
