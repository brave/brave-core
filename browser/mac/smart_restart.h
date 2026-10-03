/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_MAC_SMART_RESTART_H_
#define BRAVE_BROWSER_MAC_SMART_RESTART_H_

#include <optional>

// Returns true while Sparkle is the updater. Sparkle replaces the whole app
// bundle when it installs an update on quit, so a SmartRestart relaunch can
// start the old version while its Versions folder is being removed.
// SmartRestartPolicy blocks both SmartRestart triggers while this is true.
bool BraveIsSparkleTheUpdater();

// Makes BraveIsSparkleTheUpdater() return `sparkle_is_updater` in tests;
// std::nullopt restores the real check.
void SetSparkleIsUpdaterForTesting(std::optional<bool> sparkle_is_updater);

#endif  // BRAVE_BROWSER_MAC_SMART_RESTART_H_
