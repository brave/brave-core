/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(BRAVE_CORE)
  #import "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #import "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
  #import "brave/ios/browser/api/networking/models/extensions/BraveCertificateExtensionGeneralNameModel.h"
#else
  #import "brave_certificate_ios_utils.h"
  #import "brave_certificate_utils.h"
  #import "BraveCertificateExtensionGeneralNameModel.h"
#endif


#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/opensslconf.h"
  #if TARGET_CPU_ARM
    #include "third_party/boringssl/src/include/openssl/arm_arch.h"
  #endif
  #include "third_party/boringssl/src/include/openssl/nid.h"
#else
  #include <openssl/opensslconf.h>
  #if TARGET_CPU_ARM
    #include <openssl/arm_arch.h>
  #endif
  #include <openssl/nid.h>
#endif

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Forward declared class

@interface BraveCertificateExtensionGeneralNameModel()
@property(nonatomic, assign, readwrite) BraveGeneralNameType type;
@property(nonatomic, strong, readwrite) NSString* other;
@property(nonatomic, strong, readwrite) NSString* nameAssigner;
@property(nonatomic, strong, readwrite) NSString* partyName;
@property(nonatomic, strong, readwrite) NSDictionary<NSString*, NSString*>* dirName;
@end

namespace brave {
NSString* string_to_ns(const std::string& str) {
  if (str.empty()) {
    return [[NSString alloc] init];
  }
  return [NSString stringWithUTF8String:str.c_str()];
}

NSDate* date_to_ns(double time_interval) {
  return [NSDate dateWithTimeIntervalSince1970:time_interval];
}

BraveExtensionType extension_nid_to_extension_type(int type) {
  std::unordered_map<int, BraveExtensionType> mapping = {
    //PKIX Certificate Extensions
    {NID_basic_constraints, BraveExtensionType_BASIC_CONSTRAINTS},
    {NID_key_usage, BraveExtensionType_KEY_USAGE},
    {NID_ext_key_usage, BraveExtensionType_EXT_KEY_USAGE},
    {NID_subject_key_identifier, BraveExtensionType_SUBJECT_KEY_IDENTIFIER},
    {NID_authority_key_identifier, BraveExtensionType_AUTHORITY_KEY_IDENTIFIER},
    #ifndef OPENSSL_IS_BORINGSSL    // Chromium's BoringSSL is missing so much stuff!
    {NID_private_key_usage_period, BraveExtensionType_PRIVATE_KEY_USAGE_PERIOD},
    #endif  //   OPENSSL_IS_BORINGSSL
    {NID_subject_alt_name, BraveExtensionType_SUBJECT_ALT_NAME},
    {NID_issuer_alt_name, BraveExtensionType_ISSUER_ALT_NAME},
    {NID_info_access, BraveExtensionType_INFO_ACCESS},
    {NID_sinfo_access, BraveExtensionType_SINFO_ACCESS},
    {NID_name_constraints, BraveExtensionType_NAME_CONSTRAINTS},
    {NID_certificate_policies, BraveExtensionType_CERTIFICATE_POLICIES},
    {NID_policy_mappings, BraveExtensionType_POLICY_MAPPINGS},
    {NID_policy_constraints, BraveExtensionType_POLICY_CONSTRAINTS},
    {NID_inhibit_any_policy, BraveExtensionType_INHIBIT_ANY_POLICY},
    #ifndef OPENSSL_IS_BORINGSSL    // Chromium's BoringSSL is missing so much stuff!
    {NID_tlsfeature, BraveExtensionType_TLSFEATURE},
    #endif  //   OPENSSL_IS_BORINGSSL

    //Netscape Certificate Extensions - Largely Obsolete
    {NID_netscape_cert_type, BraveExtensionType_NETSCAPE_CERT_TYPE},
    {NID_netscape_base_url, BraveExtensionType_NETSCAPE_BASE_URL},
    {NID_netscape_revocation_url, BraveExtensionType_NETSCAPE_REVOCATION_URL},
    {NID_netscape_ca_revocation_url, BraveExtensionType_NETSCAPE_CA_REVOCATION_URL},
    {NID_netscape_renewal_url, BraveExtensionType_NETSCAPE_RENEWAL_URL},
    {NID_netscape_ca_policy_url, BraveExtensionType_NETSCAPE_CA_POLICY_URL},
    {NID_netscape_ssl_server_name, BraveExtensionType_NETSCAPE_SSL_SERVER_NAME},
    {NID_netscape_comment, BraveExtensionType_NETSCAPE_COMMENT},

    //Miscellaneous Certificate Extensions
    #ifndef OPENSSL_IS_BORINGSSL    // Chromium's BoringSSL is missing so much stuff!
    {NID_sxnet, BraveExtensionType_SXNET},
    #endif  //   OPENSSL_IS_BORINGSSL
    {NID_proxyCertInfo, BraveExtensionType_PROXYCERTINFO},

    //PKIX CRL Extensions
    {NID_crl_number, BraveExtensionType_CRL_NUMBER},
    {NID_crl_distribution_points, BraveExtensionType_CRL_DISTRIBUTION_POINTS},
    {NID_delta_crl, BraveExtensionType_DELTA_CRL},
    {NID_freshest_crl, BraveExtensionType_FRESHEST_CRL},
    {NID_invalidity_date, BraveExtensionType_INVALIDITY_DATE},
    {NID_issuing_distribution_point, BraveExtensionType_ISSUING_DISTRIBUTION_POINT},

    //CRL entry extensions from PKIX standards such as RFC5280
    {NID_crl_reason, BraveExtensionType_CRL_REASON},
    {NID_certificate_issuer, BraveExtensionType_CERTIFICATE_ISSUER},

    //OCSP Extensions
    {NID_id_pkix_OCSP_Nonce, BraveExtensionType_ID_PKIX_OCSP_NONCE},
    {NID_id_pkix_OCSP_CrlID, BraveExtensionType_ID_PKIX_OCSP_CRLID},
    {NID_id_pkix_OCSP_acceptableResponses, BraveExtensionType_ID_PKIX_OCSP_ACCEPTABLERESPONSES},
    {NID_id_pkix_OCSP_noCheck, BraveExtensionType_ID_PKIX_OCSP_NOCHECK},
    {NID_id_pkix_OCSP_archiveCutoff, BraveExtensionType_ID_PKIX_OCSP_ARCHIVECUTOFF},
    {NID_id_pkix_OCSP_serviceLocator, BraveExtensionType_ID_PKIX_OCSP_SERVICELOCATOR},
    {NID_hold_instruction_code, BraveExtensionType_HOLD_INSTRUCTION_CODE},

    //Certificate Transparency Extensions
    #ifndef OPENSSL_IS_BORINGSSL    // Chromium's BoringSSL is missing so much stuff!
    {NID_ct_precert_scts, BraveExtensionType_CT_PRECERT_SCTS},
    {NID_ct_cert_scts, BraveExtensionType_CT_CERT_SCTS}
    #endif  //   OPENSSL_IS_BORINGSSL
  };
  
  auto it = mapping.find(type);
  if (it != mapping.end()) {
    return it->second;
  }
  return BraveExtensionType_UNKNOWN;
}

BraveKeyUsage convert_key_usage(ASN1_BIT_STRING* key_usage) {
  BraveKeyUsage usage = 0;
  const unsigned char* data = key_usage ? ASN1_STRING_get0_data(key_usage) : nullptr;
  if (data && ASN1_STRING_length(key_usage) > 0) {
    if (data[0] & KU_DIGITAL_SIGNATURE) {
      //verify/sign
      usage |= BraveKeyUsage_DIGITAL_SIGNATURE;
    }
    
    if (data[0] & KU_NON_REPUDIATION) {
      usage |= BraveKeyUsage_NON_REPUDIATION;
    }
    
    if (data[0] & KU_KEY_ENCIPHERMENT) {
      //wrap
      usage |= BraveKeyUsage_KEY_ENCIPHERMENT;
    }
    
    if (data[0] & KU_DATA_ENCIPHERMENT) {
      //encrypt
      usage |= BraveKeyUsage_DATA_ENCIPHERMENT;
    }
    
    if (data[0] & KU_KEY_AGREEMENT) {
      //derive
      usage |= BraveKeyUsage_KEY_AGREEMENT;
    }
    
    if (data[0] & KU_KEY_CERT_SIGN) {
      usage |= BraveKeyUsage_KEY_CERT_SIGN;
    }
    
    if (data[0] & KU_CRL_SIGN) {
      usage |= BraveKeyUsage_CRL_SIGN;
    }
    
    if (data[0] & KU_ENCIPHER_ONLY) {
      usage |= BraveKeyUsage_ENCIPHER_ONLY;
    }
    
    if (data[0] & KU_DECIPHER_ONLY) {
      usage |= BraveKeyUsage_DECIPHER_ONLY;
    }
    
    if (ASN1_STRING_length(key_usage) > 1) {
      usage |= data[1] << 8;
    }
    
    /*if (data[0] & KU_DATA_ENCIPHERMENT ||
        (data[0] & KU_KEY_AGREEMENT &&
         data[0] & KU_KEY_ENCIPHERMENT)) {
      //encrypt
      _keyUsage |= BravePublicKeyUsage_ENCRYPT;
    }*/
  }
  return usage != 0 ? usage : BraveKeyUsage_INVALID;
}

BraveExtendedKeyUsage convert_extended_key_usage(int usage_nid) {
  BraveExtendedKeyUsage usage = 0;
  switch (usage_nid) {
    case NID_server_auth: {
      // XKU_SSL_SERVER
      usage |= BraveExtendedKeyUsage_SSL_SERVER;
    }
      break;

    case NID_client_auth: {
      // XKU_SSL_CLIENT
      usage |= BraveExtendedKeyUsage_SSL_CLIENT;
    }
      break;

    case NID_email_protect: {
      // XKU_SMIME
      usage |= BraveExtendedKeyUsage_SMIME;
    }
      break;

    case NID_code_sign: {
      // XKU_CODE_SIGN
      usage |= BraveExtendedKeyUsage_CODE_SIGN;
    }
      break;

    case NID_ms_sgc:
    case NID_ns_sgc: {
      // XKU_SGC
      usage |= BraveExtendedKeyUsage_SGC;
    }
      break;

    case NID_OCSP_sign: {
      // XKU_OCPS_SIGN
      usage |= BraveExtendedKeyUsage_OCSP_SIGN;
    }
      break;

    case NID_time_stamp: {
      // XKU_TIMESTAMP
      usage |= BraveExtendedKeyUsage_TIMESTAMP;
    }
      break;

    case NID_dvcs: {
      // XKU_DVCS
      usage |= BraveExtendedKeyUsage_DVCS;
    }
      break;

    case NID_anyExtendedKeyUsage: {
      // XKU_ANYEKU
      usage |= BraveExtendedKeyUsage_ANYEKU;
    }
      break;
  }
  return usage != 0 ? usage : BraveExtendedKeyUsage_INVALID;
}

BraveNetscapeCertificateType convert_netscape_certificate_type(ASN1_BIT_STRING* cert_type) {
  BraveNetscapeCertificateType type = 0;
  const unsigned char* data = cert_type ? ASN1_STRING_get0_data(cert_type) : nullptr;
  if (data && ASN1_STRING_length(cert_type) > 0) {
    if (data[0] & NS_SSL_CLIENT) {
      type |= BraveNetscapeCertificateType_SSL_CLIENT;
    }
    
    if (data[0] & NS_SSL_SERVER) {
      type |= BraveNetscapeCertificateType_SSL_SERVER;
    }
    
    if (data[0] & NS_SMIME) {
      type |= BraveNetscapeCertificateType_SMIME;
    }
    
    if (data[0] & NS_OBJSIGN) {
      type |= BraveNetscapeCertificateType_OBJSIGN;
    }
    
    if (data[0] & NS_SSL_CA) {
      type |= BraveNetscapeCertificateType_SSL_CA;
    }
    
    if (data[0] & NS_SMIME_CA) {
      type |= BraveNetscapeCertificateType_SMIME_CA;
    }
    
    if (data[0] & NS_OBJSIGN_CA) {
      type |= BraveNetscapeCertificateType_OBJSIGN_CA;
    }
    
    if (data[0] & NS_ANY_CA) {
      type |= BraveNetscapeCertificateType_ANY_CA;
    }
  }
  return type != 0 ? type : BraveNetscapeCertificateType_INVALID;
}

BraveCRLReasonFlags convert_crl_dist_point_reason_flags(ASN1_BIT_STRING* reason_flags) {
  BraveCRLReasonFlags reasons = 0;

  //RFC-5280
  //4.2.1.13.  CRL Distribution Points
  //ReasonFlags ::= BIT STRING {
  //    unused                  (0),
  //    keyCompromise           (1),
  //    cACompromise            (2),
  //    affiliationChanged      (3),
  //    superseded              (4),
  //    cessationOfOperation    (5),
  //    certificateHold         (6),
  //    privilegeWithdrawn      (7),
  //    aACompromise            (8) }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 0)) {
    reasons |= BraveCRLReasonFlags_UNUSED;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 1)) {
    reasons |= BraveCRLReasonFlags_KEY_COMPROMISED;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 2)) {
    reasons |= BraveCRLReasonFlags_CA_COMPROMISED;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 3)) {
    reasons |= BraveCRLReasonFlags_AFFILIATION_CHANGED;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 4)) {
    reasons |= BraveCRLReasonFlags_SUPERSEDED;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 5)) {
    reasons |= BraveCRLReasonFlags_CESSATION_OF_OPERATION;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 6)) {
    reasons |= BraveCRLReasonFlags_CERTIFICATE_HOLD;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 7)) {
    reasons |= BraveCRLReasonFlags_PRIVILEGE_WITHDRAWN;
  }
  
  if (ASN1_BIT_STRING_get_bit(reason_flags, 8)) {
    reasons |= BraveCRLReasonFlags_AA_COMPROMISED;
  }
  return reasons != 0 ? reasons : BraveCRLReasonFlags_INVALID;
}

