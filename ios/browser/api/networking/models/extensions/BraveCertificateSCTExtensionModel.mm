/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BraveCertificateSCTExtensionModel.h"
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
  #ifndef OPENSSL_NO_CT
    #include "third_party/boringssl/src/include/openssl/ct.h"
  #endif
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
#else
  #ifndef OPENSSL_NO_CT
    #include <openssl/ct.h>
  #endif
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
#endif

#ifndef OPENSSL_NO_CT
@interface BraveCertificateSCTModel()
@property(nonatomic, assign, readwrite) NSInteger version;
@property(nonatomic, assign, readwrite) NSInteger logEntryType;
@property(nonatomic, strong, readwrite) NSString* logId;
@property(nonatomic, strong, readwrite) NSDate* timestamp;
@property(nonatomic, strong, readwrite) NSString* extensions;
@property(nonatomic, assign, readwrite) NSInteger signatureNid;
@property(nonatomic, strong, readwrite) NSString* signatureName;
@property(nonatomic, strong, readwrite) NSString* signature;
@property(nullable, nonatomic, strong, readwrite) NSString* hexRepresentation;
@end

@implementation BraveCertificateSCTModel
@synthesize version = _version;
@synthesize logEntryType = _logEntryType;
@synthesize logId = _logId;
@synthesize timestamp = _timestamp;
@synthesize extensions = _extensions;
@synthesize signatureNid = _signatureNid;
@synthesize signatureName = _signatureName;
@synthesize signature = _signature;
@synthesize hexRepresentation = _hexRepresentation;

- (instancetype)init {
  if ((self = [super init])) {
    _version = -1;
    _logEntryType = -1;
    _logId = [[NSString alloc] init];
    _timestamp = [NSDate dateWithTimeIntervalSince1970:0];
  }
  return self;
}
@end

@implementation BraveCertificateSCTExtensionModel
- (void)parseExtension:(X509_EXTENSION*)extension {
  NSMutableArray* result = [[NSMutableArray alloc] init];
  
  #ifndef OPENSSL_NO_CT
  STACK_OF(SCT)* sct_list = static_cast<STACK_OF(SCT)*>(X509V3_EXT_d2i(extension));
  for (std::size_t i = 0; i < sk_SCT_num(sct_list); ++i) {
    SCT* sct = sk_SCT_value(sct_list, static_cast<int>(i));
    if (sct) {
      //printf("Signed Certificate Timestamp:\n");
      
      auto* ext = [[BraveCertificateSCTModel alloc] init];
      
      sct_version_t version = SCT_get_version(sct);
      if (version == SCT_VERSION_NOT_SET) {
        ext.hexRepresentation = [[NSString alloc] init];
        
        int length = i2o_SCT(sct, nullptr);
        if (length > 0) {
          std::vector<std::uint8_t> sct_data = std::vector<std::uint8_t>(length);
          unsigned char* buffer = &sct_data[0];
          length = i2o_SCT(sct, &buffer);
          sct_data.resize(length);
          
          if (sct_data.size() > 0) {
            ext.hexRepresentation = brave::string_to_ns(
                                            x509_utils::hex_string_from_bytes(sct_data));
          }
        }
        continue;
      }
      
      ext.version = version; //version + 1
      ext.hexRepresentation = nil;
      
      ct_log_entry_type_t log_entry_type = SCT_get_log_entry_type(sct);
      if (log_entry_type != CT_LOG_ENTRY_TYPE_NOT_SET) {
        ext.logEntryType = log_entry_type;
        
        unsigned char* log_id_buffer = nullptr;
        std::size_t log_id_length = SCT_get0_log_id(sct, &log_id_buffer);
        if (log_id_buffer) {
          std::vector<std::uint8_t> log_id_data = std::vector<std::uint8_t>(log_id_length);
          std::memcpy(&log_id_data[0], log_id_buffer, log_id_length);
          OPENSSL_free(log_id_buffer);
          
          ext.logId = brave::string_to_ns(x509_utils::hex_string_from_bytes(log_id_data));
        } else {
          ext.logId = [[NSString alloc] init];
        }
      }
      
      std::uint64_t timestamp = SCT_get_timestamp(sct);
      ext.timestamp = brave::date_to_ns(timestamp);
      
      unsigned char* extension_buffer = nullptr;
      std::size_t extension_length = SCT_get0_extensions(sct, &extension_buffer);
      if (extension_buffer) {
        std::vector<std::uint8_t> extension_data = std::vector<std::uint8_t>(extension_length);
        std::memcpy(&extension_data[0], extension_buffer, extension_length);
        OPENSSL_free(extension_buffer);
        
        ext.extensions = brave::string_to_ns(x509_utils::hex_string_from_bytes(extension_data));
      } else {
        ext.extensions = [[NSString alloc] init];
      }
      
      int signature_nid = SCT_get_signature_nid(sct);
      ext.signatureNid = signature_nid;
      
      if (signature_nid != NID_undef) {
        // NID_sha256
        // NID_ecdsa_with_SHA256
        ext.signatureName = brave::string_to_ns(OBJ_nid2ln(signature_nid));
      } else {
        ext.signatureName = [[NSString alloc] init];
      }
      
      unsigned char* signature_buffer = nullptr;
      std::size_t signature_length = SCT_get0_signature(sct, &signature_buffer);
      if (signature_buffer) {
        std::vector<std::uint8_t> signature_data = std::vector<std::uint8_t>(signature_length);
        std::memcpy(&signature_data[0], signature_buffer, signature_length);
        OPENSSL_free(signature_buffer);
        
        ext.signature = brave::string_to_ns(
                                    x509_utils::hex_string_from_bytes(signature_data));
      } else {
        ext.signature = [[NSString alloc] init];
      }
      
      [result addObject:ext];
    }
  }
  #endif
  
  _scts = result;
}
@end
#endif