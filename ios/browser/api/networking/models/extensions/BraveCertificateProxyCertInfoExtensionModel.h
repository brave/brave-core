/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateProxyPolicyExtensionModel: NSObject
@property(nonatomic, strong, readonly) NSString* language;
@property(nonatomic, strong, readonly) NSString* policyText;
@end

OBJC_EXPORT
@interface BraveCertificateProxyCertInfoExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, assign, readonly) NSInteger pathLengthConstraint;
@property(nullable, nonatomic, strong, readonly) BraveCertificateProxyPolicyExtensionModel* proxyPolicy;
@end

NS_ASSUME_NONNULL_END
