//
//  brave_certificate_enums.h
//  CertificateViewer
//
//  Created by Brandon on 2021-05-18.
//

#ifndef brave_certificate_enums_h
#define brave_certificate_enums_h

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

typedef NS_OPTIONS(NSUInteger, BraveGeneralNameType) {
  BraveGeneralNameType_INVALID    = 1 << 0,
  BraveGeneralNameType_OTHER_NAME = 1 << 1,
  BraveGeneralNameType_EMAIL      = 1 << 2,
  BraveGeneralNameType_DNS        = 1 << 3,
  BraveGeneralNameType_X400       = 1 << 4,
  BraveGeneralNameType_DIRNAME    = 1 << 5,
  BraveGeneralNameType_EDIPARTY   = 1 << 6,
  BraveGeneralNameType_URI        = 1 << 7,
  BraveGeneralNameType_IPADD      = 1 << 8,
  BraveGeneralNameType_RID        = 1 << 9
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

#endif /* brave_certificate_enums_h */
