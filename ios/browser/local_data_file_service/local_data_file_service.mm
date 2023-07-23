/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <Foundation/Foundation.h>
#include <sys/qos.h>
#include <string>
#include "url/gurl.h"

#include "brave/components/brave_component_updater/browser/local_data_files_service.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_component_installer.h"
#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"
#include "brave/ios/browser/local_data_file_service/local_data_file_service+private.h"
#include "brave/ios/browser/local_data_file_service/local_data_file_service_installer_delegate.h"
#import "net/base/mac/url_conversions.h"

using brave::URLSanitizerComponentInstaller;
using brave::URLSanitizerService;
using brave_component_updater::LocalDataFilesService;
using brave_component_updater::LocalDataFilesServiceFactory;
using local_data_file_service::LocalDataFileServiceDelegate;

// LocalDataFileService
@interface LocalDataFileService () {
  std::unique_ptr<LocalDataFileServiceDelegate> delegate_;
  std::unique_ptr<LocalDataFilesService> fileService_;
  std::unique_ptr<URLSanitizerComponentInstaller>
      urlSanitizerComponentInstaller_;
  std::unique_ptr<URLSanitizerService> urlSanitizer_;
}
@end

@implementation LocalDataFileService

- (instancetype)init {
  if ((self = [super init])) {
    delegate_ = std::make_unique<LocalDataFileServiceDelegate>();
    fileService_ = LocalDataFilesServiceFactory(delegate_.get());
    urlSanitizer_ = std::make_unique<brave::URLSanitizerService>();
    urlSanitizerComponentInstaller_ =
        std::make_unique<URLSanitizerComponentInstaller>(fileService_.get());
    urlSanitizerComponentInstaller_->AddObserver(urlSanitizer_.get());
  }
  return self;
}

- (void)start {
  fileService_.get()->Start();
}

- (NSURL*)cleanedURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  GURL cleanURL = urlSanitizer_.get()->SanitizeURL(gurl);
  return net::NSURLWithGURL(cleanURL);
}

@end
