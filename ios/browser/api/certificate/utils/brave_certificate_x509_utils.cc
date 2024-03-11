/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"

#include <string_view>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "net/base/net_export.h"
#include "net/cert/ct_objects_extractor.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/time_conversions.h"
#include "third_party/boringssl/src/include/openssl/base.h"
#include "third_party/boringssl/src/include/openssl/crypto.h"
#include "third_party/boringssl/src/include/openssl/err.h"
#include "third_party/boringssl/src/include/openssl/obj.h"
#include "third_party/boringssl/src/pki/cert_errors.h"
#include "third_party/boringssl/src/pki/input.h"
#include "third_party/boringssl/src/pki/parse_certificate.h"
#include "third_party/boringssl/src/pki/parse_values.h"
#include "third_party/boringssl/src/pki/parser.h"
#include "third_party/boringssl/src/pki/signature_algorithm.h"

namespace certificate {
namespace x509_utils {
std::vector<bssl::der::Input> SupportedExtensionOIDs() {
  return {bssl::der::Input(bssl::kSubjectKeyIdentifierOid),
          bssl::der::Input(bssl::kKeyUsageOid),
          bssl::der::Input(bssl::kSubjectAltNameOid),
          bssl::der::Input(bssl::kBasicConstraintsOid),
          bssl::der::Input(bssl::kNameConstraintsOid),
          bssl::der::Input(bssl::kCertificatePoliciesOid),
          bssl::der::Input(bssl::kAuthorityKeyIdentifierOid),
          bssl::der::Input(bssl::kPolicyConstraintsOid),
          bssl::der::Input(bssl::kExtKeyUsageOid),
          bssl::der::Input(bssl::kAuthorityInfoAccessOid),
          bssl::der::Input(bssl::kAdCaIssuersOid),
          bssl::der::Input(bssl::kAdOcspOid),
          bssl::der::Input(bssl::kCrlDistributionPointsOid)};
}

bool ExtractEmbeddedSCT(
    const CRYPTO_BUFFER* cert,
    std::vector<scoped_refptr<net::ct::SignedCertificateTimestamp>>* scts) {
  DCHECK(cert);
  DCHECK(scts);

  if (!cert) {
    return false;
  }

  std::string sct_list;
  if (!net::ct::ExtractEmbeddedSCTList(cert, &sct_list)) {
    return false;
  }

  std::vector<std::string_view> parsed_scts;
  if (!net::ct::DecodeSCTList(sct_list, &parsed_scts)) {
    return false;
  }

  if (parsed_scts.empty()) {
    return false;
  }

  bool result = true;
  for (auto&& parsed_sct : parsed_scts) {
    scoped_refptr<net::ct::SignedCertificateTimestamp> sct(
        new net::ct::SignedCertificateTimestamp());
    result =
        net::ct::DecodeSignedCertificateTimestamp(&parsed_sct, &sct) && result;
    scts->emplace_back(sct);
  }
  return result;
}

bool ParseAlgorithmIdentifier(const bssl::der::Input& input,
                              bssl::der::Input* algorithm_oid,
                              bssl::der::Input* parameters) {
  DCHECK(algorithm_oid);
  DCHECK(parameters);

  if (!algorithm_oid || !parameters) {
    return false;
  }

  bssl::der::Parser parser(input);

  bssl::der::Parser algorithm_identifier_parser;
  if (!parser.ReadSequence(&algorithm_identifier_parser)) {
    return false;
  }

  if (parser.HasMore()) {
    return false;
  }

  if (!algorithm_identifier_parser.ReadTag(CBS_ASN1_OBJECT, algorithm_oid)) {
    return false;
  }

  *parameters = bssl::der::Input();
  if (algorithm_identifier_parser.HasMore() &&
      !algorithm_identifier_parser.ReadRawTLV(parameters)) {
    return false;
  }
  return !algorithm_identifier_parser.HasMore();
}

bool ParseAlgorithmSequence(const bssl::der::Input& input,
                            bssl::der::Input* algorithm_oid,
                            bssl::der::Input* parameters) {
  DCHECK(algorithm_oid);
  DCHECK(parameters);

  if (!algorithm_oid || !parameters) {
    return false;
  }

  bssl::der::Parser parser(input);

  // Extract object identifier field
  if (!parser.ReadTag(CBS_ASN1_OBJECT, algorithm_oid)) {
    return false;
  }

  if (!parser.HasMore()) {
    return false;
  }

  // Extract the parameters field.
  *parameters = bssl::der::Input();
  if (parser.HasMore() && !parser.ReadRawTLV(parameters)) {
    return false;
  }
  return !parser.HasMore();
}

bool ParseSubjectPublicKeyInfo(const bssl::der::Input& input,
                               bssl::der::Input* algorithm_sequence,
                               bssl::der::Input* spk) {
  // From RFC 5280, Section 4.1
  //   SubjectPublicKeyInfo  ::=  SEQUENCE  {
  //     algorithm            AlgorithmIdentifier,
  //     subjectPublicKey     BIT STRING  }
  //
  //   AlgorithmIdentifier  ::=  SEQUENCE  {
  //     algorithm               OBJECT IDENTIFIER,
  //     parameters              ANY DEFINED BY algorithm OPTIONAL  }

  DCHECK(algorithm_sequence);
  DCHECK(spk);

  if (!algorithm_sequence || !spk) {
    return false;
  }

  bssl::der::Parser parser(input);
  bssl::der::Parser spki_parser;
  if (!parser.ReadSequence(&spki_parser)) {
    return false;
  }

  // Extract algorithm field.
  // ReadSequenceTLV then maybe ParseAlgorithmIdentifier instead.
  if (!spki_parser.ReadTag(CBS_ASN1_SEQUENCE, algorithm_sequence)) {
    return false;
  }

  if (!spki_parser.HasMore()) {
    return false;
  }

  // Extract the subjectPublicKey field.
  if (!spki_parser.ReadTag(CBS_ASN1_BITSTRING, spk)) {
    return false;
  }
  return true;
}

bool ParseRSAPublicKeyInfo(const bssl::der::Input& input,
                           bssl::der::Input* modulus,
                           bssl::der::Input* public_exponent) {
  // From RFC 3447, Appendix-A.1.1
  //   RSAPublicKey  ::=  SEQUENCE  {
  //     modulus            INTEGER,
  //     publicExponent     INTEGER
  //   }

  DCHECK(modulus);
  DCHECK(public_exponent);

  if (!modulus || !public_exponent) {
    return false;
  }

  bssl::der::Parser parser(input);
  bssl::der::Parser rsa_parser;
  if (!parser.ReadSequence(&rsa_parser)) {
    return false;
  }

  // Extract the modulus field.
  if (!rsa_parser.ReadTag(CBS_ASN1_INTEGER, modulus)) {
    return false;
  }

  if (!rsa_parser.HasMore()) {
    return false;
  }

  // Extract the publicExponent field.
  if (!rsa_parser.ReadTag(CBS_ASN1_INTEGER, public_exponent)) {
    return false;
  }
  return true;
}

bool IsNull(const bssl::der::Input& input) {
  auto IsEmpty = [](const bssl::der::Input& input) {
    return input.Length() == 0;
  };

  bssl::der::Parser parser(input);
  bssl::der::Input null_value;
  if (!parser.ReadTag(CBS_ASN1_NULL, &null_value)) {
    return false;
  }

  // NULL values are TLV encoded; the value is expected to be empty.
  if (!IsEmpty(null_value)) {
    return false;
  }

  // By definition of this function, the entire input must be a NULL.
  return !parser.HasMore();
}

bool OIDToNID(const bssl::der::Input& input, std::int32_t* out) {
  DCHECK(out);

  if (!out) {
    return false;
  }

  *out = -1;
  bool result = false;
  CRYPTO_library_init();

  CBS cbs;
  CBS_init(&cbs, input.UnsafeData(), input.Length());
  int nid = OBJ_cbs2nid(&cbs);
  if (nid != NID_undef) {
    result = true;
    *out = static_cast<std::int32_t>(nid);
  }

  ERR_clear_error();
  return result;
}

std::string NIDToAbsoluteOID(const bssl::der::Input& input) {
  std::int32_t nid = -1;
  if (OIDToNID(input, &nid)) {
    ASN1_OBJECT* object = OBJ_nid2obj(nid);
    if (object) {
      std::string buffer = std::string(128, '\0');
      int total_space = OBJ_obj2txt(&buffer[0], static_cast<int>(buffer.size()),
                                    object, 1 /* no_name */);
      if (total_space > 0) {
        buffer.resize(total_space);
        return buffer;
      }
    }
  }
  return std::string();
}

std::string SignatureAlgorithmDigestToName(
    const bssl::SignatureAlgorithm& signature_algorithm) {
  switch (signature_algorithm) {
    case bssl::SignatureAlgorithm::kEcdsaSha1:
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha1:
      return "SHA-1";
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha256:
    case bssl::SignatureAlgorithm::kEcdsaSha256:
    case bssl::SignatureAlgorithm::kRsaPssSha256:
      return "SHA-256";
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha384:
    case bssl::SignatureAlgorithm::kEcdsaSha384:
    case bssl::SignatureAlgorithm::kRsaPssSha384:
      return "SHA-384";
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha512:
    case bssl::SignatureAlgorithm::kEcdsaSha512:
    case bssl::SignatureAlgorithm::kRsaPssSha512:
      return "SHA-512";
  }
}

std::string SignatureAlgorithmIdToName(
    const bssl::SignatureAlgorithm& signature_algorithm) {
  switch (signature_algorithm) {
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha1:
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha256:
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha384:
    case bssl::SignatureAlgorithm::kRsaPkcs1Sha512:
      return "RSA";
    case bssl::SignatureAlgorithm::kRsaPssSha256:
    case bssl::SignatureAlgorithm::kRsaPssSha384:
    case bssl::SignatureAlgorithm::kRsaPssSha512:
      return "RSA-PSS";
    case bssl::SignatureAlgorithm::kEcdsaSha1:
    case bssl::SignatureAlgorithm::kEcdsaSha256:
    case bssl::SignatureAlgorithm::kEcdsaSha384:
    case bssl::SignatureAlgorithm::kEcdsaSha512:
      return "ECDSA";
  }
}

base::Time GeneralizedTimeToTime(
    const bssl::der::GeneralizedTime& generalized_time) {
  base::Time time;
  net::GeneralizedTimeToTime(generalized_time, &time);
  return time;
}
}  // namespace x509_utils
}  // namespace certificate
