/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/components/p3a/nitro_utils/cose.h"

#include <optional>
#include <set>
#include <string_view>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "components/cbor/reader.h"
#include "components/cbor/writer.h"
#include "crypto/sha2.h"
#include "crypto/signature_verifier.h"
#include "net/cert/asn1_util.h"
#include "net/cert/time_conversions.h"
#include "third_party/boringssl/src/include/openssl/bn.h"
#include "third_party/boringssl/src/include/openssl/bytestring.h"
#include "third_party/boringssl/src/include/openssl/ecdsa.h"
#include "third_party/boringssl/src/pki/trust_store.h"

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
  if (!r_comp) {
    return false;
  }
  BIGNUM* s_comp = BN_bin2bn(input.data() + kSignatureComponentSize,
                             kSignatureComponentSize, nullptr);
  if (!s_comp) {
    BN_free(r_comp);
    return false;
  }

  ECDSA_SIG* ecdsa_sig = ECDSA_SIG_new();
  if (!ecdsa_sig) {
    BN_free(r_comp);
    BN_free(s_comp);
    return false;
  }

  if (ECDSA_SIG_set0(ecdsa_sig, r_comp, s_comp) != 1) {
    LOG(ERROR) << "COSE: Failed to construct ECDSA SIG struct";
    BN_free(r_comp);
    BN_free(s_comp);
    ECDSA_SIG_free(ecdsa_sig);
    return false;
  }

  CBB sig_cbb;
  if (CBB_init(&sig_cbb, 0) != 1) {
    ECDSA_SIG_free(ecdsa_sig);
    return false;
  }

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

  std::optional<cbor::Value> decoded_val =
      cbor::Reader::Read(data, cbor_config);
  if (cbor_config.error_code_out != nullptr &&
      *cbor_config.error_code_out !=
          cbor::Reader::DecoderError::CBOR_NO_ERROR) {
    LOG(ERROR) << "COSE: failed to read root encoded cbor: "
               << cbor::Reader::ErrorCodeToString(
                      *(cbor_config.error_code_out));
    return false;
  }

  if (!decoded_val.has_value() || !decoded_val->is_array() ||
      decoded_val->GetArray().size() != 4) {
    LOG(ERROR) << "COSE: root decoded cbor is not array, or has incorrect size";
    return false;
  }

  const cbor::Value::ArrayValue& cose_arr = decoded_val->GetArray();

  protected_encoded_ = cose_arr[0].Clone();

  if (!protected_encoded_.is_bytestring()) {
    LOG(ERROR) << "COSE: protected value is not bstr";
    return false;
  }

  std::optional<cbor::Value> protected_decoded_val =
      cbor::Reader::Read(protected_encoded_.GetBytestring(), cbor_config);
  if (cbor_config.error_code_out != nullptr &&
      *cbor_config.error_code_out !=
          cbor::Reader::DecoderError::CBOR_NO_ERROR) {
    LOG(ERROR) << "COSE: failed to read protected cbor: "
               << cbor::Reader::ErrorCodeToString(
                      *(cbor_config.error_code_out));
    return false;
  }

  if (!protected_decoded_val.has_value() || !protected_decoded_val->is_map()) {
    LOG(ERROR) << "COSE: protected value is not a map";
    return false;
  }

  protected_headers_ = protected_decoded_val->Clone();
  const cbor::Value::MapValue& protected_headers_map =
      protected_headers_.GetMap();
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
  unprotected_headers_ = unprotected_val.Clone();

  payload_encoded_ = cose_arr[2].Clone();
  if (!payload_encoded_.is_bytestring()) {
    LOG(ERROR) << "COSE: inner payload value is not bstr";
    return false;
  }

  std::optional<cbor::Value> payload_dec_val =
      cbor::Reader::Read(payload_encoded_.GetBytestring(), cbor_config);
  if (!payload_dec_val.has_value() ||
      (cbor_config.error_code_out != nullptr &&
       *cbor_config.error_code_out !=
           cbor::Reader::DecoderError::CBOR_NO_ERROR)) {
    LOG(ERROR) << "COSE: failed to read payload cbor: "
               << cbor::Reader::ErrorCodeToString(*cbor_config.error_code_out);
    return false;
  }
  payload_ = payload_dec_val->Clone();

  const cbor::Value& signature_val = cose_arr[3];
  if (!signature_val.is_bytestring()) {
    LOG(ERROR) << "COSE: signature value is not bstr";
    return false;
  }
  signature_ = signature_val.GetBytestring();

  return true;
}

bool CoseSign1::Verify(const bssl::ParsedCertificateList& cert_chain) {
  CHECK_GT(cert_chain.size(), 1U);

  bssl::der::GeneralizedTime time_now;
  CHECK(net::EncodeTimeAsGeneralizedTime(base::Time::Now(), &time_now));

  bssl::CertPathErrors cert_path_errors;

  bssl::VerifyCertificateChain(
      cert_chain, bssl::CertificateTrust::ForTrustAnchor(), this, time_now,
      bssl::KeyPurpose::ANY_EKU, bssl::InitialExplicitPolicy::kFalse,
      std::set<bssl::der::Input>(), bssl::InitialPolicyMappingInhibit::kFalse,
      bssl::InitialAnyPolicyInhibit::kFalse, nullptr, &cert_path_errors);

  if (cert_path_errors.ContainsHighSeverityErrors()) {
    LOG(ERROR) << "COSE verification: bad certificate chain: "
               << cert_path_errors.ToDebugString(cert_chain);
    return false;
  }

  std::vector<cbor::Value> sig_data_vec;
  sig_data_vec.emplace_back(cbor::Value("Signature1"));
  sig_data_vec.push_back(protected_encoded_.Clone());
  sig_data_vec.emplace_back(cbor::Value(std::vector<uint8_t>()));
  sig_data_vec.push_back(payload_encoded_.Clone());
  cbor::Value sig_data(sig_data_vec);

  std::optional<std::vector<uint8_t>> encoded_sig_data =
      cbor::Writer::Write(sig_data);
  CHECK(encoded_sig_data.has_value());

  std::string_view low_cert_spki;
  if (!net::asn1::ExtractSPKIFromDERCert(
          cert_chain.front()->der_cert().AsStringView(), &low_cert_spki)) {
    LOG(ERROR) << "COSE verification: could not extract SPKI from cert";
    return false;
  }

  std::vector<uint8_t> low_cert_spki_vec(low_cert_spki.begin(),
                                         low_cert_spki.end());

  std::vector<uint8_t> sig_der;
  if (!ConvertCoseSignatureToDER(signature_, &sig_der)) {
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

const cbor::Value& CoseSign1::protected_headers() {
  return protected_headers_;
}

const cbor::Value& CoseSign1::unprotected_headers() {
  return unprotected_headers_;
}

const cbor::Value& CoseSign1::payload() {
  return payload_;
}

bool CoseSign1::IsSignatureAlgorithmAcceptable(
    bssl::SignatureAlgorithm signature_algorithm,
    bssl::CertErrors* errors) {
  return true;
}

bool CoseSign1::IsPublicKeyAcceptable(EVP_PKEY* public_key,
                                      bssl::CertErrors* errors) {
  return true;
}

bssl::SignatureVerifyCache* CoseSign1::GetVerifyCache() {
  return nullptr;
}

bool CoseSign1::AcceptPreCertificates() {
  return true;
}

}  // namespace nitro_utils
