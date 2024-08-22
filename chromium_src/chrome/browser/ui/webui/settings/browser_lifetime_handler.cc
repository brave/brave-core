/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/settings/browser_lifetime_handler.h"

#include "brave/browser/sparkle_buildflags.h"
#include "brave/updater/buildflags.h"

#if BUILDFLAG(ENABLE_SPARKLE)
#include "brave/browser/ui/webui/settings/brave_relaunch_handler_mac.h"
#endif

#define BrowserLifetimeHandler BrowserLifetimeHandler_ChromiumImpl
#include "src/chrome/browser/ui/webui/settings/browser_lifetime_handler.cc"
#undef BrowserLifetimeHandler

namespace settings {

BrowserLifetimeHandler::~BrowserLifetimeHandler() {}

void BrowserLifetimeHandler::HandleRelaunch(const base::Value::List& args) {
#if BUILDFLAG(ENABLE_SPARKLE) && !BUILDFLAG(BRAVE_ENABLE_UPDATER)
  if (brave_relaunch_handler::RelaunchOnMac()) {
    return;
  }
#endif
  BrowserLifetimeHandler_ChromiumImpl::HandleRelaunch(args);
}

}  // namespace settings
