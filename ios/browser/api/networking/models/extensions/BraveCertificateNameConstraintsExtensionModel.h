/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

NS_ASSUME_NONNULL_BEGIN

@class BraveCertificateExtensionGeneralNameModel;

OBJC_EXPORT
@interface BraveCertificateExtensionGeneralSubtreeModel: NSObject
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionGeneralNameModel*>* names;
@property(nonatomic, strong, readonly) NSString* minimum;
@property(nonatomic, strong, readonly) NSString* maximum;
@end

OBJC_EXPORT
@interface BraveCertificateNameConstraintsExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionGeneralSubtreeModel*>* permittedSubtrees;
@property(nonatomic, strong, readonly) NSArray<BraveCertificateExtensionGeneralSubtreeModel*>* excludedSubtrees;
@end

NS_ASSUME_NONNULL_END
