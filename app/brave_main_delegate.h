// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
#define BRAVE_APP_BRAVE_MAIN_DELEGATE_H_

#include "chrome/app/chrome_main_delegate.h"

// Chrome implementation of ContentMainDelegate.
class BraveMainDelegate : public ChromeMainDelegate {
 public:
  BraveMainDelegate();

  // |exe_entry_point_ticks| is the time at which the main function of the
  // executable was entered, or null if not available.
  explicit BraveMainDelegate(base::TimeTicks exe_entry_point_ticks);
  ~BraveMainDelegate() override;

 protected:
  // content::ContentMainDelegate implementation:
  bool ShouldEnableProfilerRecording() override;

  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;
  content::ContentUtilityClient* CreateContentUtilityClient() override;

  DISALLOW_COPY_AND_ASSIGN(BraveMainDelegate);
};

#endif  // BRAVE_APP_BRAVE_MAIN_DELEGATE_H_
