// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_WEB_BRAVE_WEB_MAIN_PARTS_H_
#define BRAVE_IOS_BROWSER_WEB_BRAVE_WEB_MAIN_PARTS_H_

#include "ios/chrome/browser/web/model/chrome_main_parts.h"

class BraveWebMainParts : public IOSChromeMainParts {
 public:
  explicit BraveWebMainParts(const base::CommandLine& parsed_command_line);
  BraveWebMainParts(const BraveWebMainParts&) = delete;
  BraveWebMainParts& operator=(const BraveWebMainParts&) = delete;
  ~BraveWebMainParts() override;

 private:
  // web::WebMainParts implementation.
  void PreCreateMainMessageLoop() override;
  void PreMainMessageLoopRun() override;
};

#endif  // BRAVE_IOS_BROWSER_WEB_BRAVE_WEB_MAIN_PARTS_H_
