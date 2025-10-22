/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/models/brave_certificate_fingerprint.h"

#include "base/containers/span.h"
#include "base/hash/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/types/fixed_array.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_enums.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_utils.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"

@implementation BraveCertificateFingerprint
- (instancetype)initWithCertificate:(CFDataRef)cert_data
                           withType:(BraveFingerprintType)type {
  if ((self = [super init])) {
    _type = type;

    switch (type) {
      case BraveFingerprintType_SHA1: {
        std::string data = base::SHA1HashString(std::string(
            reinterpret_cast<const char*>(CFDataGetBytePtr(cert_data)),
            CFDataGetLength(cert_data)));
        _fingerprintHexEncoded =
            base::SysUTF8ToNSString(base::HexEncode(data.data(), data.size()));

      } break;

      case BraveFingerprintType_SHA256: {
        base::FixedArray<uint8_t> data(crypto::kSHA256Length);
        base::FixedArray<uint8_t> cert_bytes(CFDataGetLength(cert_data));
        std::copy_n(CFDataGetBytePtr(cert_data), CFDataGetLength(cert_data),
                    cert_bytes.data());
        std::unique_ptr<crypto::SecureHash> secure_hash(
            crypto::SecureHash::Create(crypto::SecureHash::SHA256));

        secure_hash->Update(base::span<const uint8_t>(cert_bytes));
        secure_hash->Finish(data);

        _fingerprintHexEncoded =
            base::SysUTF8ToNSString(base::HexEncode(data.data(), data.size()));
      } break;
    }
  }
  return self;
}
@end
