/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_NETWORKING_COMMON_BRAVE_CERTIFICATE_ENUMS_H_
#define BRAVE_IOS_BROWSER_API_NETWORKING_COMMON_BRAVE_CERTIFICATE_ENUMS_H_

#import <Foundation/Foundation.h>

typedef NS_OPTIONS(NSUInteger, BravePublicKeyUsage) {
  BravePublicKeyUsage_INVALID = 1 << 0,
  BravePublicKeyUsage_ENCRYPT = 1 << 1,
  BravePublicKeyUsage_VERIFY  = 1 << 2,
  BravePublicKeyUsage_WRAP    = 1 << 3,
  BravePublicKeyUsage_DERIVE  = 1 << 4,
  BravePublicKeyUsage_ANY     = 1 << 5
};

typedef NS_OPTIONS(NSUInteger, BraveKeyUsage) {
  BraveKeyUsage_INVALID           = 1 << 0,
  BraveKeyUsage_DIGITAL_SIGNATURE = 1 << 1,
  BraveKeyUsage_NON_REPUDIATION   = 1 << 2,
  BraveKeyUsage_KEY_ENCIPHERMENT  = 1 << 3,
  BraveKeyUsage_DATA_ENCIPHERMENT = 1 << 4,
  BraveKeyUsage_KEY_AGREEMENT     = 1 << 5,
  BraveKeyUsage_KEY_CERT_SIGN     = 1 << 6,
  BraveKeyUsage_CRL_SIGN          = 1 << 7,
  BraveKeyUsage_ENCIPHER_ONLY     = 1 << 8,
  BraveKeyUsage_DECIPHER_ONLY     = 1 << 9
};

typedef NS_OPTIONS(NSUInteger, BraveExtendedKeyUsage) {
  BraveExtendedKeyUsage_INVALID    = 1 << 0,
  BraveExtendedKeyUsage_SSL_SERVER = 1 << 1,
  BraveExtendedKeyUsage_SSL_CLIENT = 1 << 2,
  BraveExtendedKeyUsage_SMIME      = 1 << 3,
  BraveExtendedKeyUsage_CODE_SIGN  = 1 << 4,
  BraveExtendedKeyUsage_SGC        = 1 << 5,
  BraveExtendedKeyUsage_OCSP_SIGN  = 1 << 6,
  BraveExtendedKeyUsage_TIMESTAMP  = 1 << 7,
  BraveExtendedKeyUsage_DVCS       = 1 << 8,
  BraveExtendedKeyUsage_ANYEKU     = 1 << 9
};

typedef NS_OPTIONS(NSUInteger, BraveNetscapeCertificateType) {
  BraveNetscapeCertificateType_INVALID    = 1 << 0,
  BraveNetscapeCertificateType_SSL_CLIENT = 1 << 1,
  BraveNetscapeCertificateType_SSL_SERVER = 1 << 2,
  BraveNetscapeCertificateType_SMIME      = 1 << 3,
  BraveNetscapeCertificateType_OBJSIGN    = 1 << 4,
  BraveNetscapeCertificateType_SSL_CA     = 1 << 5,
  BraveNetscapeCertificateType_SMIME_CA   = 1 << 6,
  BraveNetscapeCertificateType_OBJSIGN_CA = 1 << 7,
  BraveNetscapeCertificateType_ANY_CA     = 1 << 8
};

typedef NS_ENUM(NSUInteger, BraveGeneralNameType) {
  BraveGeneralNameType_INVALID,
  BraveGeneralNameType_OTHER_NAME,
  BraveGeneralNameType_EMAIL,
  BraveGeneralNameType_DNS,
  BraveGeneralNameType_X400,
  BraveGeneralNameType_DIRNAME,
  BraveGeneralNameType_EDIPARTY,
  BraveGeneralNameType_URI,
  BraveGeneralNameType_IPADD,
  BraveGeneralNameType_RID
};

typedef NS_OPTIONS(NSUInteger, BraveCRLReasonFlags) {
  BraveCRLReasonFlags_INVALID                = 1 << 0,
  BraveCRLReasonFlags_UNUSED                 = 1 << 1,
  BraveCRLReasonFlags_KEY_COMPROMISED        = 1 << 2,
  BraveCRLReasonFlags_CA_COMPROMISED         = 1 << 3,
  BraveCRLReasonFlags_AFFILIATION_CHANGED    = 1 << 4,
  BraveCRLReasonFlags_SUPERSEDED             = 1 << 5,
  BraveCRLReasonFlags_CESSATION_OF_OPERATION = 1 << 6,
  BraveCRLReasonFlags_CERTIFICATE_HOLD       = 1 << 7,
  BraveCRLReasonFlags_PRIVILEGE_WITHDRAWN    = 1 << 8,
  BraveCRLReasonFlags_AA_COMPROMISED         = 1 << 9
};

