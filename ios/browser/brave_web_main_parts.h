/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WEB_MAIN_PARTS_H_
#define BRAVE_IOS_BROWSER_BRAVE_WEB_MAIN_PARTS_H_

#include <memory>

#include "base/macros.h"
#include "ios/chrome/browser/ios_chrome_field_trials.h"
#include "ios/web/public/init/web_main_parts.h"

class ApplicationContextImpl;
class PrefService;

namespace base {
class FieldTrialList;
}

class BraveWebMainParts : public web::WebMainParts {
 public:
  BraveWebMainParts();
  ~BraveWebMainParts() override;

 private:
  // web::WebMainParts implementation.
  void PreEarlyInitialization() override;
  void PreMainMessageLoopStart() override;
  void PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;
  void SetupFieldTrials();

  std::unique_ptr<ApplicationContextImpl> application_context_;

  // Statistical testing infrastructure for the entire browser. NULL until
  // SetUpMetricsAndFieldTrials is called.
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  PrefService* local_state_;

  IOSChromeFieldTrials ios_field_trials_;

  DISALLOW_COPY_AND_ASSIGN(BraveWebMainParts);
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_WEB_MAIN_PARTS_H_
