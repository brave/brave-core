/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificatePoliciesExtensionModel.h"
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


@implementation BraveCertificatePolicyInfoQualifierNoticeExtensionModel
- (instancetype)init {
  if ((self = [super init])) {
    _organization = [[NSString alloc] init];
    _noticeNumbers = @[];
    _explicitText = [[NSString alloc] init];
  }
  return self;
}

- (void)setOrganization:(NSString*)organization {
  _organization = organization;
}

- (void)setNoticeNumbers:(NSArray<NSString*>*)noticeNumbers {
  _noticeNumbers = noticeNumbers;
}

- (void)setExplicitText:(NSString*)explicitText {
  _explicitText = explicitText;
}
@end

@implementation BraveCertificatePolicyInfoQualifierExtensionModel
- (instancetype)init {
  if ((self = [super init])) {
    _pqualid = 0;
    _cps = [[NSString alloc] init];
    _notice = nil;
    _unknown = [[NSString alloc] init];
  }
  return self;
}

- (void)setPqualid:(NSInteger)pqualid {
  _pqualid = pqualid;
}

- (void)setCps:(NSString*)cps {
  _cps = cps;
}

- (void)setNotice:(BraveCertificatePolicyInfoQualifierNoticeExtensionModel*)notice {
  _notice = notice;
}

- (void)setUnknown:(NSString*)unknown {
  _unknown = unknown;
}
@end

@implementation BraveCertificatePolicyInfoExtensionModel
- (instancetype)init {
  if ((self = [super init])) {
    _oid = [[NSString alloc] init];
    _qualifiers = @[];
  }
  return self;
}

- (void)setOid:(NSString*)oid {
  _oid = oid;
}

- (void)setQualifiers:(NSArray<BraveCertificatePolicyInfoQualifierExtensionModel*>*)qualifiers {
  _qualifiers = qualifiers;
}
@end

@implementation BraveCertificatePoliciesExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  
  //STACK_OF(POLICYINFO)
  CERTIFICATEPOLICIES* policies = static_cast<CERTIFICATEPOLICIES*>(X509V3_EXT_d2i(extension));
  if (policies) {
    for (std::size_t i = 0; i < sk_POLICYINFO_num(policies); ++i) {
      POLICYINFO* info = sk_POLICYINFO_value(policies, static_cast<int>(i));
      if (info) {
        auto* policy = [[BraveCertificatePolicyInfoExtensionModel alloc] init];
        [policy setOid:brave::string_to_ns(x509_utils::string_from_ASN1_OBJECT(info->policyid, true))];
        
        if (info->qualifiers) {
          NSMutableArray* qualifiers = [[NSMutableArray alloc] init];
          
          for (std::size_t i = 0; i < sk_POLICYQUALINFO_num(info->qualifiers); ++i) {
            POLICYQUALINFO* qualifier_info =
                                  sk_POLICYQUALINFO_value(info->qualifiers, static_cast<int>(i));
            if (qualifier_info) {
              auto* info = [[BraveCertificatePolicyInfoQualifierExtensionModel alloc] init];
              switch (OBJ_obj2nid(qualifier_info->pqualid)) {
                case NID_id_qt_cps: {
                  //ASN1_IA5STRING
                  [info setPqualid:NID_id_qt_cps];
                  [info setCps:brave::string_to_ns(
                                        x509_utils::string_from_ASN1STRING(qualifier_info->d.cpsuri))];
                }
                  break;

                case NID_id_qt_unotice: {
                  [info setPqualid:NID_id_qt_unotice];
                  USERNOTICE* user_notice = qualifier_info->d.usernotice;
                  if (user_notice) {
                    auto* notice = [[BraveCertificatePolicyInfoQualifierNoticeExtensionModel alloc] init];
                    
                    //i2d_USERNOTICE
                    if (user_notice->noticeref) {
                      NOTICEREF* notice_ref = user_notice->noticeref;
                      [notice setOrganization:brave::string_to_ns(
                                                  x509_utils::string_from_ASN1STRING(notice_ref->organization))];
                      
                      if (notice_ref->noticenos) {
                        NSMutableArray* noticeNumbers = [[NSMutableArray alloc] init];
                        for (std::size_t i = 0; i < sk_ASN1_INTEGER_num(notice_ref->noticenos); ++i) {
                          ASN1_INTEGER* number =
                              sk_ASN1_INTEGER_value(notice_ref->noticenos, static_cast<int>(i));
                          if (number) {
                            [noticeNumbers addObject:brave::string_to_ns(
                                                              x509_utils::string_from_ASN1INTEGER(number))];
                          }
                        }
                        [notice setNoticeNumbers:noticeNumbers];
                      }
                    }
                    
                    if (user_notice->exptext) {
                      //printf("\n");
                      [notice setExplicitText:brave::string_to_ns(
                                                        x509_utils::string_from_ASN1STRING(user_notice->exptext))];
                    }
                    
                    [info setNotice:notice];
                  }
                }
                  break;

                default: {
                  [info setPqualid:OBJ_obj2nid(qualifier_info->pqualid)];
                  [info setUnknown:brave::string_to_ns(
                                      x509_utils::string_from_ASN1_OBJECT(qualifier_info->pqualid, false))];
                  //printf("    Unknown Qualifier: %s\n", qualifier_id.c_str());
                }
                  break;
              }
              
              [qualifiers addObject:info];
            }
          }
          
          [policy setQualifiers:qualifiers];
        }
        
        [result addObject:policy];
      }
    }
    CERTIFICATEPOLICIES_free(policies);
  }
  
  _policies = result;
}
@end
