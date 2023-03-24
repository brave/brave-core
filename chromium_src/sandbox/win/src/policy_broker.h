/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_POLICY_BROKER_H_
#define BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_POLICY_BROKER_H_

namespace sandbox {
class TargetConfig;
}

#define SetupBasicInterceptions(...)                 \
  SetupBasicInterceptions_ChromiumImpl(__VA_ARGS__); \
  bool SetupBasicInterceptions(__VA_ARGS__, const TargetConfig* config)

#include "src/sandbox/win/src/policy_broker.h"  // IWYU pragma: export

#undef SetupBasicInterceptions

#endif  // BRAVE_CHROMIUM_SRC_SANDBOX_WIN_SRC_POLICY_BROKER_H_
