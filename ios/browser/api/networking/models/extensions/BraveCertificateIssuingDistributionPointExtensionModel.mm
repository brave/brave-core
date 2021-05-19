/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateIssuingDistributionPointExtensionModel.h"
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


@implementation BraveCertificateIssuingDistributionPointExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _genDistPointName = @[];
  _relativeDistPointNames = @{};
  _onlyUserCertificates = false;
  _onlyCACertificates = false;
  _onlySomeReasons = BraveCRLReasonFlags_INVALID;
  _indirectCRL = false;
  _onlyAttr = false;
  _onlyAttrValidated = false;
  
  ISSUING_DIST_POINT* dist_point = static_cast<ISSUING_DIST_POINT*>(X509V3_EXT_d2i(extension));
  if (dist_point) {
    if (dist_point->distpoint) {
      DIST_POINT_NAME* dist_point_name = dist_point->distpoint;
      if (dist_point_name->type == 0) { //GENERAL_NAMES
        NSMutableArray* gen_dist_names = [[NSMutableArray alloc] init];
        for (std::size_t i = 0; i < sk_GENERAL_NAME_num(dist_point_name->name.fullname); ++i) {
          GENERAL_NAME* name = sk_GENERAL_NAME_value(dist_point_name->name.fullname, static_cast<int>(i));
          if (name) {
            [gen_dist_names addObject:brave::convert_general_name(name)];
          }
        }
        _genDistPointName = gen_dist_names;
      } else { //X509_NAME
        // dist_point_name->dpname contains full distribution point name
        _relativeDistPointNames = brave::convert_X509_NAME_ENTRIES(dist_point_name->name.relativename);
      }
    }
    
    _onlyUserCertificates = dist_point->onlyuser > 0;
    _onlyCACertificates = dist_point->onlyCA > 0;
    
    if (dist_point->onlysomereasons) {
      _onlySomeReasons = brave::convert_crl_dist_point_reason_flags(dist_point->onlysomereasons);
    }
    
    _indirectCRL = dist_point->indirectCRL > 0;
    _onlyAttr = dist_point->onlyattr > 0;
    
    if ((dist_point->onlyuser <= 0) && (dist_point->onlyCA <= 0)
        && (dist_point->indirectCRL <= 0) && !dist_point->onlysomereasons
        && (dist_point->onlyattr <= 0)) {
      _onlyAttrValidated = false;
    } else {
      _onlyAttrValidated = true;
    }
    
    ISSUING_DIST_POINT_free(dist_point);
  }
}
@end
