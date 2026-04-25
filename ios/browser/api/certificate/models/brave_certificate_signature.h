/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_SIGNATURE_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_SIGNATURE_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificateSignature : NSObject
@property(nonatomic, readonly) NSString* algorithm;
@property(nonatomic, readonly) NSString* digest;
@property(nonatomic, readonly) NSData* objectIdentifier;
@property(nonatomic, readonly) NSString* absoluteObjectIdentifier;
@property(nonatomic, readonly) NSString* signatureHexEncoded;
@property(nonatomic, readonly) NSString* parameters;
@property(nonatomic, readonly) NSUInteger bytesSize;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_SIGNATURE_H_
