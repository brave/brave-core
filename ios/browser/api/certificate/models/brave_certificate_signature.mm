/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/models/brave_certificate_signature.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_utils.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"
#include "net/cert/pki/parsed_certificate.h"
#include "net/der/input.h"

@implementation BraveCertificateSignature
- (instancetype)initWithCertificate:(const net::ParsedCertificate*)certificate {
  if ((self = [super init])) {
    _algorithm = [[NSString alloc] init];
    _digest = [[NSString alloc] init];
    _objectIdentifier = [[NSData alloc] init];
    _absoluteObjectIdentifier = [[NSString alloc] init];
    _signatureHexEncoded = [[NSString alloc] init];
    _parameters = [[NSString alloc] init];
    _bytesSize = 0;

    if (certificate->signature_algorithm().has_value()) {
      _digest = base::SysUTF8ToNSString(
          certificate::x509_utils::SignatureAlgorithmDigestToName(
              *certificate->signature_algorithm()));
      _algorithm = base::SysUTF8ToNSString(
          certificate::x509_utils::SignatureAlgorithmIdToName(
              *certificate->signature_algorithm()));
    }

    net::der::Input signature_oid;
    net::der::Input signature_params;
    if (certificate::x509_utils::ParseAlgorithmIdentifier(
            certificate->signature_algorithm_tlv(), &signature_oid,
            &signature_params)) {
      _objectIdentifier =
          certificate::utils::NSStringToData(signature_oid.AsString());

      std::string absolute_oid =
          certificate::x509_utils::NIDToAbsoluteOID(signature_oid);
      if (!absolute_oid.empty()) {
        _absoluteObjectIdentifier = base::SysUTF8ToNSString(absolute_oid);
      }

      if (!certificate::x509_utils::IsNull(signature_params)) {
        std::string signature_params_string = signature_params.AsString();
        _parameters = base::SysUTF8ToNSString(base::HexEncode(
            signature_params_string.data(), signature_params_string.size()));
      }
    }

    std::string signature_string =
        certificate->signature_value().bytes().AsString();
    _signatureHexEncoded = base::SysUTF8ToNSString(
        base::HexEncode(signature_string.data(), signature_string.size()));
    _bytesSize = certificate->signature_value().bytes().Length();
  }
  return self;
}
@end
