/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

NS_ASSUME_NONNULL_BEGIN

#ifndef OPENSSL_IS_BORINGSSL
OBJC_EXPORT
@interface BraveCertificateTLSFeatureExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<NSNumber*>* features; //array of long
@end
#endif

NS_ASSUME_NONNULL_END
