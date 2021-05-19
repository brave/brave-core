/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateExtendedKeyUsageExtensionModel.h"
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


@implementation BraveCertificateExtensionExtendedKeyUsageModel
- (instancetype)initWithNID:(NSInteger)nid name:(NSString*)name keyUsage:(BraveExtendedKeyUsage)keyUsage {
  if ((self = [super init])) {
    _nid = nid;
    _name = name;
    _keyUsage = keyUsage;
  }
  return self;
}
@end

@implementation BraveCertificateExtendedKeyUsageExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  NSMutableArray* key_usages = [[NSMutableArray alloc] init];
  EXTENDED_KEY_USAGE* key_usage = static_cast<EXTENDED_KEY_USAGE*>(X509V3_EXT_d2i(extension));
  if (key_usage) {
    for (std::size_t i = 0; i < sk_ASN1_OBJECT_num(key_usage); ++i) {
      ASN1_OBJECT* value = sk_ASN1_OBJECT_value(key_usage, static_cast<int>(i));
      if (value) {
        int usage_nid = OBJ_obj2nid(value);
        const char* name = OBJ_nid2sn(usage_nid);
        BraveExtendedKeyUsage purpose = brave::convert_extended_key_usage(usage_nid);
        
        auto* usage = [[BraveCertificateExtensionExtendedKeyUsageModel alloc] initWithNID:usage_nid
                                                          name:brave::string_to_ns(name)
                                                             keyUsage:purpose];
        [key_usages addObject:usage];
      }
    }
    EXTENDED_KEY_USAGE_free(key_usage);
  }
  _keyPurposes = key_usages;
}
@end
