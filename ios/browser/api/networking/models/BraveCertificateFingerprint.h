/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_BRAVE_CERTIFICATE_FINGERPRINT_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_BRAVE_CERTIFICATE_FINGERPRINT_H_

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
#else
  #import "brave_certificate_enums.h"
#endif

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateFingerprint: NSObject
@property(nonatomic, assign, readonly) BraveFingerprintType type;
@property(nonatomic, strong, readonly) NSString* fingerprintHexEncoded;
@end

NS_ASSUME_NONNULL_END

#endif  //  #ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_BRAVE_CERTIFICATE_FINGERPRINT_H_
