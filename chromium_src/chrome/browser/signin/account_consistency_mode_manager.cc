/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/signin/account_consistency_mode_manager.h"

#if defined(OS_ANDROID)
#define BRAVE_COMPUTE_ACCOUNT_CONSISTENCY_METHOD \
  return AccountConsistencyMethod::kDisabled;
#else
#define BRAVE_COMPUTE_ACCOUNT_CONSISTENCY_METHOD
#endif

#include "src/chrome/browser/signin/account_consistency_mode_manager.cc"

#undef BRAVE_COMPUTE_ACCOUNT_CONSISTENCY_METHOD
