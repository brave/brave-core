/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificatePolicyInfoQualifierNoticeExtensionModel: NSObject
@property(nonatomic, strong, readonly) NSString* organization;
@property(nonatomic, strong, readonly) NSArray<NSString*>* noticeNumbers;
@property(nonatomic, strong, readonly) NSString* explicitText;
@end

OBJC_EXPORT
@interface BraveCertificatePolicyInfoQualifierExtensionModel: NSObject
@property(nonatomic, assign, readonly) NSInteger pqualid;
@property(nonatomic, strong, readonly) NSString* cps;
@property(nullable, nonatomic, strong, readonly) BraveCertificatePolicyInfoQualifierNoticeExtensionModel* notice;
@property(nonatomic, strong, readonly) NSString* unknown;
@end

OBJC_EXPORT
@interface BraveCertificatePolicyInfoExtensionModel: NSObject
@property(nonatomic, strong, readonly) NSString* oid;
@property(nonatomic, strong, readonly) NSArray<BraveCertificatePolicyInfoQualifierExtensionModel*>* qualifiers;
@end

OBJC_EXPORT
@interface BraveCertificatePoliciesExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<BraveCertificatePolicyInfoExtensionModel*>* policies;
@end

NS_ASSUME_NONNULL_END
