/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/task/thread_pool/thread_pool_instance.h"
#include "brave/test/base/brave_testing_profile.cc"

// Brave creates additional browser context keyed services during profile
// initialization that may trigger storage partition creation and background
// database tasks. When a test destroys and recreates a TestingProfile at the
// same path, these background tasks (e.g., SharedDictionary SQLite cleanup) may
// still hold file locks on Windows, causing the new profile's database
// initialization to FATAL crash. Flush the thread pool after shutting down
// storage partitions to ensure all background database tasks complete before
// the profile directory is released.
// https://github.com/brave/brave-browser/issues/52777
#define ShutdownStoragePartitions()                   \
  ShutdownStoragePartitions();                        \
  if (auto* pool = base::ThreadPoolInstance::Get()) { \
    pool->FlushForTesting();                          \
  }                                                   \
  (void)0

#include <chrome/test/base/testing_profile.cc>

#undef ShutdownStoragePartitions
