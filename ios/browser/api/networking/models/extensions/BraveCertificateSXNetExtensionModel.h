/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

NS_ASSUME_NONNULL_BEGIN

#ifndef OPENSSL_IS_BORINGSSL
OBJC_EXPORT
@interface BraveCertificateSXNetIDExtensionModel: NSObject
@property(nonatomic, strong, readonly) NSString* zone;
@property(nonatomic, strong, readonly) NSString* user;
@end

OBJC_EXPORT
@interface BraveCertificateSXNetExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, assign, readonly) NSInteger version;
@property(nonatomic, strong, readonly) NSArray<BraveCertificateSXNetIDExtensionModel*>* ids;
@end
#endif

NS_ASSUME_NONNULL_END
