/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_PRIVATE_KEY_USAGE_PERIOD_EXTENSION_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_PRIVATE_KEY_USAGE_PERIOD_EXTENSION_H_

NS_ASSUME_NONNULL_BEGIN

#ifndef OPENSSL_IS_BORINGSSL
OBJC_EXPORT
@interface BraveCertificatePrivateKeyUsagePeriodExtensionModel: BraveCertificateExtensionModel
@property (nullable, nonatomic, strong, readonly) NSDate* notBefore;
@property (nullable, nonatomic, strong, readonly) NSDate* notAfter;
@end
#endif  //  OPENSSL_IS_BORINGSSL

NS_ASSUME_NONNULL_END

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_MODELS_EXTENSIONS_BRAVE_CERTIFICATE_PRIVATE_KEY_USAGE_PERIOD_EXTENSION_H_