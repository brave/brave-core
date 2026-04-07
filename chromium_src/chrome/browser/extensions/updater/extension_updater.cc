// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/feature_list.h"

// Forward declaration; defined in brave/browser/extensions/updater/features.cc
// and linked at build time from //brave/browser/extensions:extensions.
namespace extensions::features {
BASE_DECLARE_FEATURE(kBraveAutoUpdateExtensions);
}  // namespace extensions::features

// Guards scheduled extension update checks behind the kBraveAutoUpdateExtensions
// feature flag. Injected at two call sites in extension_updater.cc via patch:
//   1. ExtensionUpdater::Start() - skips the startup check and periodic
//      scheduling when auto-updates are disabled.
//   2. ExtensionUpdater::NextCheck() - skips individual scheduled update
//      checks and their rescheduling when auto-updates are disabled.
// User-triggered CheckNow() calls (e.g. the Update button in
// chrome://extensions) are unaffected and continue to work normally.
#define BRAVE_EXTENSION_UPDATER_SCHEDULED_CHECK_GUARD        \
  if (!base::FeatureList::IsEnabled(                         \
          extensions::features::kBraveAutoUpdateExtensions)) \
    return;

#include <chrome/browser/extensions/updater/extension_updater.cc>

#undef BRAVE_EXTENSION_UPDATER_SCHEDULED_CHECK_GUARD