typedef NS_ENUM(NSInteger, BraveCRLReasonCode) {
  BraveCRLReasonCode_NONE                = -1,
  BraveCRLReasonCode_UNSPECIFIED            = 0,
  BraveCRLReasonCode_KEY_COMPROMISED        = 1,
  BraveCRLReasonCode_CA_COMPROMISED         = 2,
  BraveCRLReasonCode_AFFILIATION_CHANGED    = 3,
  BraveCRLReasonCode_SUPERSEDED             = 4,
  BraveCRLReasonCode_CESSATION_OF_OPERATION = 5,
  BraveCRLReasonCode_CERTIFICATE_HOLD       = 6,
  BraveCRLReasonCode_REMOVE_FROM_CRL        = 7,
  BraveCRLReasonCode_PRIVILEGE_WITHDRAWN    = 8,
  BraveCRLReasonCode_AA_COMPROMISED         = 9
};

typedef NS_ENUM(NSUInteger, BravePublicKeyType) {
  BravePublicKeyType_UNKNOWN,
  BravePublicKeyType_RSA,
  BravePublicKeyType_DSA,
  BravePublicKeyType_DH,
  BravePublicKeyType_EC
};

typedef NS_ENUM(NSUInteger, BraveFingerprintType) {
  BraveFingerprintType_SHA1,
  BraveFingerprintType_SHA256
};

typedef NS_ENUM(NSUInteger, BraveGenericExtensionType) {
  BraveGenericExtensionType_STRING,
  BraveGenericExtensionType_KEY_VALUE,
  BraveGenericExtensionType_HEX_STRING
};

typedef NS_ENUM(NSUInteger, BraveExtensionType) {
  BraveExtensionType_UNKNOWN,
  
  //PKIX Certificate Extensions
  BraveExtensionType_BASIC_CONSTRAINTS,
  BraveExtensionType_KEY_USAGE,
  BraveExtensionType_EXT_KEY_USAGE,
  BraveExtensionType_SUBJECT_KEY_IDENTIFIER,
  BraveExtensionType_AUTHORITY_KEY_IDENTIFIER,
  BraveExtensionType_PRIVATE_KEY_USAGE_PERIOD,
  BraveExtensionType_SUBJECT_ALT_NAME,
  BraveExtensionType_ISSUER_ALT_NAME,
  BraveExtensionType_INFO_ACCESS,
  BraveExtensionType_SINFO_ACCESS,
  BraveExtensionType_NAME_CONSTRAINTS,
  BraveExtensionType_CERTIFICATE_POLICIES,
  BraveExtensionType_POLICY_MAPPINGS,
  BraveExtensionType_POLICY_CONSTRAINTS,
  BraveExtensionType_INHIBIT_ANY_POLICY,
  BraveExtensionType_TLSFEATURE,

  //Netscape Certificate Extensions - Largely Obsolete
  BraveExtensionType_NETSCAPE_CERT_TYPE,
  BraveExtensionType_NETSCAPE_BASE_URL,
  BraveExtensionType_NETSCAPE_REVOCATION_URL,
  BraveExtensionType_NETSCAPE_CA_REVOCATION_URL,
  BraveExtensionType_NETSCAPE_RENEWAL_URL,
  BraveExtensionType_NETSCAPE_CA_POLICY_URL,
  BraveExtensionType_NETSCAPE_SSL_SERVER_NAME,
  BraveExtensionType_NETSCAPE_COMMENT,

  //Miscellaneous Certificate Extensions
  BraveExtensionType_SXNET,
  BraveExtensionType_PROXYCERTINFO,

  //PKIX CRL Extensions
  BraveExtensionType_CRL_NUMBER,
  BraveExtensionType_CRL_DISTRIBUTION_POINTS,
  BraveExtensionType_DELTA_CRL,
  BraveExtensionType_FRESHEST_CRL,
  BraveExtensionType_INVALIDITY_DATE,
  BraveExtensionType_ISSUING_DISTRIBUTION_POINT,

  //CRL entry extensions from PKIX standards such as RFC5280
  BraveExtensionType_CRL_REASON,
  BraveExtensionType_CERTIFICATE_ISSUER,

  //OCSP Extensions
  BraveExtensionType_ID_PKIX_OCSP_NONCE,
  BraveExtensionType_ID_PKIX_OCSP_CRLID,
  BraveExtensionType_ID_PKIX_OCSP_ACCEPTABLERESPONSES,
  BraveExtensionType_ID_PKIX_OCSP_NOCHECK,
  BraveExtensionType_ID_PKIX_OCSP_ARCHIVECUTOFF,
  BraveExtensionType_ID_PKIX_OCSP_SERVICELOCATOR,
  BraveExtensionType_HOLD_INSTRUCTION_CODE,

  //Certificate Transparency Extensions
  BraveExtensionType_CT_PRECERT_SCTS,
  BraveExtensionType_CT_CERT_SCTS,
};

#endif /* BRAVE_IOS_BROWSER_API_NETWORKING_COMMON_BRAVE_CERTIFICATE_ENUMS_H_ */
