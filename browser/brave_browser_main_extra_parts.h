/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_BROWSER_MAIN_EXTRA_PARTS_H_
#define BRAVE_BROWSER_BRAVE_BROWSER_MAIN_EXTRA_PARTS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_browser_main.h"
#include "chrome/browser/chrome_browser_main_extra_parts.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/views/frame/immersive_context_win.h"
#endif

class BraveBrowserMainExtraParts : public ChromeBrowserMainExtraParts {
 public:
  BraveBrowserMainExtraParts();
  BraveBrowserMainExtraParts(const BraveBrowserMainExtraParts&) = delete;
  BraveBrowserMainExtraParts& operator=(const BraveBrowserMainExtraParts&) =
      delete;
  ~BraveBrowserMainExtraParts() override;

  // ChromeBrowserMainExtraParts overrides.
  void PostBrowserStart() override;
  void PreMainMessageLoopRun() override;
  void PreProfileInit() override;

 private:
#if BUILDFLAG(IS_WIN)
  std::unique_ptr<ImmersiveContextWin> immersive_context_;
#endif
};

#endif  // BRAVE_BROWSER_BRAVE_BROWSER_MAIN_EXTRA_PARTS_H_
