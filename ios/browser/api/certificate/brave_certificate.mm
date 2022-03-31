/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/brave_certificate.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_enums.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_fingerprint.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_model+private.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_public_key_info.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_rdns_sequence.h"
#include "brave/ios/browser/api/certificate/models/brave_certificate_signature.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_utils.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"
#include "net/base/net_export.h"
#include "net/cert/internal/cert_errors.h"
#include "net/cert/internal/parsed_certificate.h"
#include "net/cert/internal/verify_name_match.h"
#include "net/cert/internal/verify_signed_data.h"
#include "net/cert/x509_cert_types.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/cert/x509_util_ios.h"
#include "net/der/input.h"
#include "third_party/boringssl/src/include/openssl/pool.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation

@interface BraveCertificateModel () {
  base::ScopedCFTypeRef<CFDataRef> cert_data_;
  scoped_refptr<net::ParsedCertificate> extended_cert_;
  base::ScopedCFTypeRef<SecKeyRef> public_key_;
}
@end

@implementation BraveCertificateModel
- (nullable instancetype)initWithCertificate:(SecCertificateRef)certificate {
  if ((self = [super init])) {
    cert_data_ =
        base::ScopedCFTypeRef<CFDataRef>(SecCertificateCopyData(certificate));
    if (!cert_data_) {
      return nullptr;
    }

    public_key_ =
        base::ScopedCFTypeRef<SecKeyRef>(SecCertificateCopyKey(certificate));

    bssl::UniquePtr<CRYPTO_BUFFER> cert_buffer(
        net::X509Certificate::CreateCertBufferFromBytes(base::make_span(
            CFDataGetBytePtr(cert_data_), CFDataGetLength(cert_data_))));

    if (!cert_buffer) {
      return nullptr;
    }

    net::CertErrors errors;
    extended_cert_ =
        scoped_refptr<net::ParsedCertificate>(net::ParsedCertificate::Create(
            std::move(cert_buffer),
            net::x509_util::DefaultParseCertificateOptions() /* {} */,
            &errors));

    if (!extended_cert_) {
      VLOG(1) << errors.ToDebugString();
      return nullptr;
    }

    [self parseCertificate];

    cert_data_.reset();
    extended_cert_.reset();
    public_key_.reset();
  }
  return self;
}

- (void)dealloc {
  extended_cert_.reset();
}

- (void)parseCertificate {
  std::string serial_number_string =
      extended_cert_->tbs().serial_number.AsString();

  _isRootCertificate = [self is_root_certificate];
  _isCertificateAuthority = [self is_certificate_authority];
  _isSelfSigned = [self is_self_signed];
  _isSelfIssued = [self is_self_issued];
  _subjectName = [[BraveCertificateRDNSequence alloc]
      initWithBERName:extended_cert_->tbs().subject_tlv
             uniqueId:extended_cert_->tbs().subject_unique_id];
  _issuerName = [[BraveCertificateRDNSequence alloc]
      initWithBERName:extended_cert_->tbs().issuer_tlv
             uniqueId:extended_cert_->tbs().issuer_unique_id];
  _serialNumber = base::SysUTF8ToNSString(base::HexEncode(
      serial_number_string.data(), serial_number_string.size()));
  _version = static_cast<std::int32_t>(extended_cert_->tbs().version) +
             1;  // version + 1
  _signature = [[BraveCertificateSignature alloc]
      initWithCertificate:extended_cert_.get()];
  _notValidBefore = certificate::x509_utils::GeneralizedTimeToTime(
                        extended_cert_->tbs().validity_not_before)
                        .ToNSDate();
  _notValidAfter = certificate::x509_utils::GeneralizedTimeToTime(
                       extended_cert_->tbs().validity_not_after)
                       .ToNSDate();
  _publicKeyInfo = [[BraveCertificatePublicKeyInfo alloc]
      initWithCertificate:extended_cert_.get()
                  withKey:public_key_.get()];
  //  _extensions = nullptr;
  _sha1Fingerprint = [[BraveCertificateFingerprint alloc]
      initWithCertificate:cert_data_.get()
                 withType:BraveFingerprintType_SHA1];
  _sha256Fingerprint = [[BraveCertificateFingerprint alloc]
      initWithCertificate:cert_data_.get()
                 withType:BraveFingerprintType_SHA256];

  [self parseExtensions];
}

- (bool)is_root_certificate {
  net::CertErrors errors;
  std::string normalized_subject;
  net::der::Input subject_value = extended_cert_->normalized_subject();
  if (!net::NormalizeName(subject_value, &normalized_subject, &errors)) {
    return false;
  }

  std::string normalized_issuer;
  net::der::Input issuer_value = extended_cert_->normalized_issuer();
  if (!net::NormalizeName(issuer_value, &normalized_issuer, &errors)) {
    return false;
  }

  return normalized_subject == normalized_issuer;
}

- (bool)is_certificate_authority {
  // return X509_check_ca(x509_cert_);
  return false;
}

- (bool)is_self_signed {
  net::CertErrors errors;
  std::string normalized_subject;
  net::der::Input subject_value = extended_cert_->normalized_subject();
  if (!net::NormalizeName(subject_value, &normalized_subject, &errors)) {
    return false;
  }

  std::string normalized_issuer;
  net::der::Input issuer_value = extended_cert_->normalized_issuer();
  if (!net::NormalizeName(issuer_value, &normalized_issuer, &errors)) {
    return false;
  }

  if (normalized_subject != normalized_issuer) {
    return false;
  }

  return net::VerifySignedData(extended_cert_->signature_algorithm(),
                               extended_cert_->tbs_certificate_tlv(),
                               extended_cert_->signature_value(),
                               extended_cert_->tbs().spki_tlv);
}

- (bool)is_self_issued {
  // return X509_get_extension_flags(x509_cert_) & EXFLAG_SI;
  return false;
}

- (void)parseExtensions {
  // TODO: Parse Extensions Here..
}
@end
