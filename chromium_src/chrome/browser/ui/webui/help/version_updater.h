/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_H_

#include "brave/browser/sparkle_buildflags.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#define PromoteUpdater                          \
  GetIsSparkleForTesting(bool& result) const {} \
  virtual void PromoteUpdater
#endif

#include "src/chrome/browser/ui/webui/help/version_updater.h"  // IWYU pragma: export

#if BUILDFLAG(ENABLE_SPARKLE)
#undef PromoteUpdater
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_H_
