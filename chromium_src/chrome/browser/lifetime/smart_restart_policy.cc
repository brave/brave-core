/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
namespace keystone_glue {
// Implemented in brave/browser/mac/keystone_glue.mm.
bool BraveIsSparkleTheUpdater();
}  // namespace keystone_glue
#endif  // BUILDFLAG(IS_MAC)

namespace {

// Called at the top of both SmartRestart checks by
// rewrite/chrome/browser/lifetime/smart_restart_policy.cc.yaml.
bool IsSmartRestartBlockedByBrave() {
#if BUILDFLAG(IS_MAC)
  return keystone_glue::BraveIsSparkleTheUpdater();
#else
  return false;
#endif  // BUILDFLAG(IS_MAC)
}

}  // namespace

#include <chrome/browser/lifetime/smart_restart_policy.cc>
