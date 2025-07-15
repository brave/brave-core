/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_MAIN_EXTRA_PARTS_P3A_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_MAIN_EXTRA_PARTS_P3A_H_

#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chrome_browser_main.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"

class BraveBrowserMainExtraPartsP3A : public ChromeBrowserMainExtraParts {
 public:
  BraveBrowserMainExtraPartsP3A();
  ~BraveBrowserMainExtraPartsP3A() override;

  BraveBrowserMainExtraPartsP3A(const BraveBrowserMainExtraPartsP3A&) = delete;
  BraveBrowserMainExtraPartsP3A& operator=(
      const BraveBrowserMainExtraPartsP3A&) = delete;

  // ChromeBrowserMainExtraParts override:
  void PostBrowserStart() override;
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_MAIN_EXTRA_PARTS_P3A_H_
