/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_loop.h"

#include "brave/browser/brave_browser_main_parts.h"

namespace brave {

void BraveBrowserMainLoop::PreShutdown() {
  parts()->PreShutdown();
  content::BrowserMainLoop::PreShutdown();
}

}  // namespace brave
