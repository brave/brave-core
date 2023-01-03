/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/environment/environment_types_unittest_util.h"

#include <ostream>

#include "base/notreached.h"

namespace ads {

namespace {

constexpr char kProductionEnvironment[] = "Production";
constexpr char kStagingEnvironment[] = "Staging";
constexpr char kUnknownEnvironment[] = "Unknown";

}  // namespace

std::string EnvironmentTypeEnumToString(
    const EnvironmentType environment_type) {
  switch (environment_type) {
    case EnvironmentType::kProduction: {
      return kProductionEnvironment;
    }

    case EnvironmentType::kStaging: {
      return kStagingEnvironment;
    }
  }

  NOTREACHED() << "Unexpected value for EnvironmentType: "
               << static_cast<int>(environment_type);
  return kUnknownEnvironment;
}

}  // namespace ads
