/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_NITRO_UTILS_COSE_H_
#define BRAVE_COMPONENTS_NITRO_UTILS_COSE_H_

#include <vector>

#include "components/cbor/values.h"
#include "net/cert/internal/verify_certificate_chain.h"

namespace nitro_utils {

class CoseSign1 : net::VerifyCertificateChainDelegate {
 public:
  CoseSign1();
  ~CoseSign1() override;

  bool DecodeFromBytes(const std::vector<uint8_t>& data);

  bool Verify(const net::ParsedCertificateList& cert_chain);

  cbor::Value protected_headers;
  cbor::Value unprotected_headers;
  cbor::Value payload;

 private:
  bool IsSignatureAlgorithmAcceptable(
      const net::SignatureAlgorithm& signature_algorithm,
      net::CertErrors* errors) override;

  bool IsPublicKeyAcceptable(EVP_PKEY* public_key,
                             net::CertErrors* errors) override;

  cbor::Value protected_encoded;
  cbor::Value payload_encoded;
  std::vector<uint8_t> signature;
};

}  // namespace nitro_utils

#endif  // BRAVE_COMPONENTS_NITRO_UTILS_COSE_H_
