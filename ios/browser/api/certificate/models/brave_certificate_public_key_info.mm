/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/models/brave_certificate_public_key_info.h"
#include <type_traits>
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_enums.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_utils.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"
#include "net/cert/pki/parsed_certificate.h"
#include "net/der/input.h"
#include "net/der/parse_values.h"

@implementation BraveCertificatePublicKeyInfo
// Chromium's GetSubjectPublicKeyBytes is NOT enough
// It only gives the Raw TLV BIT_STRING of the SubjectPublicKeyInfo
- (instancetype)initWithCertificate:(net::ParsedCertificate*)certificate
                            withKey:(SecKeyRef)key {
  if ((self = [super init])) {
    _type = BravePublicKeyType_UNKNOWN;
    _keyUsage = BravePublicKeyUsage_INVALID;
    _algorithm = [[NSString alloc] init];
    _objectIdentifier = [[NSData alloc] init];
    _absoluteObjectIdentifier = [[NSString alloc] init];
    _curveName = [[NSString alloc] init];
    _nistCurveName = [[NSString alloc] init];
    _parameters = [[NSString alloc] init];
    _keyHexEncoded = [[NSString alloc] init];
    _keyBytesSize = 0;
    _exponent = 0;
    _keySizeInBits = 0;

    net::der::Input algorithm_tlv;
    net::der::Input spk;
    if (certificate::x509_utils::ParseSubjectPublicKeyInfo(
            certificate->tbs().spki_tlv, &algorithm_tlv, &spk)) {
      net::der::Input algorithm_oid;
      net::der::Input parameters;

      if (certificate::x509_utils::ParseAlgorithmSequence(
              algorithm_tlv, &algorithm_oid, &parameters)) {
        _objectIdentifier =
            certificate::utils::NSStringToData(algorithm_oid.AsString());

        std::string absolute_oid =
            certificate::x509_utils::NIDToAbsoluteOID(algorithm_oid);
        if (!absolute_oid.empty()) {
          _absoluteObjectIdentifier = base::SysUTF8ToNSString(absolute_oid);
        }

        if (!certificate::x509_utils::IsNull(parameters)) {
          std::string parameters_string = parameters.AsString();
          _parameters = base::SysUTF8ToNSString(base::HexEncode(
              parameters_string.data(), parameters_string.size()));
        }
      }

      // SPK has the unused bit count. Remove it.
      // When doing extensions, we can use the below to parse the SPK and then
      // the extensions. For now, not needed.
      /*auto spk_string = spk.AsStringPiece();
      if (base::StartsWith(spk_string, "\0")) {
        spk_string.remove_prefix(1);
        spk = net::der::Input(spk_string);
      }*/
    }

    _keyBytesSize = SecKeyGetBlockSize(key);
    NSData* external_representation =
        (__bridge_transfer NSData*)SecKeyCopyExternalRepresentation(key, nil);

    _keyHexEncoded = base::SysUTF8ToNSString(base::HexEncode(
        [external_representation bytes], [external_representation length]));

    NSDictionary* attributes =
        (__bridge_transfer NSDictionary*)SecKeyCopyAttributes(key);
    if (attributes) {
      _keySizeInBits =
          [attributes[(NSString*)kSecAttrKeySizeInBits] integerValue];
      _effectiveSize =
          [attributes[(NSString*)kSecAttrEffectiveKeySize] integerValue];

      if ([attributes[(NSString*)kSecAttrCanEncrypt] boolValue]) {
        _keyUsage |= BravePublicKeyUsage_ENCRYPT;
      }

      if ([attributes[(NSString*)kSecAttrCanDecrypt] boolValue]) {
        _keyUsage |= BravePublicKeyUsage_DECRYPT;
      }

      if ([attributes[(NSString*)kSecAttrCanSign] boolValue]) {
        _keyUsage |= BravePublicKeyUsage_SIGN;
      }

      if ([attributes[(NSString*)kSecAttrCanVerify] boolValue]) {
        _keyUsage |= BravePublicKeyUsage_VERIFY;
      }

      if ([attributes[(NSString*)kSecAttrCanWrap] boolValue]) {
        _keyUsage |= BravePublicKeyUsage_WRAP;
      }

      if ([attributes[(NSString*)kSecAttrCanDerive] boolValue]) {
        _keyUsage |= BravePublicKeyUsage_DERIVE;
      }

      BravePublicKeyUsage any_usage = static_cast<BravePublicKeyUsage>(0);
      any_usage &= BravePublicKeyUsage_ENCRYPT;
      any_usage &= BravePublicKeyUsage_SIGN;
      any_usage &= BravePublicKeyUsage_VERIFY;
      any_usage &= BravePublicKeyUsage_WRAP;
      any_usage &= BravePublicKeyUsage_DERIVE;

      if (_keyUsage == any_usage) {
        _keyUsage = BravePublicKeyUsage_ANY;
      }

      if ([attributes[(NSString*)kSecAttrType]
              isEqualToString:(NSString*)kSecAttrKeyTypeRSA]) {
        _type = BravePublicKeyType_RSA;
        _algorithm = @"RSA";

        // Recreate the SPK without the unused bit count because the previous
        // remove_prefix code didn't work. Parse the RSA ASN.1 PKCS1 structure
        // from SecKeyCopyExternalRepresentation.
        net::der::Input modulus;
        net::der::Input public_exponent;
        spk = net::der::Input(
            static_cast<const std::uint8_t*>([external_representation bytes]),
            [external_representation length]);
        if (certificate::x509_utils::ParseRSAPublicKeyInfo(spk, &modulus,
                                                           &public_exponent)) {
          std::string modulus_string = modulus.AsString();
          _keyHexEncoded = base::SysUTF8ToNSString(
              base::HexEncode(modulus_string.data(), modulus_string.size()));

          std::uint64_t parsed_public_exponent = 0;
          if (net::der::ParseUint64(public_exponent, &parsed_public_exponent)) {
            _exponent =
                static_cast<decltype(_exponent)>(parsed_public_exponent);
          }
        }
      }

      if ([attributes[(NSString*)kSecAttrType]
              isEqualToString:(NSString*)kSecAttrKeyTypeEC]) {
        _type = BravePublicKeyType_EC;
        _algorithm = @"Elliptic Curve";
      }

      if ([attributes[(NSString*)kSecAttrType]
              isEqualToString:(NSString*)kSecAttrKeyTypeECSECPrimeRandom]) {
        _type = BravePublicKeyType_EC;
        _algorithm = @"Elliptic Curve (Prime Random)";
      }
    }

    //    // Parse SPKI
    //    net::CertErrors key_errors;
    //    bssl::UniquePtr<EVP_PKEY> pkey;
    //    if (!net::ParsePublicKey(certificate->tbs().spki_tlv, &pkey)) {
    //      key_errors.AddError(net::cert_errors::kFailedParsingSpki);
    //      NSLog(@"Error: %s\n", key_errors.ToDebugString().c_str());
    //      return self;
    //    }
    //
    //    if (!pkey) {
    //      NSLog(@"Error: %s\n", key_errors.ToDebugString().c_str());
    //      return self;
    //    }

    // TODO(@Brandon-T)
    // TODO Parse Key further for EC Curve Name and RSA Exponent
    // TODO Parse Key further for Raw Key without ASN1 DER Header/PKCS
  }
  return self;
}
@end