BraveCRLReasonCode convert_crl_reason(ASN1_ENUMERATED* reason_code) {
  BraveCRLReasonCode reason = BraveCRLReasonCode_NONE;

  //RFC-5280
  //5.3.1.  Reason Code
  //CRLReason ::= ENUMERATED {
  //    unspecified             (0),
  //    keyCompromise           (1),
  //    cACompromise            (2),
  //    affiliationChanged      (3),
  //    superseded              (4),
  //    cessationOfOperation    (5),
  //    certificateHold         (6),
  //         -- value 7 is not used
  //    removeFromCRL           (8),
  //    privilegeWithdrawn      (9),
  //    aACompromise           (10) }

  switch (ASN1_ENUMERATED_get(reason_code)) {
    case CRL_REASON_UNSPECIFIED: {
      reason = BraveCRLReasonCode_UNSPECIFIED;
    }
      break;
      
    case CRL_REASON_KEY_COMPROMISE: {
      reason = BraveCRLReasonCode_KEY_COMPROMISED;
    }
      break;
      
    case CRL_REASON_CA_COMPROMISE: {
      reason = BraveCRLReasonCode_CA_COMPROMISED;
    }
      break;
      
    case CRL_REASON_AFFILIATION_CHANGED: {
      reason = BraveCRLReasonCode_AFFILIATION_CHANGED;
    }
      break;
      
    case CRL_REASON_SUPERSEDED: {
      reason = BraveCRLReasonCode_SUPERSEDED;
    }
      break;
      
    case CRL_REASON_CESSATION_OF_OPERATION: {
      reason = BraveCRLReasonCode_CESSATION_OF_OPERATION;
    }
      break;
      
    case CRL_REASON_CERTIFICATE_HOLD: {
      reason = BraveCRLReasonCode_CERTIFICATE_HOLD;
    }
      break;
      
    case CRL_REASON_REMOVE_FROM_CRL: {
      reason = BraveCRLReasonCode_REMOVE_FROM_CRL;
    }
      break;
      
    case CRL_REASON_PRIVILEGE_WITHDRAWN: {
      reason = BraveCRLReasonCode_PRIVILEGE_WITHDRAWN;
    }
      break;
      
    case CRL_REASON_AA_COMPROMISE: {
      reason = BraveCRLReasonCode_AA_COMPROMISED;
    }
      break;
  }

  return reason;
}

