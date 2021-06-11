/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateSubjectInformationAccessExtensionModel.h"
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

@interface BraveCertificateSubjectInformationAccessDescriptionExtensionModel()
@property(nonatomic, strong, readwrite) NSString* oidName;
@property(nonatomic, strong, readwrite) NSString* oid;
@property(nonatomic, strong, readwrite) NSArray<BraveCertificateExtensionGeneralNameModel*>* locations;
@end

@implementation BraveCertificateSubjectInformationAccessDescriptionExtensionModel
- (instancetype)init {
  if ((self = [super init])) {
    _oidName = [[NSString alloc] init];
    _oid = [[NSString alloc] init];
    _locations = @[];
  }
  return self;
}
@end


@implementation BraveCertificateSubjectInformationAccessExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  //NID_sinfo_access and NID_info_access share the same structures of ACCESS_DESCRIPTION stacks

  NSMutableArray* access_descriptions = [[NSMutableArray alloc] init];
  
  //STACK_OF(ACCESS_DESCRIPTION)
  AUTHORITY_INFO_ACCESS* info_access = static_cast<AUTHORITY_INFO_ACCESS*>(X509V3_EXT_d2i(extension));
  if (info_access) {
    for (std::size_t i = 0; i < sk_ACCESS_DESCRIPTION_num(info_access); ++i) {
      ACCESS_DESCRIPTION* desc = sk_ACCESS_DESCRIPTION_value(info_access, static_cast<int>(i));
      if (desc) {
        auto* access_description = [[BraveCertificateSubjectInformationAccessDescriptionExtensionModel alloc] init];

        access_description.oidName = brave::string_to_ns(x509_utils::string_from_ASN1_OBJECT(desc->method, false));
        access_description.oid = brave::string_to_ns(x509_utils::string_from_ASN1_OBJECT(desc->method, true));

        if (desc->location) {
          NSMutableArray* locations = [[NSMutableArray alloc] init];
          auto* converted_name = brave::convert_general_name(desc->location);
          if (converted_name) {
            [locations addObject:converted_name];
          }
          access_description.locations = locations;
        }

        [access_descriptions addObject:access_description];
      }
    }
    AUTHORITY_INFO_ACCESS_free(info_access);
  }
  _accessDescriptions = access_descriptions;
}
@end
