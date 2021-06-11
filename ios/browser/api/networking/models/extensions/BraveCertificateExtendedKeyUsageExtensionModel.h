/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENDED_KEY_USAGE_EXTENSION_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENDED_KEY_USAGE_EXTENSION_H_

#if defined(BRAVE_CORE)
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
#else
  #import "brave_certificate_enums.h"
#endif

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateExtensionExtendedKeyUsageModel: NSObject
@property(nonatomic, assign, readonly) NSInteger nid;
@property(nonatomic, assign, readonly) NSString* nidString;
@property(nonatomic, strong, readonly) NSString* name;
@property(nonatomic, assign, readonly) BraveExtendedKeyUsage keyUsage;
@end

OBJC_EXPORT
@interface BraveCertificateExtendedKeyUsageExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionExtendedKeyUsageModel*>* keyPurposes;
@end

NS_ASSUME_NONNULL_END

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENDED_KEY_USAGE_EXTENSION_H_
