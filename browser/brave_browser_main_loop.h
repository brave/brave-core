/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_MAIN_LOOP_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_MAIN_LOOP_H_

#include "content/browser/browser_main_loop.h"

namespace brave {

class BraveBrowserMainLoop : public content::BrowserMainLoop {
 public:
  using BrowserMainLoop::BrowserMainLoop;

  BraveBrowserMainLoop(const BraveBrowserMainLoop&) = delete;
  BraveBrowserMainLoop& operator=(const BraveBrowserMainLoop&) = delete;
  ~BraveBrowserMainLoop() override = default;

  void PreShutdown() override;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_MAIN_LOOP_H_
