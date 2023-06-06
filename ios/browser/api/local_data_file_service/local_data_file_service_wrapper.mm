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
#include "brave/ios/browser/api/local_data_file_service/local_data_file_service_installer_delegate.h"
#include "brave/ios/browser/api/local_data_file_service/local_data_file_service_wrapper+private.h"
#import "net/base/mac/url_conversions.h"

using brave::URLSanitizerComponentInstaller;
using brave::URLSanitizerService;
using brave_component_updater::LocalDataFilesService;
using brave_component_updater::LocalDataFilesServiceFactory;
using local_data_file_service::LocalDataFileServiceDelegate;

// LocalDataFileServiceWrapper
@interface LocalDataFileServiceWrapper () {
  std::unique_ptr<LocalDataFileServiceDelegate> _delegate;
  std::unique_ptr<LocalDataFilesService> _fileService;
  std::unique_ptr<URLSanitizerComponentInstaller>
      _urlSanitizerComponentInstaller;
  std::unique_ptr<URLSanitizerService> _urlSanitizer;
}
@end

@implementation LocalDataFileServiceWrapper

- (instancetype)init {
  if ((self = [super init])) {
    _delegate = std::make_unique<LocalDataFileServiceDelegate>();
    _fileService = LocalDataFilesServiceFactory(_delegate.get());
    _urlSanitizer = std::make_unique<brave::URLSanitizerService>();
    _urlSanitizerComponentInstaller =
        std::make_unique<URLSanitizerComponentInstaller>(_fileService.get());
    _urlSanitizerComponentInstaller->AddObserver(_urlSanitizer.get());
  }
  return self;
}

- (void)start {
  _fileService.get()->Start();
}

- (NSURL*)cleanedURL:(NSURL*)url {
  GURL gurl = net::GURLWithNSURL(url);
  GURL cleanURL = _urlSanitizer.get()->SanitizeURL(gurl);
  return net::NSURLWithGURL(cleanURL);
}

@end
