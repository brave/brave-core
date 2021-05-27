/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificatePolicyMappingsExtensionModel.h"
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

@interface BraveCertificatePolicyMappingExtensionModel()
@property(nonatomic, strong, readwrite) NSString* issuerDomainPolicy;
@property(nonatomic, strong, readwrite) NSString* subjectDomainPolicy;
@end


@implementation BraveCertificatePolicyMappingExtensionModel
@synthesize issuerDomainPolicy = _issuerDomainPolicy;
@synthesize subjectDomainPolicy = _subjectDomainPolicy;

- (instancetype)init {
  if ((self = [super init])) {
    _issuerDomainPolicy = [[NSString alloc] init];
    _subjectDomainPolicy = [[NSString alloc] init];
  }
  return self;
}
@end

@implementation BraveCertificatePolicyMappingsExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  
  //STACK_OF(POLICY_MAPPING)
  POLICY_MAPPINGS* policy_mappings = static_cast<POLICY_MAPPINGS*>(X509V3_EXT_d2i(extension));
  if (policy_mappings) {
    for (std::size_t i = 0; i < sk_POLICY_MAPPING_num(policy_mappings); ++i) {
      POLICY_MAPPING* policy_mapping = sk_POLICY_MAPPING_value(policy_mappings, static_cast<int>(i));
      if (policy_mapping) {
        auto* mapping = [[BraveCertificatePolicyMappingExtensionModel alloc] init];
        if (policy_mapping->issuerDomainPolicy) {
          mapping.issuerDomainPolicy = brave::string_to_ns(
                                                x509_utils::string_from_ASN1_OBJECT(policy_mapping->issuerDomainPolicy, false));
        }
        
        if (policy_mapping->subjectDomainPolicy) {
          mapping.subjectDomainPolicy = brave::string_to_ns(
                                                  x509_utils::string_from_ASN1_OBJECT(policy_mapping->subjectDomainPolicy, false));
        }
        [result addObject:mapping];
      }
    }
    sk_POLICY_MAPPING_free(policy_mappings);
  }
  
  _policies = result;
}
@end
