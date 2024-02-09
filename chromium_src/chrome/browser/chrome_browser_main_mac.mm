/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/mac/keystone_glue.h"
#include "chrome/browser/updater/browser_updater_client_util.h"

#define EnsureUpdater                                        \
  [[KeystoneGlue defaultKeystoneGlue] registerWithKeystone]; \
  if (false)                                                 \
  EnsureUpdater

#include "src/chrome/browser/chrome_browser_main_mac.mm"

#undef EnsureUpdater
