/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/net/certificate_utility.h"

#include <limits>
#include <memory>
#include <vector>

#include "base/base64.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#import "base/mac/foundation_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#import "base/notreached.h"
#include "base/strings/sys_string_conversions.h"

#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/cert/cert_verify_proc_ios.h"
#include "net/cert/cert_verify_result.h"
#include "net/cert/crl_set.h"
#include "net/cert/pem.h"
#include "net/cert/pki/cert_errors.h"
#include "net/cert/pki/parsed_certificate.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/cert/x509_util_apple.h"
#include "net/der/input.h"
#include "net/http/transport_security_state.h"
#include "net/log/net_log_with_source.h"
#include "net/net_buildflags.h"
#include "net/tools/transport_security_state_generator/cert_util.h"
#include "net/tools/transport_security_state_generator/spki_hash.h"

#include "third_party/boringssl/src/include/openssl/mem.h"
#include "third_party/boringssl/src/include/openssl/sha.h"
#include "third_party/boringssl/src/include/openssl/x509.h"

namespace net {
namespace {
#if BUILDFLAG(INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST)
#include "net/http/transport_security_state_static.h"  // nogncheck
#endif  // INCLUDE_TRANSPORT_SECURITY_STATE_PRELOAD_LIST
}  // namespace
}  // namespace net

@implementation BraveCertificateUtility
+ (NSArray<NSData*>*)acceptableSPKIHashes {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  for (std::size_t i = 0; i < sizeof(net::kBraveAcceptableCerts) /
                                  sizeof(net::kBraveAcceptableCerts[0]);
       ++i) {
    if (net::kBraveAcceptableCerts[i]) {
      std::string data = std::string(net::kBraveAcceptableCerts[i]);
      if (data.size() > 0) {
        [result addObject:[NSData dataWithBytes:&data[0] length:data.size()]];
      }
    }
  }
  return result;
}

+ (NSString*)pemEncodeCertificate:(SecCertificateRef)certificate {
  base::ScopedCFTypeRef<CFDataRef> cert_data =
      base::ScopedCFTypeRef<CFDataRef>(SecCertificateCopyData(certificate));
  if (!cert_data) {
    return nil;
  }

  bssl::UniquePtr<CRYPTO_BUFFER> cert_buffer(
      net::X509Certificate::CreateCertBufferFromBytes(base::make_span(
          CFDataGetBytePtr(cert_data),
          base::checked_cast<size_t>(CFDataGetLength(cert_data)))));

  if (!cert_buffer) {
    return nil;
  }

  std::string pem_encoded;
  net::X509Certificate::GetPEMEncoded(cert_buffer.get(), &pem_encoded);
  if (pem_encoded.empty()) {
    return nil;
  }

  return base::SysUTF8ToNSString(pem_encoded);
}

+ (NSData*)hashCertificateSPKI:(SecCertificateRef)certificate {
  base::ScopedCFTypeRef<CFDataRef> cert_data =
      base::ScopedCFTypeRef<CFDataRef>(SecCertificateCopyData(certificate));
  if (!cert_data) {
    return nil;
  }

  bssl::UniquePtr<CRYPTO_BUFFER> cert_buffer(
      net::X509Certificate::CreateCertBufferFromBytes(base::make_span(
          CFDataGetBytePtr(cert_data),
          base::checked_cast<size_t>(CFDataGetLength(cert_data)))));

  if (!cert_buffer) {
    return nil;
  }

  net::CertErrors errors;
  std::shared_ptr<const net::ParsedCertificate> extended_cert =
      std::shared_ptr<const net::ParsedCertificate>(
          net::ParsedCertificate::Create(
              std::move(cert_buffer),
              net::x509_util::DefaultParseCertificateOptions() /* {} */,
              &errors));

  if (!extended_cert) {
    VLOG(1) << errors.ToDebugString();
    return nil;
  }

  std::uint8_t data[32] = {0};
  net::der::Input spki = extended_cert->tbs().spki_tlv;
  SHA256(spki.UnsafeData(), spki.Length(), data);

  return [NSData dataWithBytes:data length:sizeof(data) / sizeof(data[0])];
}

