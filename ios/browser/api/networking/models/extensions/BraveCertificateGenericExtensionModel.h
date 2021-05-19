/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
#else
  #import "brave_certificate_enums.h"
#endif

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateGenericExtensionPairModel: NSObject
@property(nonatomic, strong, readonly) NSString* key;
@property(nonatomic, strong, readonly) NSString* value;
@end

OBJC_EXPORT
@interface BraveCertificateGenericExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, assign, readonly) BraveGenericExtensionType extensionType;
@property(nonatomic, strong, readonly) NSString* key;
@property(nullable, nonatomic, strong, readonly) NSString* stringValue;
@property(nullable, nonatomic, strong, readonly) NSArray<BraveCertificateGenericExtensionPairModel*>* arrayValue;
@end

NS_ASSUME_NONNULL_END
