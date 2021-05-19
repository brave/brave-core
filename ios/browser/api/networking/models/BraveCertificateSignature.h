/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateSignature: NSObject
@property(nonatomic, strong, readonly) NSString* algorithm;
@property(nonatomic, strong, readonly) NSString* objectIdentifier;
@property(nonatomic, strong, readonly) NSString* signatureHexEncoded;
@property(nonatomic, strong, readonly) NSString* parameters;
@property(nonatomic, assign, readonly) NSUInteger bytesSize;
@end

NS_ASSUME_NONNULL_END
