// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_transfer/browser/test/run_until.h"

namespace brave_transfer {

namespace {

constexpr base::TimeDelta kStep = base::Milliseconds(100);

}  // namespace

bool RunUntilWithMockTime(base::test::TaskEnvironment& task_environment,
                          base::FunctionRef<bool()> condition,
                          base::TimeDelta max_time) {
  base::TimeDelta elapsed;
  while (true) {
    task_environment.RunUntilIdle();
    if (condition()) {
      return true;
    }
    if (elapsed >= max_time) {
      return false;
    }
    task_environment.FastForwardBy(kStep);
    elapsed += kStep;
  }
}

}  // namespace brave_transfer
