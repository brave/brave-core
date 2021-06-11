/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENSION_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENSION_H_

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
#else
  #import "brave_certificate_enums.h"
#endif

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateExtensionModel: NSObject
@property(nonatomic, assign, readonly) BraveExtensionType type;
@property(nonatomic, assign, readonly) bool isCritical;
@property(nonatomic, strong, readonly) NSString* onid;
@property(nonatomic, assign, readonly) NSInteger nid;
@property(nonatomic, strong, readonly) NSString* name;
@property(nonatomic, strong, readonly) NSString* title;
@end

NS_ASSUME_NONNULL_END

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_EXTENSION_H_
