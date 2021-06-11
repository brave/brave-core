/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateNetscapeURLExtensionModel.h"
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


@implementation BraveCertificateNetscapeURLExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _url = [[NSString alloc] init];
  
  ASN1_IA5STRING* url = static_cast<ASN1_IA5STRING*>(X509V3_EXT_d2i(extension));
  if (url) {
    _url = brave::string_to_ns(x509_utils::string_from_ASN1STRING(url));
    ASN1_IA5STRING_free(url);
  }
}
@end
