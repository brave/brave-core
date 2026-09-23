/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/task/thread_pool/thread_pool_instance.h"
// Included before the macro below so that `Builder::SetProfileName()` is
// declared while it still means what it says.
#include "chrome/test/base/testing_profile.h"

// This fixture derives the profile directory from the profile name so that a
// profile can be unloaded and recreated at the same path. Brave adds browser
// context keyed services that keep background database tasks (e.g.
// SharedDictionary SQLite cleanup) running past the destruction of the previous
// profile, and on Windows those tasks still hold file locks in the profile
// directory, which makes the new profile's database initialization fail
// fatally. Flush the thread pool before building the profile so the previous
// one's teardown has fully completed.
// https://github.com/brave/brave-browser/issues/52777
#define SetProfileName(name)                          \
  SetProfileName(name);                               \
  if (auto* pool = base::ThreadPoolInstance::Get()) { \
    pool->FlushForTesting();                          \
  }                                                   \
  (void)0

// The compiler doesn't realize that we're only linking against the chromium_src
// override object file, so it's ok for object duplication to occur here.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunique-object-duplication"

#include <chrome/browser/media/webrtc/webrtc_event_log_manager_unittest.cc>

#pragma clang diagnostic pop

#undef SetProfileName
