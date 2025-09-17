/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"

#if BUILDFLAG(IS_MAC)
// When the current build is more than several weeks old, upstream takes this as
// a sign that automatic updates are broken and shows a prominent "Can't update
// - please reinstall" notification. This makes sense for upstream, which has
// background updates on macOS. But we do not have background updates on macOS
// yet - see github.com/brave/brave-browser/issues/45086. Under these
// circumstances, the "reinstall" prompt is very confusing, especially because
// it is likely that Brave is just downloading an update in the background. To
// work around this, we disable outdated build detection until we also have
// background updates on macOS.
#define BRAVE_UPGRADE_DETECTOR_IMPL_START_OUTDATED_BUILD_DETECTOR return;
#else
#define BRAVE_UPGRADE_DETECTOR_IMPL_START_OUTDATED_BUILD_DETECTOR
#endif  // BUILDFLAG(IS_MAC)

#include <chrome/browser/upgrade_detector/upgrade_detector_impl.cc>

#undef BRAVE_UPGRADE_DETECTOR_IMPL_START_OUTDATED_BUILD_DETECTOR
