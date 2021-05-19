/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateCRLDistributionPointsExtensionModel.h"
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


@implementation BraveCertificateCRLDistPointExtensionModel
- (void)setGenDistPointName:(NSArray<BraveCertificateExtensionGeneralNameModel*>*)genDistPointName {
  _genDistPointName = genDistPointName;
}

- (void)setRelativeDistPointNames:(NSDictionary<NSString*,NSString*>*)relativeDistPointNames {
  _relativeDistPointNames = relativeDistPointNames;
}

- (void)setReasonFlags:(BraveCRLReasonFlags)reasonFlags {
  _reasonFlags = reasonFlags;
}

- (void)setCrlIssuer:(NSArray<BraveCertificateExtensionGeneralNameModel*>*)crlIssuer {
  _crlIssuer = crlIssuer;
}

- (void)setDpReason:(NSInteger)dpReason {
  _dpReason = dpReason;
}
@end

@implementation BraveCertificateCRLDistributionPointsExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  
  CRL_DIST_POINTS* crl_dist_points = static_cast<CRL_DIST_POINTS*>(X509V3_EXT_d2i(extension));
  if (crl_dist_points) {
    for (std::size_t i = 0; i < sk_DIST_POINT_num(crl_dist_points); ++i) {
      DIST_POINT* value = sk_DIST_POINT_value(crl_dist_points, static_cast<int>(i));
      if (value) {
        auto* dist_point = [[BraveCertificateCRLDistPointExtensionModel alloc] init];
        
        if (value->distpoint) {
          DIST_POINT_NAME* dist_point_name = value->distpoint;
          if (dist_point_name->type == 0) { //GENERAL_NAMES
            NSMutableArray* gen_dist_names = [[NSMutableArray alloc] init];
            for (std::size_t i = 0; i < sk_GENERAL_NAME_num(dist_point_name->name.fullname); ++i) {
              GENERAL_NAME* name = sk_GENERAL_NAME_value(dist_point_name->name.fullname, static_cast<int>(i));
              if (name) {
                [gen_dist_names addObject:brave::convert_general_name(name)];
              }
            }
            [dist_point setGenDistPointName:gen_dist_names];
          } else { //X509_NAME
            // dist_point_name->dpname contains full distribution point name
            [dist_point setRelativeDistPointNames:brave::convert_X509_NAME_ENTRIES(dist_point_name->name.relativename)];
          }
        }
        
        if (value->reasons) {
          [dist_point setReasonFlags:brave::convert_crl_dist_point_reason_flags(value->reasons)];
        }
        
        if (value->CRLissuer) {
          NSMutableArray* crl_issuer_names = [[NSMutableArray alloc] init];
          for (std::size_t i = 0; i < sk_GENERAL_NAME_num(value->CRLissuer); ++i) {
            GENERAL_NAME* name = sk_GENERAL_NAME_value(value->CRLissuer, static_cast<int>(i));
            if (name) {
              [crl_issuer_names addObject:brave::convert_general_name(name)];
            }
          }
          [dist_point setCrlIssuer:crl_issuer_names];
        }
        
        // Same as value->reasons but as an integer instead of bit string
        [dist_point setDpReason:value->dp_reasons];
        /*if (!value->reasons && value->dp_reasons == CRLDP_ALL_REASONS) {
          
        }*/
        
        [result addObject:dist_point];
      }
    }
    CRL_DIST_POINTS_free(crl_dist_points);
  }
  _distPoints = result;
}
@end
