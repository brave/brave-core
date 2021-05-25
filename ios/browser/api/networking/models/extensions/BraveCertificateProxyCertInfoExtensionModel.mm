/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateProxyCertInfoExtensionModel.h"
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

@interface BraveCertificateProxyPolicyExtensionModel()
@property(nonatomic, strong, readwrite) NSString* language;
@property(nonatomic, strong, readwrite) NSString* policyText;
@end

@implementation BraveCertificateProxyPolicyExtensionModel
- (instancetype)init {
  if ((self = [super init])) {
    _language = [[NSString alloc] init];
    _policyText = [[NSString alloc] init];
  }
  return self;
}
@end

@implementation BraveCertificateProxyCertInfoExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _pathLengthConstraint = -1; // infinite - spec defines -1 as infinite instead of NSIntegerMax
  
  PROXY_CERT_INFO_EXTENSION* proxy_cert_info = static_cast<PROXY_CERT_INFO_EXTENSION*>(X509V3_EXT_d2i(extension));
  if (proxy_cert_info) {
    if (proxy_cert_info->pcPathLengthConstraint) {
      _pathLengthConstraint = ASN1_INTEGER_get(proxy_cert_info->pcPathLengthConstraint);
    }
    
    if (proxy_cert_info->proxyPolicy) {
      _proxyPolicy = [[BraveCertificateProxyPolicyExtensionModel alloc] init];
      
      ASN1_OBJECT* policy_language = proxy_cert_info->proxyPolicy->policyLanguage;
      if (policy_language) {
        _proxyPolicy.language = brave::string_to_ns(
                                    x509_utils::string_from_ASN1_OBJECT(policy_language, false));
      }
      
      ASN1_OCTET_STRING* policy_text = proxy_cert_info->proxyPolicy->policy;
      if (policy_text) {
        _proxyPolicy.policyText = brave::string_to_ns(
                                      x509_utils::string_from_ASN1STRING(policy_text));
      }
    }
    PROXY_CERT_INFO_EXTENSION_free(proxy_cert_info);
  }
}
@end
