/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "net/base/net_export.h"
#include "net/cert/ct_objects_extractor.h"
#include "net/cert/ct_serialization.h"
#include "net/cert/internal/cert_errors.h"
#include "net/cert/internal/parse_certificate.h"
#include "net/cert/internal/signature_algorithm.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/der/encode_values.h"
#include "net/der/input.h"
#include "net/der/parse_values.h"
#include "net/der/parser.h"
#include "net/der/tag.h"
#include "third_party/boringssl/src/include/openssl/base.h"
#include "third_party/boringssl/src/include/openssl/crypto.h"
#include "third_party/boringssl/src/include/openssl/err.h"
#include "third_party/boringssl/src/include/openssl/obj.h"

namespace certificate {
namespace x509_utils {
std::vector<net::der::Input> SupportedExtensionOIDs() {
  return {net::der::Input(net::kSubjectKeyIdentifierOid),
          net::der::Input(net::kKeyUsageOid),
          net::der::Input(net::kSubjectAltNameOid),
          net::der::Input(net::kBasicConstraintsOid),
          net::der::Input(net::kNameConstraintsOid),
          net::der::Input(net::kCertificatePoliciesOid),
          net::der::Input(net::kAuthorityKeyIdentifierOid),
          net::der::Input(net::kPolicyConstraintsOid),
          net::der::Input(net::kExtKeyUsageOid),
          net::der::Input(net::kAuthorityInfoAccessOid),
          net::der::Input(net::kAdCaIssuersOid),
          net::der::Input(net::kAdOcspOid),
          net::der::Input(net::kCrlDistributionPointsOid)};
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

  std::vector<base::StringPiece> parsed_scts;
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

bool ParseAlgorithmIdentifier(const net::der::Input& input,
                              net::der::Input* algorithm_oid,
                              net::der::Input* parameters) {
  DCHECK(algorithm_oid);
  DCHECK(parameters);

  if (!algorithm_oid || !parameters) {
    return false;
  }

  net::der::Parser parser(input);

  net::der::Parser algorithm_identifier_parser;
  if (!parser.ReadSequence(&algorithm_identifier_parser)) {
    return false;
  }

  if (parser.HasMore()) {
    return false;
  }

  if (!algorithm_identifier_parser.ReadTag(net::der::kOid, algorithm_oid)) {
    return false;
  }

  *parameters = net::der::Input();
  if (algorithm_identifier_parser.HasMore() &&
      !algorithm_identifier_parser.ReadRawTLV(parameters)) {
    return false;
  }
  return !algorithm_identifier_parser.HasMore();
}

bool ParseAlgorithmSequence(const net::der::Input& input,
                            net::der::Input* algorithm_oid,
                            net::der::Input* parameters) {
  DCHECK(algorithm_oid);
  DCHECK(parameters);

  if (!algorithm_oid || !parameters) {
    return false;
  }

  net::der::Parser parser(input);

  // Extract object identifier field
  if (!parser.ReadTag(net::der::kOid, algorithm_oid)) {
    return false;
  }

  if (!parser.HasMore()) {
    return false;
  }

  // Extract the parameters field.
  *parameters = net::der::Input();
  if (parser.HasMore() && !parser.ReadRawTLV(parameters)) {
    return false;
  }
  return !parser.HasMore();
}

bool ParseSubjectPublicKeyInfo(const net::der::Input& input,
                               net::der::Input* algorithm_sequence,
                               net::der::Input* spk) {
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

  net::der::Parser parser(input);
  net::der::Parser spki_parser;
  if (!parser.ReadSequence(&spki_parser)) {
    return false;
  }

  // Extract algorithm field.
  // ReadSequenceTLV then maybe ParseAlgorithmIdentifier instead.
  if (!spki_parser.ReadTag(net::der::kSequence, algorithm_sequence)) {
    return false;
  }

  if (!spki_parser.HasMore()) {
    return false;
  }

  // Extract the subjectPublicKey field.
  if (!spki_parser.ReadTag(net::der::kBitString, spk)) {
    return false;
  }
  return true;
}

bool ParseRSAPublicKeyInfo(const net::der::Input& input,
                           net::der::Input* modulus,
                           net::der::Input* public_exponent) {
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

  net::der::Parser parser(input);
  net::der::Parser rsa_parser;
  if (!parser.ReadSequence(&rsa_parser)) {
    return false;
  }

  // Extract the modulus field.
  if (!rsa_parser.ReadTag(net::der::kInteger, modulus)) {
    return false;
  }

  if (!rsa_parser.HasMore()) {
    return false;
  }

  // Extract the publicExponent field.
  if (!rsa_parser.ReadTag(net::der::kInteger, public_exponent)) {
    return false;
  }
  return true;
}

bool IsNull(const net::der::Input& input) {
  auto IsEmpty = [](const net::der::Input& input) {
    return input.Length() == 0;
  };

  net::der::Parser parser(input);
  net::der::Input null_value;
  if (!parser.ReadTag(net::der::kNull, &null_value)) {
    return false;
  }

  // NULL values are TLV encoded; the value is expected to be empty.
  if (!IsEmpty(null_value)) {
    return false;
  }

  // By definition of this function, the entire input must be a NULL.
  return !parser.HasMore();
}

bool OIDToNID(const net::der::Input& input, std::int32_t* out) {
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

std::string NIDToAbsoluteOID(const net::der::Input& input) {
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
    const net::SignatureAlgorithm& signature_algorithm) {
  switch (signature_algorithm.digest()) {
    case net::DigestAlgorithm::Md2:
      return "MD2";
    case net::DigestAlgorithm::Md4:
      return "MD4";
    case net::DigestAlgorithm::Md5:
      return "MD5";
    case net::DigestAlgorithm::Sha1:
      return "SHA-1";
    case net::DigestAlgorithm::Sha256:
      return "SHA-256";
    case net::DigestAlgorithm::Sha384:
      return "SHA-384";
    case net::DigestAlgorithm::Sha512:
      return "SHA-512";
  }
}

std::string SignatureAlgorithmIdToName(
    const net::SignatureAlgorithm& signature_algorithm) {
  switch (signature_algorithm.algorithm()) {
    case net::SignatureAlgorithmId::RsaPkcs1:
      return "RSA";
    case net::SignatureAlgorithmId::RsaPss:
      return "RSA-PSS";
    case net::SignatureAlgorithmId::Ecdsa:
      return "ECDSA";
    case net::SignatureAlgorithmId::Dsa:
      return "DSA";
  }
}

base::Time GeneralizedTimeToTime(
    const net::der::GeneralizedTime& generalized_time) {
  base::Time time;
  net::der::GeneralizedTimeToTime(generalized_time, &time);
  return time;
}
}  // namespace x509_utils
}  // namespace certificate
