/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificatePKIXOCSPNonceExtensionModel.h"
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#else
  #import "brave_certificate_enums.h"
  #include "brave_certificate_ios_utils.h"
  #include "brave_certificate_utils.h"
#endif

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
#else
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
#endif


@implementation BraveCertificatePKIXOCSPNonceExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _nonce = [[NSString alloc] init];
  
  ASN1_OCTET_STRING* nonce = static_cast<ASN1_OCTET_STRING*>(X509V3_EXT_d2i(extension));
  if (nonce) {
    _nonce = brave::string_to_ns(x509_utils::string_from_ASN1STRING(nonce));
    ASN1_OCTET_STRING_free(nonce);
  }
}
@end
