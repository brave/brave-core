/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/metrics/enabled_state_provider.h"

#include "base/base_switches.h"
#include "base/command_line.h"

namespace metrics {

bool EnabledStateProvider::IsReportingEnabled() const {
  return false;
}

}  // namespace metrics
