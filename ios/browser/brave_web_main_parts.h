/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_WEB_MAIN_PARTS_H_
#define BRAVE_IOS_BROWSER_BRAVE_WEB_MAIN_PARTS_H_

#include <memory>

#include "ios/chrome/browser/flags/ios_chrome_field_trials.h"
#include "ios/web/public/init/web_main_parts.h"

class BraveApplicationContextImpl;
class PrefService;

namespace base {
class FieldTrialList;
}

class BraveWebMainParts : public web::WebMainParts {
 public:
  BraveWebMainParts();
  BraveWebMainParts(const BraveWebMainParts&) = delete;
  BraveWebMainParts& operator=(const BraveWebMainParts&) = delete;
  ~BraveWebMainParts() override;

 private:
  void SetupMetrics();
  // web::WebMainParts implementation.
  void PreCreateMainMessageLoop() override;
  void PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  void PostMainMessageLoopRun() override;
  void PostDestroyThreads() override;
  void PostCreateThreads() override;
  void SetupFieldTrials();

  std::unique_ptr<BraveApplicationContextImpl> application_context_;

  // Statistical testing infrastructure for the entire browser. NULL until
  // SetUpMetricsAndFieldTrials is called.
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  PrefService* local_state_;

  IOSChromeFieldTrials ios_field_trials_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_WEB_MAIN_PARTS_H_
