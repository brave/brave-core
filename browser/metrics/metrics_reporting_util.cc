/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/metrics/metrics_reporting_util.h"

#include "base/logging.h"
#include "chrome/common/channel_info.h"
#include "components/version_info/channel.h"

bool GetDefaultPrefValueForMetricsReporting() {
  switch (chrome::GetChannel()) {
    case version_info::Channel::STABLE:
      return false;
    case version_info::Channel::BETA:    // fall through
    case version_info::Channel::DEV:     // fall through
    case version_info::Channel::CANARY:
      return true;
    case version_info::Channel::UNKNOWN:
      return false;
    default:
      NOTREACHED();
      return false;
  }
}
