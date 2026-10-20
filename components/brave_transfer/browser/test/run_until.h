// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_RUN_UNTIL_H_
#define BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_RUN_UNTIL_H_

#include "base/functional/function_ref.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"

namespace brave_transfer {

// Like base::test::RunUntil(), for MOCK_TIME task environments with repeating
// timers. base::test::RunUntil() only checks `condition` when the thread is
// idle, which never happens while a repeating timer lets mock time advance.
// Runs until idle without advancing time, then advances time in small steps.
// Returns false if `condition` is still false after `max_time`.
bool RunUntilWithMockTime(base::test::TaskEnvironment& task_environment,
                          base::FunctionRef<bool()> condition,
                          base::TimeDelta max_time = base::Minutes(5));

}  // namespace brave_transfer

#endif  // BRAVE_COMPONENTS_BRAVE_TRANSFER_BROWSER_TEST_RUN_UNTIL_H_
