/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/p3a/nitro_utils/attestation.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/p3a/nitro_utils/cose.h"
#include "components/cbor/reader.h"
#include "crypto/random.h"
#include "net/base/url_util.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/boringssl/src/include/openssl/pool.h"
#include "third_party/boringssl/src/pki/parsed_certificate.h"

namespace nitro_utils {

namespace {

constexpr size_t kAttestationBodyMaxSize = 16384;
// The user_data field contains a multihash of the TLS cert fingerprint
// See https://multiformats.io/multihash/#the-multihash-format and
// https://github.com/multiformats/multicodec/blob/b98f2f38fc63/table.csv#L9
constexpr size_t kMultihashPrefixLength = 2;
constexpr uint8_t kMultihashSHA256Code = 0x12;
constexpr size_t kSHA256HashLength = 32;
constexpr size_t kUserDataMinLength =
    kMultihashPrefixLength + kSHA256HashLength;
// AWS Nitro Enclave Root certificate downloaded from
// https://aws-nitro-enclaves.amazonaws.com/AWS_NitroEnclaves_Root-G1.zip
// Fingerprint `openssl x509 -fingerprint -sha256 -in root.pem -noout`
constexpr net::SHA256HashValue kAWSRootCertFP{
    .data = {0x64, 0x1A, 0x03, 0x21, 0xA3, 0xE2, 0x44, 0xEF, 0xE4, 0x56, 0x46,
             0x31, 0x95, 0xD6, 0x06, 0x31, 0x7E, 0xD7, 0xCD, 0xCC, 0x3C, 0x17,
             0x56, 0xE0, 0x98, 0x93, 0xF3, 0xC6, 0x8F, 0x79, 0xBB, 0x5B}};

// Old-style user_data is a pair of prefix:<binary digest> values
// separated by semicolons. The first value is the TLS cert fingerprint.
constexpr char kHashPrefix[] = "sha256:";
constexpr size_t kHashPrefixLength = sizeof(kHashPrefix) - 1;
constexpr size_t kUserDataOldLength =
    2 * (kSHA256HashLength + kHashPrefixLength) + 1;

net::NetworkTrafficAnnotationTag AttestationAnnotation() {
  return net::DefineNetworkTrafficAnnotation("nitro_utils_attestation", R"(
    semantics {
      sender:
        "AWS Nitro Enclave Attestation/Validation"
      description:
        "Used to validate an AWS Nitro Enclave attestation document. "
        "Nitro Enclaves are used in services such as P3A, to protect user anonymity."
      trigger:
        "Attestation requests are automatically sent at intervals while Brave "
        "is running."
      data: "A random nonce, and attestation document"
      destination: WEBSITE
    }
    policy {
      cookies_allowed: NO
      setting:
        "This feature can be disabled via the P3A setting."
      policy_exception_justification:
        "Not implemented."
    })");
}

bool VerifyNonce(const cbor::Value::MapValue& cose_map,
                 const std::vector<uint8_t>& orig_nonce) {
  cbor::Value::MapValue::const_iterator nonce_it =
      cose_map.find(cbor::Value("nonce"));
  if (nonce_it == cose_map.end() || !nonce_it->second.is_bytestring()) {
    LOG(ERROR) << "Nitro verification: nonce is missing or is not bstr";
    return false;
  }

  if (nonce_it->second.GetBytestring() != orig_nonce) {
    LOG(ERROR) << "Nitro verification: nonce mismatch";
    return false;
  }
  return true;
}

bool VerifyUserDataKey(scoped_refptr<net::X509Certificate> server_cert,
                       const cbor::Value::MapValue& cose_map) {
  CHECK(server_cert);

  const auto user_data_it = cose_map.find(cbor::Value("user_data"));
  if (user_data_it == cose_map.end() || !user_data_it->second.is_bytestring()) {
    LOG(ERROR) << "Nitro verification: user data is missing or is not bstr";
    return false;
  }
  const auto user_data_bytes = user_data_it->second.GetBytestring();

  // Fingerprint of the connection TLS cert to compare against.
  const net::SHA256HashValue server_cert_fp =
      net::X509Certificate::CalculateFingerprint256(server_cert->cert_buffer());

  // The hashes in the  old and new user_data schemes have incommensurate
  // lengths, so use the total length to distinguish between them.
  if (user_data_bytes.size() == kUserDataOldLength) {
    if (memcmp(user_data_bytes.data(), kHashPrefix, kHashPrefixLength) != 0) {
      LOG(ERROR)
          << "Nitro verification: user data is missing sha256 hash prefix";
      return false;
    }
    if (memcmp(server_cert_fp.data, user_data_bytes.data() + kHashPrefixLength,
               kSHA256HashLength) == 0) {
      return true;
    }
  } else {
    // Look for the TLS cert fingerprint as a multihash.
    if (user_data_bytes.size() < kUserDataMinLength) {
      LOG(ERROR) << "Nitro verification: user data is not at least "
                 << kUserDataMinLength << " bytes";
      return false;
    }
    // We only support sha2-256 fingerprints.
    if (user_data_bytes[0] != kMultihashSHA256Code &&
        user_data_bytes[1] != kSHA256HashLength) {
      LOG(ERROR) << "Nitro verification: user data not a sha2-256 multihash";
      return false;
    }
    if (memcmp(server_cert_fp.data,
               user_data_bytes.data() + kMultihashPrefixLength,
               kSHA256HashLength) == 0) {
      return true;
    }
  }
  LOG(ERROR)
      << "Nitro verification: server cert fp does not match user data fp, "
      << "user data = " << base::HexEncode(user_data_bytes)
      << ", server cert fp = " << base::HexEncode(server_cert_fp.data);
  return false;
}

std::optional<bssl::ParsedCertificateList> ParseCertificatesAndCheckRoot(
    scoped_refptr<net::X509Certificate> server_cert,
    const cbor::Value::MapValue& cose_map) {
  const auto cert_it = cose_map.find(cbor::Value("certificate"));
  const auto cabundle_it = cose_map.find(cbor::Value("cabundle"));
  if (cert_it == cose_map.end() || cabundle_it == cose_map.end() ||
      !cabundle_it->second.is_array()) {
    LOG(ERROR) << "Nitro verification: certificate and/or cabundle are "
               << "missing or not the right type";
    return std::nullopt;
  }

  const cbor::Value::ArrayValue& cabundle_arr = cabundle_it->second.GetArray();

  std::vector<cbor::Value> cert_vals;
  cert_vals.push_back(cert_it->second.Clone());
  std::transform(cabundle_arr.rbegin(), cabundle_arr.rend(),
                 std::back_inserter(cert_vals),
                 [](const cbor::Value& val) { return val.Clone(); });

  bssl::ParsedCertificateList cert_chain;

  bssl::ParseCertificateOptions parse_cert_options;
  // Nitro enclave certs seem to contain serial numbers that Chromium does not
  // like, so we disable serial number validation
  parse_cert_options.allow_invalid_serial_numbers = true;
  bssl::CertErrors cert_errors;
  for (auto& cert_val : cert_vals) {
    if (!cert_val.is_bytestring()) {
      LOG(ERROR) << "Nitro verification: certificate is not bstr";
      return std::nullopt;
    }
    if (!bssl::ParsedCertificate::CreateAndAddToVector(
            net::x509_util::CreateCryptoBuffer(cert_val.GetBytestring()),
            parse_cert_options, &cert_chain, &cert_errors)) {
      LOG(ERROR) << "Nitro verification: failed to parse certificate: "
                 << cert_errors.ToDebugString();
      return std::nullopt;
    }
  }

  net::SHA256HashValue root_cert_fp =
      net::X509Certificate::CalculateFingerprint256(
          cert_chain.back()->cert_buffer());
  if (root_cert_fp != kAWSRootCertFP) {
    LOG(ERROR)
        << "Nitro verification: root cert fp does not match AWS root cert fp";
    return std::nullopt;
  }

  return std::make_optional(cert_chain);
}

void ParseAndVerifyDocument(
    std::unique_ptr<network::SimpleURLLoader> url_loader,
    std::vector<uint8_t> nonce,
    base::OnceCallback<void(scoped_refptr<net::X509Certificate>)>
        result_callback,
    std::unique_ptr<std::string> response_body) {
  if (response_body == nullptr || response_body->empty()) {
    LOG(ERROR) << "Nitro verification: no body received from server";
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  std::string_view trimmed_body =
      base::TrimWhitespaceASCII(*response_body, base::TrimPositions::TRIM_ALL);
  std::optional<std::vector<uint8_t>> cose_encoded =
      base::Base64Decode(trimmed_body);
  if (!cose_encoded.has_value()) {
    LOG(ERROR) << "Nitro verification: Failed to decode base64 document";
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  CoseSign1 cose_doc;

  if (!cose_doc.DecodeFromBytes(*cose_encoded) ||
      !cose_doc.payload().is_map()) {
    LOG(ERROR) << "Nitro verification: Failed to decode COSE/CBOR document";
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  const cbor::Value::MapValue& cose_map = cose_doc.payload().GetMap();

  if (!VerifyNonce(cose_map, nonce)) {
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  const network::mojom::URLResponseHead* response_info =
      url_loader->ResponseInfo();
  if (!response_info || !response_info->ssl_info.has_value() ||
      response_info->ssl_info->cert == nullptr) {
    LOG(ERROR) << "Nitro verification: ssl info is missing from response info";
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }
  scoped_refptr<net::X509Certificate> server_cert =
      response_info->ssl_info->cert;

  if (!VerifyUserDataKey(server_cert, cose_map)) {
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  std::optional<bssl::ParsedCertificateList> cert_chain =
      ParseCertificatesAndCheckRoot(server_cert, cose_map);
  if (!cert_chain.has_value()) {
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  if (!cose_doc.Verify(*cert_chain)) {
    LOG(ERROR) << "Nitro verification: COSE verification failed";
    std::move(result_callback).Run(scoped_refptr<net::X509Certificate>());
    return;
  }

  std::move(result_callback).Run(server_cert);
}

}  // namespace

void RequestAndVerifyAttestationDocument(
    const GURL& attestation_url,
    network::mojom::URLLoaderFactory* url_loader_factory,
    base::OnceCallback<void(scoped_refptr<net::X509Certificate>)>
        result_callback) {
  std::vector<uint8_t> nonce(20);
  crypto::RandBytes(nonce);

  std::unique_ptr<network::ResourceRequest> resource_request =
      std::make_unique<network::ResourceRequest>();
  std::string nonce_hex = base::ToLowerASCII(base::HexEncode(nonce));
  resource_request->url =
      net::AppendQueryParameter(attestation_url, "nonce", nonce_hex);

  std::unique_ptr<network::SimpleURLLoader> url_loader =
      network::SimpleURLLoader::Create(std::move(resource_request),
                                       AttestationAnnotation());
  url_loader->SetURLLoaderFactoryOptions(
      network::mojom::kURLLoadOptionSendSSLInfoWithResponse);
  url_loader->DownloadToString(
      url_loader_factory,
      base::BindOnce(&ParseAndVerifyDocument, std::move(url_loader), nonce,
                     std::move(result_callback)),
      kAttestationBodyMaxSize);
}

bool VerifyNonceForTesting(const std::vector<uint8_t>& attestation_bytes,
                           const std::vector<uint8_t>& orig_nonce) {
  auto document = cbor::Reader::Read(attestation_bytes);
  if (!document.has_value() || !document.value().is_map()) {
    LOG(ERROR) << "Nitro verification: expected cbor map for test";
    return false;
  }
  return VerifyNonce(document.value().GetMap(), orig_nonce);
}

bool VerifyUserDataKeyForTesting(
    const std::vector<uint8_t>& attestation_bytes,
    scoped_refptr<net::X509Certificate> expected_cert) {
  auto document = cbor::Reader::Read(attestation_bytes);
  if (!document.has_value() || !document.value().is_map()) {
    LOG(ERROR) << "Nitro verification: expected cbor map for test";
    return false;
  }
  return VerifyUserDataKey(expected_cert, document.value().GetMap());
}

}  // namespace nitro_utils
