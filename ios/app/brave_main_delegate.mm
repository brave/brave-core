/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/app/brave_main_delegate.h"

#import "brave/ios/browser/brave_web_client.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveMainDelegate::BraveMainDelegate() {
}

BraveMainDelegate::~BraveMainDelegate() {
}

void BraveMainDelegate::BasicStartupComplete() {
  IOSChromeMainDelegate::BasicStartupComplete();

  web_client_.reset(new BraveWebClient());
  web::SetWebClient(web_client_.get());
}
