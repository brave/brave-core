/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateSXNetExtensionModel.h"
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

#ifndef OPENSSL_IS_BORINGSSL
@interface BraveCertificateSXNetIDExtensionModel: NSObject
@property(nonatomic, strong, readwrite) NSString* zone;
@property(nonatomic, strong, readwrite) NSString* user;
@end

@implementation BraveCertificateSXNetIDExtensionModel
@synthesize zone = _zone;
@synthesize user = _user;

- (instancetype)init {
  if ((self = [super init])) {
    _zone = [[NSString alloc] init];
    _user = [[NSString alloc] init];
  }
  return self;
}
@end

@implementation BraveCertificateSXNetExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _version = -1;
  _ids = @[];
  
  SXNET* sx_net = static_cast<SXNET*>(X509V3_EXT_d2i(extension));
  if (sx_net) {
    if (sx_net->version) {
      _version = ASN1_INTEGER_get(sx_net->version); //version + 1
    }
    
    if (sx_net->ids) {
      NSMutableArray* net_ids = [[NSMutableArray alloc] init];
      for (std::size_t i = 0; i < sk_SXNETID_num(sx_net->ids); ++i) {
        SXNETID* net_id = sk_SXNETID_value(sx_net->ids, static_cast<int>(i));
        if (net_id) {
          auto* zone_user_info = [[BraveCertificateSXNetIDExtensionModel alloc] init];
          
          if (net_id->zone) {
            zone_user_info.zone = brave::string_to_ns(
                                    x509_utils::string_from_ASN1INTEGER(net_id->zone));
          }
          
          if (net_id->user) {
            zone_user_info.user = brave::string_to_ns(
                                    x509_utils::string_from_ASN1STRING(net_id->user));
          }
          [net_ids addObject:zone_user_info];
        }
      }
      _ids = net_ids;
    }
    
    SXNET_free(sx_net);
  }
}
@end
#endif