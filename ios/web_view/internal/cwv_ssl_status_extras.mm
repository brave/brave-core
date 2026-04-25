// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_ssl_status_extras.h"

#include <Foundation/Foundation.h>

#include "base/strings/sys_string_conversions.h"
#include "components/ssl_errors/error_info.h"
#include "ios/web_view/internal/cwv_ssl_status_internal.h"
#include "net/base/apple/url_conversions.h"
#include "net/cert/cert_status_flags.h"
#include "url/gurl.h"

@interface CWVSSLErrorInformation ()
@property(nonatomic, copy) NSString* details;
@property(nonatomic, copy) NSString* shortDescription;
@end

@implementation CWVSSLErrorInformation

- (instancetype)initWithErrorInfo:(ssl_errors::ErrorInfo)errorInfo {
  if ((self = [super init])) {
    self.details = base::SysUTF16ToNSString(errorInfo.details());
    self.shortDescription =
        base::SysUTF16ToNSString(errorInfo.short_description());
  }
  return self;
}

@end

@implementation CWVSSLStatus (Extras)

- (BOOL)isCertStatusError {
  return net::IsCertStatusError(self.internalStatus.cert_status);
}

- (NSArray<CWVSSLErrorInformation*>*)certStatusErrorsForURL:(NSURL*)url {
  std::vector<ssl_errors::ErrorInfo> errors;
  ssl_errors::ErrorInfo::GetErrorsForCertStatus(
      self.internalStatus.certificate, self.internalStatus.cert_status,
      net::GURLWithNSURL(url), &errors);
  NSMutableArray<CWVSSLErrorInformation*>* bridgedErrors =
      [[NSMutableArray alloc] init];
  for (size_t i = 0; i < errors.size(); ++i) {
    [bridgedErrors
        addObject:[[CWVSSLErrorInformation alloc] initWithErrorInfo:errors[i]]];
  }
  return [bridgedErrors copy];
}

@end
