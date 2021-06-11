/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_UTILS_BRAVE_CERTIFICATE_IOS_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_UTILS_BRAVE_CERTIFICATE_IOS_H_

#import <Foundation/Foundation.h>
#include <string>

#if defined(BRAVE_CORE)
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
#else
  #import "brave_certificate_enums.h"
#endif

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
  #include "third_party/boringssl/src/include/openssl/asn1.h"
#else
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
  #include <openssl/asn1.h>
#endif

@class BraveCertificateExtensionGeneralNameModel;

namespace brave {
NSString* string_to_ns(const std::string& str);
NSDate* date_to_ns(double time_interval);

BraveExtensionType extension_nid_to_extension_type(int type);
BraveKeyUsage convert_key_usage(ASN1_BIT_STRING* key_usage);
BraveExtendedKeyUsage convert_extended_key_usage(int usage_nid);
BraveNetscapeCertificateType convert_netscape_certificate_type(ASN1_BIT_STRING* cert_type);
BraveCRLReasonFlags convert_crl_dist_point_reason_flags(ASN1_BIT_STRING* reason_flags);
BraveCRLReasonCode convert_crl_reason(ASN1_ENUMERATED* reason_code);
BraveCertificateExtensionGeneralNameModel* convert_general_name(GENERAL_NAME* name);
NSDictionary<NSString*, NSString*>* convert_X509_NAME_ENTRIES(STACK_OF(X509_NAME_ENTRY)* entries);
} // namespace brave

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_UTILS_BRAVE_CERTIFICATE_IOS_H_
