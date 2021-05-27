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

@interface BraveCertificateCRLDistPointExtensionModel()
@property(nonatomic, strong, readwrite) NSArray<BraveCertificateExtensionGeneralNameModel*>* genDistPointName;
@property(nonatomic, strong, readwrite) NSDictionary<NSString*, NSString*>* relativeDistPointNames;
@property(nonatomic, assign, readwrite) BraveCRLReasonFlags reasonFlags;
@property(nonatomic, strong, readwrite) NSArray<BraveCertificateExtensionGeneralNameModel*>* crlIssuer;
@property(nonatomic, assign, readwrite) NSInteger dpReason;
@end

@implementation BraveCertificateCRLDistPointExtensionModel
@synthesize genDistPointName = _genDistPointName;
@synthesize relativeDistPointNames = _relativeDistPointNames;
@synthesize reasonFlags = _reasonFlags;
@synthesize crlIssuer = _crlIssuer;
@synthesize dpReason = _dpReason;

- (instancetype)init {
  if ((self = [super init])) {
    _relativeDistPointNames = @{};
    _reasonFlags = BraveCRLReasonFlags_INVALID;
    _crlIssuer = @[];
    _dpReason = -1; // infinite according to spec, instead of NSIntegerMax
  }
  return self;
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
            for (std::size_t j = 0; j < sk_GENERAL_NAME_num(dist_point_name->name.fullname); ++j) {
              GENERAL_NAME* name = sk_GENERAL_NAME_value(dist_point_name->name.fullname, static_cast<int>(j));
              if (name) {
                [gen_dist_names addObject:brave::convert_general_name(name)];
              }
            }
            dist_point.genDistPointName = gen_dist_names;
          } else { //X509_NAME
            // dist_point_name->dpname contains full distribution point name
            dist_point.relativeDistPointNames = brave::convert_X509_NAME_ENTRIES(dist_point_name->name.relativename);
          }
        }
        
        if (value->reasons) {
          dist_point.reasonFlags = brave::convert_crl_dist_point_reason_flags(value->reasons);
        }
        
        if (value->CRLissuer) {
          NSMutableArray* crl_issuer_names = [[NSMutableArray alloc] init];
          for (std::size_t j = 0; j < sk_GENERAL_NAME_num(value->CRLissuer); ++j) {
            GENERAL_NAME* name = sk_GENERAL_NAME_value(value->CRLissuer, static_cast<int>(j));
            if (name) {
              [crl_issuer_names addObject:brave::convert_general_name(name)];
            }
          }
          dist_point.crlIssuer = crl_issuer_names;
        }
        
        // Same as value->reasons but as an integer instead of bit string
        dist_point.dpReason = value->dp_reasons;
        [result addObject:dist_point];
      }
    }
    CRL_DIST_POINTS_free(crl_dist_points);
  }
  _distPoints = result;
}
@end
