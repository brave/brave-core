/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/updater/buildflags.h"

#if BUILDFLAG(BRAVE_ENABLE_UPDATER)
#include "src/chrome/browser/ui/webui/help/version_updater_mac.mm"
#else
#include "chrome/browser/ui/webui/help/version_updater_mac_sparkle.mm"
#endif
