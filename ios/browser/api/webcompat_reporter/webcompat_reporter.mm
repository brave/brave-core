// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter.h"

#include "brave/components/webcompat_reporter/browser/webcompat_reporter_service.h"
#include "ios/chrome/browser/shared/model/browser_state/chrome_browser_state.h"
#include "brave/ios/browser/api/webcompat_reporter/webcompat_reporter_service_factory.h"


@interface WebcompatReporterAPI () {
  raw_ptr<ChromeBrowserState> browser_state_;
  raw_ptr<webcompat_reporter::WebcompatReporterService> service_;
}
@end

@implementation WebcompatReporterAPI
- (instancetype)initWithChromeBrowserState:(ChromeBrowserState*)browserState {
  if ((self = [super init])) {
    browser_state_ = browserState;

    service_ =
        webcompat_reporter::WebcompatReporterServiceFactory::GetForBrowserState(browser_state_);
  }
  return self;
}

- (void)dealloc {
}

- (void)submitReport {//:(WebcompatReporterReportInfo*)reportInfo {
 //service_->SubmitWebcompatReport(reportInfo);
}
@end