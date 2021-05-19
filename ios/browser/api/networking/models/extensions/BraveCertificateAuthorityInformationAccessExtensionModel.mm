/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateAuthorityInformationAccessExtensionModel.h"
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#else
  #import "brave_certificate_enums.h"
  #include "brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#endif

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
#else
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
#endif


@implementation BraveCertificateAuthorityInformationAccessExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _oid_name = [[NSString alloc] init];
  _oid = [[NSString alloc] init];
  NSMutableArray* locations = [[NSMutableArray alloc] init];
  
  //STACK_OF(ACCESS_DESCRIPTION)
  AUTHORITY_INFO_ACCESS* info_access = static_cast<AUTHORITY_INFO_ACCESS*>(X509V3_EXT_d2i(extension));
  if (info_access) {
    for (std::size_t i = 0; i < sk_ACCESS_DESCRIPTION_num(info_access); ++i) {
      ACCESS_DESCRIPTION* desc = sk_ACCESS_DESCRIPTION_value(info_access, static_cast<int>(i));
      if (desc) {
        _oid_name = brave::string_to_ns(x509_utils::string_from_ASN1_OBJECT(desc->method, false));
        _oid = brave::string_to_ns(x509_utils::string_from_ASN1_OBJECT(desc->method, true));

        if (desc->location) {
          [locations addObject:brave::convert_general_name(desc->location)];
        }
      }
    }
    AUTHORITY_INFO_ACCESS_free(info_access);
  }
  _locations = locations;
}
@end
