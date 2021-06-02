/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateNameConstraintsExtensionModel.h"
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


@implementation BraveCertificateExtensionGeneralSubtreeModel
- (instancetype)init {
  if ((self = [super init])) {
    _names = @[];
    _minimum = [[NSString alloc] init];
    _maximum = [[NSString alloc] init];
  }
  return self;
}

- (void)setNames:(NSArray<BraveCertificateExtensionGeneralNameModel*>*)names {
  _names = names;
}

- (void)setMinimum:(NSString*)minimum {
  _minimum = minimum;
}

- (void)setMaximum:(NSString*)maximum {
  _maximum = maximum;
}
@end

@implementation BraveCertificateNameConstraintsExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _permittedSubtrees = @[];
  _excludedSubtrees = @[];
  
  NAME_CONSTRAINTS* name_constraints = static_cast<NAME_CONSTRAINTS*>(X509V3_EXT_d2i(extension));
  if (name_constraints) {
    if (name_constraints->permittedSubtrees) {
      _permittedSubtrees = [self parseSubtrees:name_constraints->permittedSubtrees];
    }
    
    if (name_constraints->excludedSubtrees) {
      _excludedSubtrees = [self parseSubtrees:name_constraints->excludedSubtrees];
    }
    NAME_CONSTRAINTS_free(name_constraints);
  }
}

- (NSArray*)parseSubtrees:(STACK_OF(GENERAL_SUBTREE)*)subtrees {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  if (!subtrees) {
    return result;
  }
  
  for (std::size_t i = 0; i < sk_GENERAL_SUBTREE_num(subtrees); ++i) {
    GENERAL_SUBTREE* subtree = sk_GENERAL_SUBTREE_value(subtrees, static_cast<int>(i));
    if (subtree) {
      auto* tree = [[BraveCertificateExtensionGeneralSubtreeModel alloc] init];
      
      NSMutableArray* names = [[NSMutableArray alloc] init];
      if (subtree->base) {
        [names addObject:brave::convert_general_name(subtree->base)];
      }
      [tree setNames:names];
      
      if (subtree->minimum) {
        [tree setMinimum:brave::string_to_ns(x509_utils::string_from_ASN1INTEGER(subtree->minimum))];
      }
      
      if (subtree->maximum) {
        [tree setMaximum:brave::string_to_ns(x509_utils::string_from_ASN1INTEGER(subtree->maximum))];
      }
      
      [result addObject:tree];
    }
  }
  return result;
}
@end
