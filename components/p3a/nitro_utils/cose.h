/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_NITRO_UTILS_COSE_H_
#define BRAVE_COMPONENTS_P3A_NITRO_UTILS_COSE_H_

#include <vector>

#include "components/cbor/values.h"
#include "third_party/boringssl/src/pki/verify_certificate_chain.h"

namespace nitro_utils {

// Class for parsing CoseSign1 structures and verifying their
// signatures with certificate chains.
class CoseSign1 : bssl::VerifyCertificateChainDelegate {
 public:
  CoseSign1();
  ~CoseSign1() override;

  CoseSign1(const CoseSign1&) = delete;
  CoseSign1& operator=(const CoseSign1&) = delete;

  // Parses a CBOR encoded CoseSign1 structure and returns true
  // upon success.
  bool DecodeFromBytes(const std::vector<uint8_t>& data);

  // Verifies the signature with a given certificate chain and
  // returns true upon success.
  bool Verify(const bssl::ParsedCertificateList& cert_chain);

  // Retrieves value containing headers protected by the signature.
  const cbor::Value& protected_headers();
  // Retrieves value containing headers not protected by the signature.
  const cbor::Value& unprotected_headers();
  // Retrieves value containing the payload of the CoseSign1 structure.
  const cbor::Value& payload();

 private:
  bool IsSignatureAlgorithmAcceptable(
      bssl::SignatureAlgorithm signature_algorithm,
      bssl::CertErrors* errors) override;

  bool IsPublicKeyAcceptable(EVP_PKEY* public_key,
                             bssl::CertErrors* errors) override;

  bssl::SignatureVerifyCache* GetVerifyCache() override;

  cbor::Value protected_headers_;
  cbor::Value unprotected_headers_;
  cbor::Value payload_;

  cbor::Value protected_encoded_;
  cbor::Value payload_encoded_;
  std::vector<uint8_t> signature_;
};

}  // namespace nitro_utils

#endif  // BRAVE_COMPONENTS_P3A_NITRO_UTILS_COSE_H_
