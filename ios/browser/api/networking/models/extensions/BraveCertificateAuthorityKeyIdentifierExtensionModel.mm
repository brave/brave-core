/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateAuthorityKeyIdentifierExtensionModel.h"
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


@implementation BraveCertificateAuthorityKeyIdentifierExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _keyId = [[NSString alloc] init];
  _serial = [[NSString alloc] init];
  NSMutableArray* issuer = [[NSMutableArray alloc] init];
  
  // STACK_OF(ASN1_OBJECT)*
  AUTHORITY_KEYID* auth_key_id = static_cast<AUTHORITY_KEYID*>(X509V3_EXT_d2i(extension));
  if (auth_key_id) {
    _keyId = brave::string_to_ns(
                              x509_utils::hex_string_from_ASN1STRING(auth_key_id->keyid));
    
    if (auth_key_id->issuer) {
      for (std::size_t i = 0; i < sk_GENERAL_NAME_num(auth_key_id->issuer); ++i) {
        GENERAL_NAME* name = sk_GENERAL_NAME_value(auth_key_id->issuer, static_cast<int>(i));
        if (name) {
          auto* converted_name = brave::convert_general_name(name);
          if (converted_name) {
            [issuer addObject:converted_name];
          }
        }
      }
    }
    if (auth_key_id->serial) {
      std::string serial = x509_utils::hex_string_from_ASN1INTEGER(auth_key_id->serial);
    }
    AUTHORITY_KEYID_free(auth_key_id);
  }
  _issuer = issuer;
}
@end
