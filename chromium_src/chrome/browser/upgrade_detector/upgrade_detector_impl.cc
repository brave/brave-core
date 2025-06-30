/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/updater/buildflags.h"

#if BUILDFLAG(ENABLE_OMAHA4)
// When the current build is more than several weeks old, upstream takes this as
// a sign that automatic updates are broken and shows a prominent "Can't update
// - please reinstall" notification. This makes sense for upstream, which uses
// Omaha 4 with background updates on macOS. But we still use Sparkle, which
// only updates while the browser is running and requires a relaunch to install
// new versions. In this case, the "reinstall" prompt is very confusing,
// especially because it is likely that Brave is just downloading an update in
// the background. To work around this, we disable outdated build detection
// until we also have background updates on macOS.
#include "brave/browser/updater/features.h"

#define BRAVE_UPGRADE_DETECTOR_IMPL_START_OUTDATED_BUILD_DETECTOR \
  if (!brave_updater::ShouldUseOmaha4()) {                        \
    return;                                                       \
  }
#else
#define BRAVE_UPGRADE_DETECTOR_IMPL_START_OUTDATED_BUILD_DETECTOR
#endif  // BUILDFLAG(ENABLE_OMAHA4)

#include "src/chrome/browser/upgrade_detector/upgrade_detector_impl.cc"

#undef BRAVE_UPGRADE_DETECTOR_IMPL_START_OUTDATED_BUILD_DETECTOR
