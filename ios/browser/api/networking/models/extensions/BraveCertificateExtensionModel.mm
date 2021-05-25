/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateExtensionModel.h"
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

@implementation BraveCertificateExtensionModel
- (instancetype)initWithType:(NSUInteger)type withExtension:(X509_EXTENSION*)extension {
  if ((self = [super init])) {
    _type = type;
    _isCritical = X509_EXTENSION_get_critical(extension);
    
    ASN1_OBJECT* object = X509_EXTENSION_get_object(extension);
    if (object) {
      _name = brave::string_to_ns(x509_utils::string_from_ASN1_OBJECT(object, false));
      _nid = OBJ_obj2nid(object);
      _title = brave::string_to_ns(OBJ_nid2ln(static_cast<int>(_nid)));
    } else {
      _name = [[NSString alloc] init];
      _nid = NID_undef;
      _title = [[NSString alloc] init];
    }

    [self parseExtension:extension];
  }
  return self;
}

- (void)parseExtension:(X509_EXTENSION*)extension {
  NSAssert(NO, @"SHOULD NOT BE CALLED! - Subclass needs to implement this function");
}
@end
