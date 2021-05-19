/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificatePolicyConstraintsExtensionModel.h"
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


@implementation BraveCertificatePolicyConstraintsExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _requireExplicitPolicy = [[NSString alloc] init];
  _inhibitPolicyMapping = [[NSString alloc] init];
  
  POLICY_CONSTRAINTS* policy_constraints = static_cast<POLICY_CONSTRAINTS*>(X509V3_EXT_d2i(extension));
  if (policy_constraints) {
    if (policy_constraints->requireExplicitPolicy) {
      _requireExplicitPolicy = brave::string_to_ns(
                                        x509_utils::string_from_ASN1INTEGER(policy_constraints->requireExplicitPolicy));
    }
    
    if (policy_constraints->inhibitPolicyMapping) {
      _inhibitPolicyMapping = brave::string_to_ns(
                                       x509_utils::string_from_ASN1INTEGER(policy_constraints->inhibitPolicyMapping));
    }
    POLICY_CONSTRAINTS_free(policy_constraints);
  }
}
@end
