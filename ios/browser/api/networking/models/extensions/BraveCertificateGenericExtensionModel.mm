/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateGenericExtensionModel.h"
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

@implementation BraveCertificateGenericExtensionPairModel
- (instancetype)initWithKey:(NSString*)key withValue:(NSString*)value {
  if ((self = [super init])) {
    _key = key;
    _value = value;
  }
  return self;
}
@end

@implementation BraveCertificateGenericExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  _extensionType = BraveGenericExtensionType_STRING;
  _stringValue = nullptr;
  _arrayValue = nullptr;
  
  std::unique_ptr<x509_utils::X509_EXTENSION_INFO> info =
                                      x509_utils::decode_extension_info(extension);
  if (info) {
    if (info->get_type() == x509_utils::X509_EXTENSION_INFO::TYPE::STRING) {
      std::string* string_ptr = info->get_extension_string();
      _extensionType = BraveGenericExtensionType_STRING;
      _stringValue = string_ptr ? brave::string_to_ns(*string_ptr) : [[NSString alloc] init];
      return;
    } else if (info->get_type() == x509_utils::X509_EXTENSION_INFO::TYPE::MULTI_VALUE) {
      _extensionType = BraveGenericExtensionType_KEY_VALUE;
      
      NSMutableArray* pairs = [[NSMutableArray alloc] init];
      std::vector<std::pair<std::string, std::string>>* vec = info->get_extension_multi_value();
      for (auto it = vec->begin(); it != vec->end(); ++it) {
        [pairs addObject:[[BraveCertificateGenericExtensionPairModel alloc] initWithKey:
                                                                                  brave::string_to_ns(it->first)
                                                                              withValue:brave::string_to_ns(it->second)]];
      }
      _arrayValue = pairs;
      return;
    }
  }
    
  _extensionType = BraveGenericExtensionType_HEX_STRING;
  ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(extension);
  _stringValue = data ? brave::string_to_ns(x509_utils::hex_string_from_ASN1STRING(data)) : [[NSString alloc] init];
}

- (void)debugPrint:(X509_EXTENSION*)extension {
  std::string result;
  
  std::unique_ptr<x509_utils::X509_EXTENSION_INFO> info =
                                      x509_utils::decode_extension_info(extension);
  if (info) {
    if (info->get_type() == x509_utils::X509_EXTENSION_INFO::TYPE::STRING) {
      result += *info->get_extension_string();
      result += "\n";
    } else if (info->get_type() == x509_utils::X509_EXTENSION_INFO::TYPE::MULTI_VALUE) {
      std::vector<std::pair<std::string, std::string>>* vec = info->get_extension_multi_value();
      for (auto it = vec->begin(); it != vec->end(); ++it) {
        if (it->first.empty()) {
          result += it->second;
        } else if (it->second.empty()) {
          result += it->first;
        } else {
          result += it->first + ": ";
          result += it->second;
        }

        result += "\n";
      }
    } else {
      ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(extension);
      if (data) {
        result += x509_utils::hex_string_from_ASN1STRING(data);
        result += "\n";
      }
    }
  } else {
    ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(extension);
    if (data) {
      result += x509_utils::hex_string_from_ASN1STRING(data);
      result += "\n";
    }
  }

  NSLog(@"%s\n", result.c_str());
}
@end
