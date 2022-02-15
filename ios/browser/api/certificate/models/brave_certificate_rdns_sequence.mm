/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/certificate/models/brave_certificate_rdns_sequence.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/base/mac/conversions.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_utils.h"
#include "brave/ios/browser/api/certificate/utils/brave_certificate_x509_utils.h"
#include "net/cert/x509_cert_types.h"
#include "net/der/input.h"

@implementation BraveCertificateRDNSequence
- (instancetype)initWithBERName:(const net::der::Input&)berName
                       uniqueId:(const net::der::BitString&)uniqueId {
  if ((self = [super init])) {
    net::CertPrincipal rdns;  // relative_distinquished_name_sequence;
    auto string_handling =
        net::CertPrincipal::PrintableStringHandling::kDefault;
    if (!rdns.ParseDistinguishedName(berName.UnsafeData(), berName.Length(),
                                     string_handling)) {
      _stateOrProvince = [[NSString alloc] init];
      _locality = [[NSString alloc] init];
      _organization = [[NSArray alloc] init];
      _organizationalUnit = [[NSArray alloc] init];
      _commonName = [[NSString alloc] init];
      _streetAddress = [[NSArray alloc] init];
      _domainComponent = [[NSArray alloc] init];
      _userId = [[NSString alloc] init];
      _countryOrRegion = [[NSString alloc] init];
      return self;
    }

    _countryOrRegion = base::SysUTF8ToNSString(rdns.country_name);
    _stateOrProvince = base::SysUTF8ToNSString(rdns.state_or_province_name);
    _locality = base::SysUTF8ToNSString(rdns.locality_name);
    _organization = brave::vector_to_ns(rdns.organization_names);
    _organizationalUnit = brave::vector_to_ns(rdns.organization_unit_names);
    _commonName = base::SysUTF8ToNSString(rdns.common_name);
    _streetAddress = brave::vector_to_ns(rdns.street_addresses);
    _domainComponent = brave::vector_to_ns(rdns.domain_components);
    _userId = base::SysUTF8ToNSString(uniqueId.bytes().AsString());
  }
  return self;
}
@end
