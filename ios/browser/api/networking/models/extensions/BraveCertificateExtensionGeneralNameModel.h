/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

#if defined(BRAVE_CORE)
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
#else
  #import "brave_certificate_enums.h"
#endif

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateExtensionGeneralNameModel: NSObject
@property(nonatomic, assign, readonly) BraveGeneralNameType type;
@property(nonatomic, strong, readonly) NSString* other;
@property(nonatomic, strong, readonly) NSString* nameAssigner;
@property(nonatomic, strong, readonly) NSString* partyName;
@property(nonatomic, strong, readonly) NSDictionary<NSString*, NSString*>* dirName;
@end

NS_ASSUME_NONNULL_END
