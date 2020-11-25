/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/app/brave_main_delegate.h"

#import "brave/ios/browser/brave_web_client.h"
#import "brave/ios/browser/chrome_paths.h"

#include "base/logging.h"
#include "components/component_updater/component_updater_paths.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

BraveMainDelegate::BraveMainDelegate() {
}

BraveMainDelegate::~BraveMainDelegate() {
}

void BraveMainDelegate::BasicStartupComplete() {

    // Initialize the Chrome path provider.
    ios::RegisterPathProvider();

    // Register the component updater path provider.
    // Bundled components are not supported on ios, so DIR_USER_DATA is passed
    // for all three arguments.
    component_updater::RegisterPathProvider(
        ios::DIR_USER_DATA, ios::DIR_USER_DATA, ios::DIR_USER_DATA);

    // Upstream wires up log file handling here based on flags; for now that's
    // not supported, and this is called just to handle vlog levels and patterns.
    // If redirecting to a file is ever needed, add it here (see
    // logging_chrome.cc for example code).
    logging::LoggingSettings log_settings;
    logging::InitLogging(log_settings);
    
  web_client_.reset(new BraveWebClient());
  web::SetWebClient(web_client_.get());
}
