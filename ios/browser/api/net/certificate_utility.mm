/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/net/certificate_utility.h"

#include <algorithm>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/containers/span.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/strings/sys_string_conversions.h"

#include "net/cert/pem.h"
#include "net/cert/pki/cert_errors.h"
#include "net/cert/pki/parsed_certificate.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"
#include "net/der/input.h"
#include "net/http/transport_security_state.h"
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
          CFDataGetBytePtr(cert_data), CFDataGetLength(cert_data))));

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
          CFDataGetBytePtr(cert_data), CFDataGetLength(cert_data))));

  if (!cert_buffer) {
    return nil;
  }

  net::CertErrors errors;
  scoped_refptr<net::ParsedCertificate> extended_cert =
      scoped_refptr<net::ParsedCertificate>(net::ParsedCertificate::Create(
          std::move(cert_buffer),
          net::x509_util::DefaultParseCertificateOptions() /* {} */, &errors));

  if (!extended_cert) {
    VLOG(1) << errors.ToDebugString();
    return nil;
  }

  std::uint8_t data[32] = {0};
  net::der::Input spki = extended_cert->tbs().spki_tlv;
  SHA256(spki.UnsafeData(), spki.Length(), data);

  return [NSData dataWithBytes:data length:sizeof(data) / sizeof(data[0])];
}
@end
