/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_H_

#include "base/macros.h"
#include "chrome/browser/chrome_browser_main.h"

class BraveBrowserMainParts : public ChromeBrowserMainParts {
 public:
  using ChromeBrowserMainParts::ChromeBrowserMainParts;
  ~BraveBrowserMainParts() override = default;

  // ChromeBrowserMainParts overrides:
  // Marking this method final so that if any of the OS specific subclasses
  // (e.g. ChromeBrowserMainPartsWin) decides to override this method in the
  // future we would get a compilation error.
  void PreShutdown() final;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveBrowserMainParts);
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_MAIN_PARTS_H_
