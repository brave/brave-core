/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/nitro_utils/attestation.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/p3a/nitro_utils/cose.h"
#include "crypto/random.h"
#include "net/base/url_util.h"
#include "net/cert/pki/parsed_certificate.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/url_loader_factory.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/boringssl/src/include/openssl/pool.h"

namespace nitro_utils {

namespace {

constexpr size_t kAttestationBodyMaxSize = 16384;
constexpr net::SHA256HashValue kAWSRootCertFP{
    .data = {0x64, 0x1A, 0x03, 0x21, 0xA3, 0xE2, 0x44, 0xEF, 0xE4, 0x56, 0x46,
             0x31, 0x95, 0xD6, 0x06, 0x31, 0x7E, 0xD7, 0xCD, 0xCC, 0x3C, 0x17,
             0x56, 0xE0, 0x98, 0x93, 0xF3, 0xC6, 0x8F, 0x79, 0xBB, 0x5B}};

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
  DCHECK(server_cert);

  const auto user_data_it = cose_map.find(cbor::Value("user_data"));
  if (user_data_it == cose_map.end() || !user_data_it->second.is_bytestring() ||
      user_data_it->second.GetBytestring().size() != 32) {
    LOG(ERROR) << "Nitro verification: user data is missing or is not bstr "
               << "or is not 32 bytes";
    return false;
  }
  const net::SHA256HashValue server_cert_fp =
      net::X509Certificate::CalculateFingerprint256(server_cert->cert_buffer());
  if (memcmp(server_cert_fp.data, user_data_it->second.GetBytestring().data(),
             32) != 0) {
    LOG(ERROR)
        << "Nitro verification: server cert fp does not match user data fp, "
        << "user data = "
        << base::HexEncode(user_data_it->second.GetBytestring())
        << ", server cert fp = " << base::HexEncode(server_cert_fp.data);
    return false;
  }
  return true;
}

absl::optional<net::ParsedCertificateList> ParseCertificatesAndCheckRoot(
    scoped_refptr<net::X509Certificate> server_cert,
    const cbor::Value::MapValue& cose_map) {
  const auto cert_it = cose_map.find(cbor::Value("certificate"));
  const auto cabundle_it = cose_map.find(cbor::Value("cabundle"));
  if (cert_it == cose_map.end() || cabundle_it == cose_map.end() ||
      !cabundle_it->second.is_array()) {
    LOG(ERROR) << "Nitro verification: certificate and/or cabundle are "
               << "missing or not the right type";
    return absl::nullopt;
  }

  const cbor::Value::ArrayValue& cabundle_arr = cabundle_it->second.GetArray();

  std::vector<cbor::Value> cert_vals;
  cert_vals.push_back(cert_it->second.Clone());
  std::transform(cabundle_arr.rbegin(), cabundle_arr.rend(),
                 std::back_inserter(cert_vals),
                 [](const cbor::Value& val) { return val.Clone(); });

  net::ParsedCertificateList cert_chain;

  net::ParseCertificateOptions parse_cert_options;
  // Nitro enclave certs seem to contain serial numbers that Chromium does not
  // like, so we disable serial number validation
  parse_cert_options.allow_invalid_serial_numbers = true;
  net::CertErrors cert_errors;
  for (auto& cert_val : cert_vals) {
    if (!cert_val.is_bytestring()) {
      LOG(ERROR) << "Nitro verification: certificate is not bstr";
      return absl::nullopt;
    }
    if (!net::ParsedCertificate::CreateAndAddToVector(
            net::x509_util::CreateCryptoBuffer(cert_val.GetBytestring()),
            parse_cert_options, &cert_chain, &cert_errors)) {
      LOG(ERROR) << "Nitro verification: failed to parse certificate: "
                 << cert_errors.ToDebugString();
      return absl::nullopt;
    }
  }

  net::SHA256HashValue root_cert_fp =
      net::X509Certificate::CalculateFingerprint256(
          cert_chain.back()->cert_buffer());
  if (root_cert_fp != kAWSRootCertFP) {
    LOG(ERROR)
        << "Nitro verification: root cert fp does not match AWS root cert fp";
    return absl::nullopt;
  }

  return absl::make_optional(cert_chain);
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

  base::TrimString(*response_body, " ", response_body.get());
  absl::optional<std::vector<uint8_t>> cose_encoded =
      base::Base64Decode(*response_body);
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
  DCHECK(response_info);
  if (!response_info->ssl_info.has_value() ||
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

  absl::optional<net::ParsedCertificateList> cert_chain =
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

}  // namespace nitro_utils
