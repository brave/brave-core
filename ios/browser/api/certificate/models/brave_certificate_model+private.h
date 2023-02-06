/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_MODEL_PRIVATE_H_
#define BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_MODEL_PRIVATE_H_

#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include "brave/ios/browser/api/certificate/models/brave_certificate_enums.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_fingerprint.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_public_key_info.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_rdns_sequence.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_signature.h"

namespace net {
class ParsedCertificate;

namespace der {
class BitString;
class Input;
}  // namespace der
}  // namespace net

typedef NS_ENUM(NSUInteger, BraveFingerprintType);

NS_ASSUME_NONNULL_BEGIN

@interface BraveCertificateSignature ()
- (instancetype)initWithCertificate:(const net::ParsedCertificate*)certificate;
@end

@interface BraveCertificatePublicKeyInfo ()
- (instancetype)initWithCertificate:(const net::ParsedCertificate*)certificate
                            withKey:(SecKeyRef)key;
@end

@interface BraveCertificateFingerprint ()
- (instancetype)initWithCertificate:(CFDataRef)cert_data
                           withType:(BraveFingerprintType)type;
@end

@interface BraveCertificateRDNSequence ()
- (instancetype)initWithBERName:(const net::der::Input&)berName
                       uniqueId:(const net::der::BitString&)uniqueId;
@end

NS_ASSUME_NONNULL_END

#endif  // BRAVE_IOS_BROWSER_API_CERTIFICATE_MODELS_BRAVE_CERTIFICATE_MODEL_PRIVATE_H_
