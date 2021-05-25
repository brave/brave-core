/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateSignature.h"
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#else
  #import "brave_certificate_enums.h"
  #include "brave_certificate_ios_utils.h"
  #include "brave_certificate_utils.h"
#endif

@implementation BraveCertificateSignature
- (instancetype)initWithCertificate:(X509*)certificate {
  if ((self = [super init])) {
    _algorithm = [[NSString alloc] init];
    _objectIdentifier = [[NSString alloc] init];
    _signatureHexEncoded = [[NSString alloc] init];
    _parameters = [[NSString alloc] init];
    
    const ASN1_BIT_STRING* psig = nullptr;
    const X509_ALGOR* palg = nullptr;
    X509_get0_signature(&psig, &palg, certificate);
    
    if (psig) {
      std::string signature_hex_string = x509_utils::hex_string_from_ASN1STRING(psig);
      _signatureHexEncoded = brave::string_to_ns(signature_hex_string);
      _bytesSize = ASN1_STRING_length(psig);
    }
    
    if (palg) {
      _algorithm = brave::string_to_ns(
                                    x509_utils::string_from_x509_algorithm(palg));
      
      int param_type = 0;
      const void* param = nullptr;
      const ASN1_OBJECT* object = nullptr;
      X509_ALGOR_get0(&object, &param_type, &param, palg);
      
      if (object) {
        _objectIdentifier = brave::string_to_ns(
                                             x509_utils::string_from_ASN1_OBJECT(object, true));
      }
      
      //Alternative to just hex-encoding the sequence,
      //is to unpack it and return all of the PSS parameters
      //Not needed for now
      //const RSA_PSS_PARAMS *RSA_get0_pss_params(const RSA *r)
      //ASN1_TYPE_unpack_sequence(param, param_type);
      
      if (param) {
        if (param_type == V_ASN1_SEQUENCE) {
          const ASN1_STRING* seq_string = static_cast<const ASN1_STRING*>(param);
          _parameters = brave::string_to_ns(
                                         x509_utils::hex_string_from_ASN1STRING(seq_string));
        }
      }
    }
  }
  return self;
}
@end
