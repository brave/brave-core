/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_FIRST_RUN_FIRST_RUN_H_
#define BRAVE_BROWSER_FIRST_RUN_FIRST_RUN_H_

#include "base/version_info/channel.h"

namespace brave::first_run {

bool IsMetricsReportingOptIn(version_info::Channel channel);
bool IsMetricsReportingOptIn();

}  // namespace brave::first_run

#endif  // BRAVE_BROWSER_FIRST_RUN_FIRST_RUN_H_
