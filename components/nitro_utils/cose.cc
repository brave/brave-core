/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/nitro_utils/cose.h"

#include <algorithm>
#include <set>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "components/cbor/reader.h"
#include "components/cbor/writer.h"
#include "crypto/sha2.h"
#include "crypto/signature_verifier.h"
#include "net/cert/asn1_util.h"
#include "net/cert/internal/trust_store.h"
#include "net/der/encode_values.h"
#include "third_party/boringssl/src/include/openssl/bn.h"
#include "third_party/boringssl/src/include/openssl/bytestring.h"
#include "third_party/boringssl/src/include/openssl/ecdsa.h"

namespace nitro_utils {

namespace {

constexpr int kCoseES384AlgorithmValue = -35;
constexpr int kSignatureComponentSize = 48;

bool ConvertCoseSignatureToDER(const std::vector<uint8_t>& input,
                               std::vector<uint8_t>* output) {
  if (input.size() != (kSignatureComponentSize * 2)) {
    LOG(ERROR) << "COSE: bad signature size";
    return false;
  }

  BIGNUM* r_comp = BN_bin2bn(input.data(), kSignatureComponentSize, nullptr);
  DCHECK(r_comp);
  BIGNUM* s_comp = BN_bin2bn(input.data() + kSignatureComponentSize,
                             kSignatureComponentSize, nullptr);
  DCHECK(s_comp);

  ECDSA_SIG* ecdsa_sig = ECDSA_SIG_new();
  DCHECK(ecdsa_sig);

  if (ECDSA_SIG_set0(ecdsa_sig, r_comp, s_comp) != 1) {
    LOG(ERROR) << "COSE: Failed to construct ECDSA SIG struct";
    BN_free(r_comp);
    BN_free(s_comp);
    ECDSA_SIG_free(ecdsa_sig);
    return false;
  }

  CBB sig_cbb;
  DCHECK_EQ(CBB_init(&sig_cbb, 0), 1);

  if (ECDSA_SIG_marshal(&sig_cbb, ecdsa_sig) != 1) {
    LOG(ERROR) << "COSE: Failed to marshal ECDSA SIG struct";
    CBB_cleanup(&sig_cbb);
    ECDSA_SIG_free(ecdsa_sig);
    return false;
  }

  const uint8_t* sig_cbb_data = CBB_data(&sig_cbb);
  *output =
      std::vector<uint8_t>(sig_cbb_data, sig_cbb_data + CBB_len(&sig_cbb));

  CBB_cleanup(&sig_cbb);
  ECDSA_SIG_free(ecdsa_sig);

  return true;
}

}  // namespace

CoseSign1::CoseSign1() {}
CoseSign1::~CoseSign1() {}

bool CoseSign1::DecodeFromBytes(const std::vector<uint8_t>& data) {
  cbor::Reader::Config cbor_config;
  cbor_config.allow_and_canonicalize_out_of_order_keys = true;

  absl::optional<cbor::Value> decoded_val =
      cbor::Reader::Read(data, cbor_config);
  if (cbor_config.error_code_out != nullptr &&
      *cbor_config.error_code_out !=
          cbor::Reader::DecoderError::CBOR_NO_ERROR) {
    LOG(ERROR) << "COSE: failed to read root encoded cbor: "
               << cbor::Reader::ErrorCodeToString(
                      *(cbor_config.error_code_out));
    return false;
  }
  DCHECK(decoded_val.has_value());

  if (!decoded_val->is_array() || decoded_val->GetArray().size() != 4) {
    LOG(ERROR) << "COSE: root decoded cbor is not array, or has incorrect size";
    return false;
  }

  const cbor::Value::ArrayValue& cose_arr = decoded_val->GetArray();

  protected_encoded = cose_arr[0].Clone();

  if (!protected_encoded.is_bytestring()) {
    LOG(ERROR) << "COSE: protected value is not bstr";
    return false;
  }

  absl::optional<cbor::Value> protected_decoded_val =
      cbor::Reader::Read(protected_encoded.GetBytestring(), cbor_config);
  if (cbor_config.error_code_out != nullptr &&
      *cbor_config.error_code_out !=
          cbor::Reader::DecoderError::CBOR_NO_ERROR) {
    LOG(ERROR) << "COSE: failed to read protected cbor: "
               << cbor::Reader::ErrorCodeToString(
                      *(cbor_config.error_code_out));
    return false;
  }
  DCHECK(protected_decoded_val.has_value());

  if (!protected_decoded_val->is_map()) {
    LOG(ERROR) << "COSE: protected value is not a map";
    return false;
  }

  protected_headers = protected_decoded_val->Clone();
  const cbor::Value::MapValue& protected_headers_map =
      protected_headers.GetMap();
  cbor::Value::MapValue::const_iterator alg_value_it =
      protected_headers_map.find(cbor::Value(1));

  if (alg_value_it == protected_headers_map.end() ||
      !alg_value_it->second.is_integer()) {
    LOG(ERROR) << "COSE: protected alg value is missing, or is not an integer";
    return false;
  }

  if (alg_value_it->second.GetInteger() != kCoseES384AlgorithmValue) {
    LOG(ERROR) << "COSE: Bad algo, only ES384 is supported";
    return false;
  }

  const cbor::Value& unprotected_val = cose_arr[1];
  if (!unprotected_val.is_map()) {
    LOG(ERROR) << "COSE: unprotected value is not map";
    return false;
  }
  unprotected_headers = unprotected_val.Clone();

  payload_encoded = cose_arr[2].Clone();
  if (!payload_encoded.is_bytestring()) {
    LOG(ERROR) << "COSE: inner payload value is not bstr";
    return false;
  }

  absl::optional<cbor::Value> payload_dec_val =
      cbor::Reader::Read(payload_encoded.GetBytestring(), cbor_config);
  if (cbor_config.error_code_out != nullptr &&
      *cbor_config.error_code_out !=
          cbor::Reader::DecoderError::CBOR_NO_ERROR) {
    LOG(ERROR) << "COSE: failed to read payload cbor: "
               << cbor::Reader::ErrorCodeToString(*cbor_config.error_code_out);
    return false;
  }
  DCHECK(payload_dec_val.has_value());
  payload = payload_dec_val->Clone();

  const cbor::Value& signature_val = cose_arr[3];
  if (!signature_val.is_bytestring()) {
    LOG(ERROR) << "COSE: signature value is not bstr";
    return false;
  }
  signature = signature_val.GetBytestring();

  return true;
}

bool CoseSign1::Verify(const net::ParsedCertificateList& cert_chain) {
  DCHECK_GT(cert_chain.size(), 1U);

  net::der::GeneralizedTime time_now;
  DCHECK(net::der::EncodeTimeAsGeneralizedTime(base::Time::Now(), &time_now));

  net::CertPathErrors cert_path_errors;

  net::VerifyCertificateChain(
      cert_chain, net::CertificateTrust::ForTrustAnchorEnforcingExpiration(),
      this, time_now, net::KeyPurpose::ANY_EKU,
      net::InitialExplicitPolicy::kFalse, std::set<net::der::Input>(),
      net::InitialPolicyMappingInhibit::kFalse,
      net::InitialAnyPolicyInhibit::kFalse, nullptr, &cert_path_errors);

  if (cert_path_errors.ContainsHighSeverityErrors()) {
    LOG(ERROR) << "COSE verification: bad certificate chain: "
               << cert_path_errors.ToDebugString(cert_chain);
    return false;
  }

  std::vector<cbor::Value> sig_data_vec;
  sig_data_vec.emplace_back(cbor::Value("Signature1"));
  sig_data_vec.push_back(protected_encoded.Clone());
  sig_data_vec.emplace_back(cbor::Value(std::vector<uint8_t>()));
  sig_data_vec.push_back(payload_encoded.Clone());
  cbor::Value sig_data(sig_data_vec);

  absl::optional<std::vector<uint8_t>> encoded_sig_data =
      cbor::Writer::Write(sig_data);
  DCHECK(encoded_sig_data.has_value());

  base::StringPiece low_cert_spki;
  if (!net::asn1::ExtractSPKIFromDERCert(
          cert_chain.front()->der_cert().AsStringPiece(), &low_cert_spki)) {
    LOG(ERROR) << "COSE verification: could not extract SPKI from cert";
    return false;
  }

  std::vector<uint8_t> low_cert_spki_vec;
  std::transform(low_cert_spki.begin(), low_cert_spki.end(),
                 std::back_inserter(low_cert_spki_vec),
                 [](char c) { return static_cast<uint8_t>(c); });

  std::vector<uint8_t> sig_der;
  if (!ConvertCoseSignatureToDER(signature, &sig_der)) {
    return false;
  }

  crypto::SignatureVerifier sig_verifier;
  if (!sig_verifier.VerifyInit(
          crypto::SignatureVerifier::SignatureAlgorithm::ECDSA_SHA384, sig_der,
          low_cert_spki_vec)) {
    LOG(ERROR) << "COSE verification: failed to init cert verifier";
    return false;
  }

  sig_verifier.VerifyUpdate(*encoded_sig_data);

  return sig_verifier.VerifyFinal();
}

bool CoseSign1::IsSignatureAlgorithmAcceptable(
    const net::SignatureAlgorithm& signature_algorithm,
    net::CertErrors* errors) {
  return true;
}

bool CoseSign1::IsPublicKeyAcceptable(EVP_PKEY* public_key,
                                      net::CertErrors* errors) {
  return true;
}

}  // namespace nitro_utils
