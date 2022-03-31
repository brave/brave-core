/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_PUBLIC_KEY_INFO_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_PUBLIC_KEY_INFO_H_

#import <Foundation/Foundation.h>
#include "brave_certificate_enums.h"  // NOLINT

NS_ASSUME_NONNULL_BEGIN

OBJC_EXPORT
@interface BraveCertificatePublicKeyInfo : NSObject
@property(nonatomic, readonly) BravePublicKeyType type;
@property(nonatomic, readonly) NSString* algorithm;
@property(nonatomic, readonly) NSData* objectIdentifier;
@property(nonatomic, readonly) NSString* absoluteObjectIdentifier;
@property(nonatomic, readonly) NSString* curveName;
@property(nonatomic, readonly) NSString* nistCurveName;
@property(nonatomic, readonly) NSString* parameters;
@property(nonatomic, readonly) NSString* keyHexEncoded;
@property(nonatomic, readonly) NSUInteger keyBytesSize;
@property(nonatomic, readonly) NSUInteger effectiveSize;
@property(nonatomic, readonly) NSUInteger exponent;
@property(nonatomic, readonly) NSUInteger keySizeInBits;
@property(nonatomic, readonly) BravePublicKeyUsage keyUsage;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_PUBLIC_KEY_INFO_H_
