/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateSubjectModel.h"
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#else
  #import "brave_certificate_enums.h"
  #include "brave_certificate_ios_utils.h"
  #include "brave_certificate_utils.h"
#endif

@implementation BraveCertificateSubjectModel
- (instancetype)initWithCertificate:(X509*)certificate {
  if ((self = [super init])) {
    X509_NAME* subject_name = X509_get_subject_name(certificate);
    _countryOrRegion = brave::string_to_ns(
                           x509_utils::name_entry_from_nid(subject_name,
                                                           NID_countryName));
    _stateOrProvince = brave::string_to_ns(
                           x509_utils::name_entry_from_nid(subject_name,
                                                           NID_stateOrProvinceName));
    _locality = brave::string_to_ns(
                    x509_utils::name_entry_from_nid(subject_name,
                                                    NID_localityName));
    _organization = brave::string_to_ns(
                        x509_utils::name_entry_from_nid(subject_name,
                                                        NID_organizationName));
    _organizationalUnit = brave::string_to_ns(
                              x509_utils::name_entry_from_nid(subject_name,
                                                              NID_organizationalUnitName));
    _commonName =  brave::string_to_ns(
                       x509_utils::name_entry_from_nid(subject_name,
                                                       NID_commonName));
    _streetAddress =  brave::string_to_ns(
                          x509_utils::name_entry_from_nid(subject_name,
                                                          NID_streetAddress));
    _domainComponent =  brave::string_to_ns(
                            x509_utils::name_entry_from_nid(subject_name,
                                                            NID_domainComponent));
    _userId =  brave::string_to_ns(
                   x509_utils::name_entry_from_nid(subject_name,
                                                   NID_userId));
  }
  return self;
}
@end