+ (int)verifyTrust:(SecTrustRef)trust
              host:(NSString*)host
              port:(NSInteger)port {
  // Get the chain of Trust
  // Create a certificate with all the intermediates from the Trust
  auto create_cert_from_trust =
      [](SecTrustRef trust) -> scoped_refptr<net::X509Certificate> {
    if (!trust) {
      return nullptr;
    }

    CFIndex cert_count = SecTrustGetCertificateCount(trust);
    if (cert_count == 0) {
      return nullptr;
    }

    std::vector<base::ScopedCFTypeRef<SecCertificateRef>> intermediates;

    base::ScopedCFTypeRef<CFArrayRef> certificateChain(
        SecTrustCopyCertificateChain(trust));
    for (CFIndex i = 1; i < cert_count; i++) {
      SecCertificateRef secCertificate =
          base::mac::CFCastStrict<SecCertificateRef>(
              CFArrayGetValueAtIndex(certificateChain, i));
      intermediates.emplace_back(secCertificate, base::scoped_policy::RETAIN);
    }
    SecCertificateRef secCertificate =
        base::mac::CFCastStrict<SecCertificateRef>(
            CFArrayGetValueAtIndex(certificateChain, 0));
    return net::x509_util::CreateX509CertificateFromSecCertificate(
        base::ScopedCFTypeRef<SecCertificateRef>(secCertificate,
                                                 base::scoped_policy::RETAIN),
        intermediates);
  };

  auto cert = create_cert_from_trust(trust);
  if (!cert) {
    return net::ERR_FAILED;
  }

  // Validate the chain of Trust
  net::CertVerifyResult verify_result;
  scoped_refptr<net::CRLSet> crl_set = net::CRLSet::BuiltinCRLSet();
  scoped_refptr<net::CertVerifyProc> verifier =
      base::MakeRefCounted<net::CertVerifyProcIOS>();
  verifier->Verify(cert.get(), base::SysNSStringToUTF8(host),
                   /*ocsp_response=*/std::string(),
                   /*sct_list=*/std::string(),
                   /*flags=*/0,
                   /*crl_set=*/crl_set.get(),
                   /*additional_trust_anchors=*/net::CertificateList(),
                   &verify_result, net::NetLogWithSource());

  // Create a transport security state to pin certificates
  auto transport_security_state =
      std::make_unique<net::TransportSecurityState>();
  if (!transport_security_state) {
    return net::ERR_FAILED;
  }

  // Get the PKPState to check if pins are available for the specified domain
  net::TransportSecurityState::PKPState pkp_state;
  transport_security_state->GetPKPState(base::SysNSStringToUTF8(host),
                                        &pkp_state);

  // Nothing to pin against for this domain
  if (!pkp_state.HasPublicKeyPins()) {
    return std::numeric_limits<std::int32_t>::min();
  }

  // Check the Public Key Pins to see if the certificate chain is valid
  // For this, we use the verification result above
  std::string failure_log;
  net::NetworkAnonymizationKey network_anonymization_key =
      net::NetworkAnonymizationKey::CreateTransient();

  net::TransportSecurityState::PKPStatus status =
      transport_security_state->CheckPublicKeyPins(
          net::HostPortPair(base::SysNSStringToUTF8(host), port),
          verify_result.is_issued_by_known_root,
          verify_result.public_key_hashes, cert.get(),
          verify_result.verified_cert.get(),
          net::TransportSecurityState::ENABLE_PIN_REPORTS,
          network_anonymization_key, &failure_log);
  switch (status) {
    case net::TransportSecurityState::PKPStatus::VIOLATED:
      return net::ERR_FAILED;
    case net::TransportSecurityState::PKPStatus::OK:
      return net::OK;
    case net::TransportSecurityState::PKPStatus::BYPASSED:
      return net::ERR_FAILED;
    default:
      break;
  }

  // If the status is none of the above, we fall back to the verification result
  return verify_result.cert_status;
}
@end
