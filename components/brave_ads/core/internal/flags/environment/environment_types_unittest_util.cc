/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/environment/environment_types_unittest_util.h"

namespace brave_ads::test {

namespace {

constexpr char kProductionEnvironment[] = "Production";
constexpr char kStagingEnvironment[] = "Staging";

}  // namespace

std::string ToString(const mojom::EnvironmentType environment_type) {
  switch (environment_type) {
    case mojom::EnvironmentType::kProduction: {
      return kProductionEnvironment;
    }

    case mojom::EnvironmentType::kStaging: {
      return kStagingEnvironment;
    }
  }
}

}  // namespace brave_ads::test
