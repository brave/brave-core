/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_ENUMS_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_ENUMS_H_

#import <Foundation/Foundation.h>

typedef NS_OPTIONS(NSUInteger, BravePublicKeyUsage) {
  BravePublicKeyUsage_INVALID = 1 << 0,
  BravePublicKeyUsage_ENCRYPT = 1 << 1,
  BravePublicKeyUsage_DECRYPT = 1 << 2,
  BravePublicKeyUsage_SIGN = 1 << 3,
  BravePublicKeyUsage_VERIFY = 1 << 4,
  BravePublicKeyUsage_WRAP = 1 << 5,
  BravePublicKeyUsage_DERIVE = 1 << 6,
  BravePublicKeyUsage_ANY = 1 << 7
};

typedef NS_ENUM(NSUInteger, BravePublicKeyType) {
  BravePublicKeyType_UNKNOWN,
  BravePublicKeyType_RSA,
  BravePublicKeyType_DSA,
  BravePublicKeyType_DH,
  BravePublicKeyType_EC
};

typedef NS_ENUM(NSUInteger, BraveFingerprintType) {
  BraveFingerprintType_SHA1,
  BraveFingerprintType_SHA256
};

#endif  // BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_ENUMS_H_
