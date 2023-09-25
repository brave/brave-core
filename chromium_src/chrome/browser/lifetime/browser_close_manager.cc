/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/lifetime/browser_close_manager.h"

#define StartClosingBrowsers StartClosingBrowsers_ChromiumImpl
#define CancelBrowserClose CancelBrowserClose_ChromiumImpl

#include "src/chrome/browser/lifetime/browser_close_manager.cc"

#undef CancelBrowserClose
#undef StartClosingBrowsers

namespace {

// Whether the browser closing is in-progress or not.
// Introduced this new flag instead of using browser_shutdown::IsTryingToQuit()
// because it returns true when all browser windows are closed on Windows/Linux.
// I assume that the reason is background running on Windows/Linux.
bool g_browser_closing_started = false;

}  // namespace

// static
bool BrowserCloseManager::BrowserClosingStarted() {
  return g_browser_closing_started;
}

void BrowserCloseManager::StartClosingBrowsers() {
  g_browser_closing_started = true;
  StartClosingBrowsers_ChromiumImpl();
}

void BrowserCloseManager::CancelBrowserClose() {
  g_browser_closing_started = false;
  CancelBrowserClose_ChromiumImpl();
}
