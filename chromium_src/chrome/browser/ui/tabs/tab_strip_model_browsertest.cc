/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/dcheck_is_on.h"

// In official builds without DCHECKs, CHECK() discards its log message
// (base/check.h CHECK_WILL_STREAM is false). The death test for
// TestCloseTabDuringMoveOperation expects "Check failed" in the subprocess
// stderr, which never appears, causing the regex match to fail.
// We disable it by pre-defining the token that the upstream MAYBE_ macro
// resolves to on non-ChromeOS builds so the two-step expansion produces
// DISABLED_TestCloseTabDuringMoveOperation instead.
#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
#define TestCloseTabDuringMoveOperation DISABLED_TestCloseTabDuringMoveOperation
#endif

#include <chrome/browser/ui/tabs/tab_strip_model_browsertest.cc>  // IWYU pragma: export

#if defined(OFFICIAL_BUILD) && !DCHECK_IS_ON()
#undef TestCloseTabDuringMoveOperation
#endif
