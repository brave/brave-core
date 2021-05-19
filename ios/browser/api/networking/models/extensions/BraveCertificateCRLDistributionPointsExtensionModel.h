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

@class BraveCertificateExtensionGeneralNameModel;

OBJC_EXPORT
@interface BraveCertificateCRLDistPointExtensionModel: NSObject
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionGeneralNameModel*>* genDistPointName;
@property(nonatomic, strong, readonly) NSDictionary<NSString*, NSString*>* relativeDistPointNames;
@property(nonatomic, assign, readonly) BraveCRLReasonFlags reasonFlags;
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionGeneralNameModel*>* crlIssuer;
@property(nonatomic, assign, readonly) NSInteger dpReason;
@end

OBJC_EXPORT
@interface BraveCertificateCRLDistributionPointsExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<BraveCertificateCRLDistPointExtensionModel*>* distPoints;
@end

NS_ASSUME_NONNULL_END
