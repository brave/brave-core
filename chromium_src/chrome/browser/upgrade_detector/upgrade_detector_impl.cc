/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "chrome/browser/buildflags.h"

#if BUILDFLAG(IS_MAC)
// When the current build is more than several weeks old, upstream takes this as
// a sign that automatic updates are broken and shows a prominent "Can't update
// - please reinstall" notification. This makes sense for upstream, which uses
// Omaha 4 with background updates on macOS. But we still use Sparkle, which
// only updates while the browser is running and requires a relaunch to install
// new versions. In this case, the "reinstall" prompt is very confusing,
// especially because it is likely that Brave is just downloading an update in
// the background. To work around this until we also have background updates on
// macOS, we disable the outdated build detection feature. The easiest way to do
// this here is as follows:
#define FEATURE_ENABLED_BY_DEFAULT FEATURE_DISABLED_BY_DEFAULT
#endif

#include "src/chrome/browser/upgrade_detector/upgrade_detector_impl.cc"

#if BUILDFLAG(IS_MAC)
#undef FEATURE_ENABLED_BY_DEFAULT
#endif