BraveCertificateExtensionGeneralNameModel* convert_general_name(GENERAL_NAME* name) {
  if (name) {
    auto* result = [[BraveCertificateExtensionGeneralNameModel alloc] init];
    
    int type = 0;
    std::string other;
    std::string name_assigner;
    std::string party_name;
    std::unordered_map<std::string, std::string> dir_name;
    
    x509_utils::string_from_GENERAL_NAME(name,
                                         type,
                                         other,
                                         name_assigner,
                                         party_name,
                                         dir_name);
      
    switch(type) {
      case GEN_OTHERNAME: {
        result.type = BraveGeneralNameType_OTHER_NAME;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      case GEN_X400: {
        result.type = BraveGeneralNameType_X400;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      case GEN_EDIPARTY: {
        result.type = BraveGeneralNameType_EDIPARTY;
        result.nameAssigner = brave::string_to_ns(name_assigner);
        result.partyName = brave::string_to_ns(party_name);
      }
        break;
        
      case GEN_EMAIL: {
        result.type = BraveGeneralNameType_EMAIL;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      case GEN_DNS: {
        result.type = BraveGeneralNameType_DNS;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      case GEN_URI: {
        result.type = BraveGeneralNameType_URI;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      case GEN_DIRNAME: {
        result.type = BraveGeneralNameType_DIRNAME;
        if (!dir_name.empty()) {
          NSMutableDictionary* dirName = [[NSMutableDictionary alloc] init];
          for (auto it = dir_name.begin(); it != dir_name.end(); ++it) {
            [dirName setObject:brave::string_to_ns(it->second) forKey:brave::string_to_ns(it->first)];
          }
          result.dirName = dirName;
        }
      }
        break;
        
      case GEN_IPADD: {
        result.type = BraveGeneralNameType_IPADD;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      case GEN_RID: {
        result.type = BraveGeneralNameType_RID;
        result.other = brave::string_to_ns(other);
      }
        break;
        
      default: {
        result.type = BraveGeneralNameType_INVALID;
        result.other = brave::string_to_ns(other);
      }
        break;
    }
    return result;
  }
  return nullptr;
}

NSDictionary<NSString*, NSString*>* convert_X509_NAME_ENTRIES(STACK_OF(X509_NAME_ENTRY)* entries) {
  NSMutableDictionary* result = [[NSMutableDictionary alloc] init];
  
  if (entries) {
    auto mapping = x509_utils::map_from_X509_NAME_ENTRIES(entries);
    for (auto it = mapping.begin(); it != mapping.end(); ++it) {
      [result setObject:brave::string_to_ns(it->second) forKey:brave::string_to_ns(it->first)];
    }
  }
  return result;
}
} // namespace brave
