/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>
#import "BraveCertificateExtensionModel.h"

NS_ASSUME_NONNULL_BEGIN

#ifndef OPENSSL_NO_CT
OBJC_EXPORT
@interface BraveCertificateSCTModel: NSObject
@property(nonatomic, assign, readonly) NSInteger version;
@property(nonatomic, assign, readonly) NSInteger logEntryType;
@property(nonatomic, strong, readonly) NSString* logId;
@property(nonatomic, strong, readonly) NSDate* timestamp;
@property(nonatomic, strong, readonly) NSString* extensions;
@property(nonatomic, assign, readonly) NSInteger signatureNid;
@property(nonatomic, strong, readonly) NSString* signatureName;
@property(nonatomic, strong, readonly) NSString* signature;
@property(nullable, nonatomic, strong, readonly) NSString* hexRepresentation;
@end

OBJC_EXPORT
@interface BraveCertificateSCTExtensionModel: BraveCertificateExtensionModel
@property(nonatomic, strong, readonly) NSArray<BraveCertificateSCTModel*>* scts;
@end
#endif

NS_ASSUME_NONNULL_END
