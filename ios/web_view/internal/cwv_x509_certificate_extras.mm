// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/web_view/public/cwv_x509_certificate_extras.h"

#include "ios/web_view/internal/cwv_x509_certificate_internal.h"
#include "net/cert/x509_util_apple.h"

@implementation CWVX509Certificate (Extras)

- (SecTrustRef)createServerTrust {
  auto certificates =
      net::x509_util::CreateSecCertificateArrayForX509Certificate(
          self.internalCertificate.get());
  base::apple::ScopedCFTypeRef<SecPolicyRef> policy(
      SecPolicyCreateSSL(true, nullptr));
  SecTrustRef trustRef;
  if (SecTrustCreateWithCertificates(certificates.get(), policy.get(),
                                     &trustRef) != errSecSuccess) {
    return nil;
  }
  return trustRef;
}

@end
