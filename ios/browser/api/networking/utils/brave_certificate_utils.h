/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_UTILS_BRAVE_CERTIFICATE_UTILS_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_UTILS_BRAVE_CERTIFICATE_UTILS_H_

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
  #include "third_party/boringssl/src/include/openssl/asn1.h"
#else
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
  #include <openssl/asn1.h>
#endif

#include <string>
#include <vector>
#include <unordered_map>

namespace x509_utils {
class X509_EXTENSION_INFO;
} // namespace x509_utils

namespace x509_utils {
const char* EC_curve_nid2nist(int nid);
int EC_curve_nid2num_bits(int nid);
std::string int_to_hex_string(std::uint64_t value);
std::string hex_string_from_bytes(const std::vector<std::uint8_t>& bytes);
std::string string_from_ASN1STRING(const ASN1_STRING* string);
std::string hex_string_from_ASN1STRING(const ASN1_STRING* string);
std::string hex_string_from_ASN1_BIT_STRING(ASN1_BIT_STRING* string);
std::string string_from_ASN1INTEGER(ASN1_INTEGER* integer);
std::string hex_string_from_ASN1INTEGER(ASN1_INTEGER* integer);
std::string string_from_x509_algorithm(const X509_ALGOR* algorithm);
std::string string_from_ASN1_OBJECT(const ASN1_OBJECT* object, bool no_name);
std::string string_from_GENERAL_NAME(const GENERAL_NAME* name);
void string_from_GENERAL_NAME(const GENERAL_NAME* name,
                              int& type,
                              std::string& other,
                              std::string& name_assigner,
                              std::string& party_name,
                              std::unordered_map<std::string, std::string>& dir_name);
double date_from_ASN1TIME(const ASN1_TIME* date);
std::string name_entry_from_nid(X509_NAME* name, std::int32_t nid);
std::unordered_map<std::string, std::string> map_from_X509_NAME(X509_NAME* name);
std::unordered_map<std::string, std::string> map_from_X509_NAME_ENTRIES(STACK_OF(X509_NAME_ENTRY)* entries);
std::string serial_number_from_certificate(X509* certificate);
std::string fingerprint_with_nid(X509* certificate, int nid);
std::unique_ptr<X509_EXTENSION_INFO> decode_extension_info(X509_EXTENSION* extension);
} // namespace x509_utils

namespace x509_utils {
class X509_EXTENSION_INFO {
public:
  enum TYPE {
    STRING,
    MULTI_VALUE
  };
  
  X509_EXTENSION_INFO(const std::string& value);
  X509_EXTENSION_INFO(const std::vector<
                                  std::pair<std::string,
                                           std::string>>& values,
                      bool is_multi_line);
  ~X509_EXTENSION_INFO();
  TYPE get_type();
  bool has_multi_line_flag();
  std::string* get_extension_string();
  std::vector<std::pair<std::string, std::string>>* get_extension_multi_value();
  
  // Delete copy constructor and copy assignment operator.
  X509_EXTENSION_INFO(const X509_EXTENSION_INFO& other) = delete;
  X509_EXTENSION_INFO& operator = (const X509_EXTENSION_INFO& other) = delete;
  
  // Allow move constructor and move assign
  X509_EXTENSION_INFO(X509_EXTENSION_INFO&& other);
  X509_EXTENSION_INFO& operator = (X509_EXTENSION_INFO&& other);
  
private:
  void* value; // could use std::any but our enum is only two types anyway
  TYPE value_type;
  bool is_multi_line;
};
} // namespace x509_utils

#endif  //  BRAVE_IOS_BROWSER_API_NETWORKING_UTILS_BRAVE_CERTIFICATE_UTILS_H_
