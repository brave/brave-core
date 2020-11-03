/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_APP_BRAVE_MAIN_DELEGATE_H_
#define BRAVE_IOS_APP_BRAVE_MAIN_DELEGATE_H_

#include "base/macros.h"
#include "ios/chrome/app/startup/ios_chrome_main_delegate.h"

class BraveWebClient;

class BraveMainDelegate : public IOSChromeMainDelegate {
 public:
  BraveMainDelegate();
  ~BraveMainDelegate() override;

 protected:
  // web::WebMainDelegate implementation:
  void BasicStartupComplete() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveMainDelegate);
};

#endif  // BRAVE_IOS_APP_BRAVE_MAIN_DELEGATE_H_
