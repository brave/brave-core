/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/confirmations/internal/time.h"

#include "base/time/time.h"

namespace confirmations {

uint64_t Time::NowInSeconds() {
  auto now = base::Time::Now();
  return static_cast<uint64_t>((now - base::Time()).InSeconds());
}

}  // namespace confirmations
