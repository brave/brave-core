/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "brave_certificate_model.h"

#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #include "third_party/boringssl/src/include/openssl/opensslconf.h"
  #if TARGET_CPU_ARM
    #include "third_party/boringssl/src/include/openssl/arm_arch.h"
  #endif
  #include "third_party/boringssl/src/include/openssl/bio.h"
  #include "third_party/boringssl/src/include/openssl/conf.h"
  #include "third_party/boringssl/src/include/openssl/crypto.h"
  #include "third_party/boringssl/src/include/openssl/x509.h"
  #include "third_party/boringssl/src/include/openssl/x509v3.h"
  #include "third_party/boringssl/src/include/openssl/pem.h"
  #include "third_party/boringssl/src/include/openssl/asn1.h"
  #include "third_party/boringssl/src/include/openssl/objects.h"
#else
  // OpenSSL
  #include <openssl/opensslconf.h>
  #if TARGET_CPU_ARM
    #include <openssl/arm_arch.h>
  #endif
  #include <openssl/bio.h>
  #include <openssl/conf.h>
  #include <openssl/crypto.h>
  #include <openssl/x509.h>
  #include <openssl/x509v3.h>
  #include <openssl/pem.h>
  #include <openssl/asn1.h>
  #include <openssl/objects.h>
#endif

// Utils
#if defined(BRAVE_CORE) // Compiling in Brave-Core
  #import "brave/ios/browser/api/networking/common/brave_certificate_enums.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_ios_utils.h"
  #include "brave/ios/browser/api/networking/utils/brave_certificate_utils.h"
#else
  #import "brave_certificate_enums.h"
  #include "brave_certificate_ios_utils.h"
  #include "brave_certificate_utils.h"
#endif

// Basic Classes
#if defined(BRAVE_CORE) // Compiling in Brave-Core
#import "brave/ios/browser/api/networking/models/BraveCertificate_Priv.h"
#import "brave/ios/browser/api/networking/models/BraveCertificateSubjectModel.h"
#import "brave/ios/browser/api/networking/models/BraveCertificateIssuerName.h"
#import "brave/ios/browser/api/networking/models/BraveCertificateSignature.h"
#import "brave/ios/browser/api/networking/models/BraveCertificatePublicKeyInfo.h"
#import "brave/ios/browser/api/networking/models/BraveCertificateFingerprint.h"
#else
#import "BraveCertificate_Priv.h"
#import "BraveCertificateSubjectModel.h"
#import "BraveCertificateIssuerName.h"
#import "BraveCertificateSignature.h"
#import "BraveCertificatePublicKeyInfo.h"
#import "BraveCertificateFingerprint.h"
#endif

#if defined(BRAVE_CORE) // Compiling in Brave-Core
// Generic Extension
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateGenericExtensionModel.h"

// PKIX Certificate Extensions
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateExtensionModel_Priv.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateBasicConstraintsExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateKeyUsageExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateExtendedKeyUsageExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateSubjectKeyIdentifierExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateAuthorityKeyIdentifierExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificatePrivateKeyUsagePeriodExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateSubjectAlternativeNameExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateIssuerAlternativeNameExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateAuthorityInformationAccessExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateSubjectInformationAccessExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateNameConstraintsExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificatePoliciesExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificatePolicyMappingsExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificatePolicyConstraintsExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateInhibitAnyPolicyExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateTLSFeatureExtensionModel.h"

// Netscape Certificate Extensions - Largely Obsolete
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateNetscapeCertTypeExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateNetscapeURLExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateNetscapeURLExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateNetscapeURLExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateNetscapeStringExtensionModel.h"

// Miscellaneous Certificate Extensions
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateSXNetExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateProxyCertInfoExtensionModel.h"

// PKIX CRL Extensions
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateCRLNumberExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateCRLDistributionPointsExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateDeltaCRLExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateCRLDistributionPointsExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateInvalidityDateExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateIssuingDistributionPointExtensionModel.h"

// CRL entry extensions from PKIX standards such as RFC5280
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateCRLReasonExtensionModel.h"
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateIssuerExtensionModel.h"

// OCSP Extensions
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificatePKIXOCSPNonceExtensionModel.h"

// Certificate Transparency Extensions
#import "brave/ios/browser/api/networking/models/extensions/BraveCertificateSCTExtensionModel.h"
#else
// Generic Extension
#import "BraveCertificateGenericExtensionModel.h"

// PKIX Certificate Extensions
#import "BraveCertificateExtensionModel_Priv.h"
#import "BraveCertificateBasicConstraintsExtensionModel.h"
#import "BraveCertificateKeyUsageExtensionModel.h"
#import "BraveCertificateExtendedKeyUsageExtensionModel.h"
#import "BraveCertificateSubjectKeyIdentifierExtensionModel.h"
#import "BraveCertificateAuthorityKeyIdentifierExtensionModel.h"
#import "BraveCertificatePrivateKeyUsagePeriodExtensionModel.h"
#import "BraveCertificateSubjectAlternativeNameExtensionModel.h"
#import "BraveCertificateIssuerAlternativeNameExtensionModel.h"
#import "BraveCertificateAuthorityInformationAccessExtensionModel.h"
#import "BraveCertificateSubjectInformationAccessExtensionModel.h"
#import "BraveCertificateNameConstraintsExtensionModel.h"
#import "BraveCertificatePoliciesExtensionModel.h"
#import "BraveCertificatePolicyMappingsExtensionModel.h"
#import "BraveCertificatePolicyConstraintsExtensionModel.h"
#import "BraveCertificateInhibitAnyPolicyExtensionModel.h"
#import "BraveCertificateTLSFeatureExtensionModel.h"

// Netscape Certificate Extensions - Largely Obsolete
#import "BraveCertificateNetscapeCertTypeExtensionModel.h"
#import "BraveCertificateNetscapeURLExtensionModel.h"
#import "BraveCertificateNetscapeURLExtensionModel.h"
#import "BraveCertificateNetscapeURLExtensionModel.h"
#import "BraveCertificateNetscapeStringExtensionModel.h"

// Miscellaneous Certificate Extensions
#import "BraveCertificateSXNetExtensionModel.h"
#import "BraveCertificateProxyCertInfoExtensionModel.h"

// PKIX CRL Extensions
#import "BraveCertificateCRLNumberExtensionModel.h"
#import "BraveCertificateCRLDistributionPointsExtensionModel.h"
#import "BraveCertificateDeltaCRLExtensionModel.h"
#import "BraveCertificateCRLDistributionPointsExtensionModel.h"
#import "BraveCertificateInvalidityDateExtensionModel.h"
#import "BraveCertificateIssuingDistributionPointExtensionModel.h"

// CRL entry extensions from PKIX standards such as RFC5280
#import "BraveCertificateCRLReasonExtensionModel.h"
#import "BraveCertificateIssuerExtensionModel.h"

// OCSP Extensions
#import "BraveCertificatePKIXOCSPNonceExtensionModel.h"

// Certificate Transparency Extensions
#import "BraveCertificateSCTExtensionModel.h"
#endif

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - Implementation


@interface BraveCertificateModel()
{
  X509* x509_cert_;
}
@end

@implementation BraveCertificateModel
- (nullable instancetype)initWithData:(NSData *)data {
  if ((self = [super init])) {
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>([data bytes]);
    if (!bytes) {
      return nullptr;
    }
    
    x509_cert_ = d2i_X509(nullptr, &bytes, [data length]);
    if (!x509_cert_) {
      BIO* bio = BIO_new(BIO_s_mem());
      if (bio) {
        BIO_write(bio, bytes, static_cast<int>([data length]));
        x509_cert_ = PEM_read_bio_X509(bio, nullptr, 0, nullptr);
        BIO_free_all(bio);
      }
      
      if (!x509_cert_) {
        return nullptr;
      }
    }
    
    [self parseCertificate];
  }
  return self;
}

- (nullable instancetype)initWithFilePath:(NSString *)path {
  if ((self = [super init])) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (bio) {
      if (BIO_read_filename(bio, [path UTF8String])) {
        x509_cert_ = d2i_X509_bio(bio, nullptr);
        if (!x509_cert_) {
          x509_cert_ = PEM_read_bio_X509(bio, nullptr, 0, nullptr);
        }
      }
      BIO_free_all(bio);
    }
    
    if (!x509_cert_) {
      return nullptr;
    }
    
    [self parseCertificate];
  }
  return self;
}

- (nullable instancetype)initWithCertificate:(nonnull SecCertificateRef)certificate {
  if ((self = [super init])) {
    NSData* data = (__bridge NSData*)SecCertificateCopyData(certificate);
    if (!data) {
      return nullptr;
    }
    
    const std::uint8_t* bytes = static_cast<const std::uint8_t*>([data bytes]);
    if (!bytes) {
      return nullptr;
    }
    
    x509_cert_ = d2i_X509(nullptr, &bytes, [data length]);
    if (!x509_cert_) {
      return nullptr;
    }
    
    [self parseCertificate];
  }
  return self;
}

- (void)dealloc {
  X509_free(x509_cert_);
}

- (void)parseCertificate {
  _isRootCertificate = [self is_root_certificate];
  _isCertificateAuthority = [self is_certificate_authority];
  _isSelfSigned = [self is_self_signed];
  _isSelfIssued = [self is_self_issued];
  _subjectName = [[BraveCertificateSubjectModel alloc] initWithCertificate:x509_cert_];
  _issuerName = [[BraveCertificateIssuerName alloc] initWithCertificate:x509_cert_];
  _serialNumber = brave::string_to_ns(x509_utils::serial_number_from_certificate(x509_cert_));
  _version = X509_get_version(x509_cert_) + 1;
  _signature = [[BraveCertificateSignature alloc] initWithCertificate:x509_cert_];
  _notValidBefore = brave::date_to_ns(x509_utils::date_from_ASN1TIME(X509_get0_notBefore(x509_cert_)));
  _notValidAfter = brave::date_to_ns(x509_utils::date_from_ASN1TIME(X509_get0_notAfter(x509_cert_)));
  _publicKeyInfo = [[BraveCertificatePublicKeyInfo alloc] initWithCertificate:x509_cert_];
  _extensions = nullptr;
  _sha1Fingerprint = [[BraveCertificateFingerprint alloc] initWithCertificate:x509_cert_
                                                                     withType:BraveFingerprintType_SHA1];
  _sha256Fingerprint = [[BraveCertificateFingerprint alloc] initWithCertificate:x509_cert_
                                                                       withType:BraveFingerprintType_SHA256];
  
  [self parseExtensions];
}

- (bool)is_root_certificate {
  X509_NAME* subject_name = X509_get_subject_name(x509_cert_);
  X509_NAME* issuer_name = X509_get_issuer_name(x509_cert_);
  
  if (subject_name && issuer_name) {
    return !X509_NAME_cmp(subject_name, issuer_name);
  }
  return false;
}

- (bool)is_certificate_authority {
  return X509_check_ca(x509_cert_);
}

- (bool)is_self_signed {
  return X509_get_extension_flags(x509_cert_) & EXFLAG_SS;
}

- (bool)is_self_issued {
  return X509_get_extension_flags(x509_cert_) & EXFLAG_SI;
}

- (void)parseExtensions {
  std::unordered_map<int, Class> supported_extensions = {
    //PKIX Certificate Extensions
    {NID_basic_constraints, [BraveCertificateBasicConstraintsExtensionModel class]},
    {NID_key_usage, [BraveCertificateKeyUsageExtensionModel class]},
    {NID_ext_key_usage, [BraveCertificateExtendedKeyUsageExtensionModel class]},
    {NID_subject_key_identifier, [BraveCertificateSubjectKeyIdentifierExtensionModel class]},
    {NID_authority_key_identifier, [BraveCertificateAuthorityKeyIdentifierExtensionModel class]},
    {NID_private_key_usage_period, [BraveCertificatePrivateKeyUsagePeriodExtensionModel class]},
    {NID_subject_alt_name, [BraveCertificateSubjectAlternativeNameExtensionModel class]},
    {NID_issuer_alt_name, [BraveCertificateIssuerAlternativeNameExtensionModel class]},
    {NID_info_access, [BraveCertificateAuthorityInformationAccessExtensionModel class]},
    {NID_sinfo_access, [BraveCertificateSubjectInformationAccessExtensionModel class]},
    {NID_name_constraints, [BraveCertificateNameConstraintsExtensionModel class]},
    {NID_certificate_policies, [BraveCertificatePoliciesExtensionModel class]},
    {NID_policy_mappings, [BraveCertificatePolicyMappingsExtensionModel class]},
    {NID_policy_constraints, [BraveCertificatePolicyConstraintsExtensionModel class]},
    {NID_inhibit_any_policy, [BraveCertificateInhibitAnyPolicyExtensionModel class]},
    #ifndef OPENSSL_IS_BORINGSSL
    {NID_tlsfeature, [BraveCertificateTLSFeatureExtensionModel class]},
    #elif NID_tlsfeature // Chromium's BoringSSL is missing so much stuff!
    {NID_tlsfeature, [BraveCertificateGenericExtensionModel class]},
    #endif
    
    //Netscape Certificate Extensions - Largely Obsolete
    {NID_netscape_cert_type, [BraveCertificateNetscapeCertTypeExtensionModel class]},
    {NID_netscape_base_url, [BraveCertificateNetscapeURLExtensionModel class]},
    {NID_netscape_revocation_url, [BraveCertificateNetscapeURLExtensionModel class]},
    {NID_netscape_ca_revocation_url, [BraveCertificateNetscapeURLExtensionModel class]},
    {NID_netscape_renewal_url, [BraveCertificateNetscapeURLExtensionModel class]},
    {NID_netscape_ca_policy_url, [BraveCertificateNetscapeURLExtensionModel class]},
    {NID_netscape_ssl_server_name, [BraveCertificateNetscapeStringExtensionModel class]},
    {NID_netscape_comment, [BraveCertificateNetscapeStringExtensionModel class]},
    
    //Miscellaneous Certificate Extensions
    #ifndef OPENSSL_IS_BORINGSSL
    {NID_sxnet, [BraveCertificateSXNetExtensionModel class]},
    #elif NID_sxnet // Chromium's BoringSSL is missing so much stuff!
    {NID_sxnet, [BraveCertificateGenericExtensionModel class]},
    #endif
    {NID_proxyCertInfo, [BraveCertificateProxyCertInfoExtensionModel class]},
    
    //PKIX CRL Extensions
    {NID_crl_number, [BraveCertificateCRLNumberExtensionModel class]},
    {NID_crl_distribution_points, [BraveCertificateCRLDistributionPointsExtensionModel class]},
    {NID_delta_crl, [BraveCertificateDeltaCRLExtensionModel class]},
    {NID_freshest_crl, [BraveCertificateCRLDistributionPointsExtensionModel class]},
    {NID_invalidity_date, [BraveCertificateInvalidityDateExtensionModel class]},
    {NID_issuing_distribution_point, [BraveCertificateIssuingDistributionPointExtensionModel class]},
    
    //CRL entry extensions from PKIX standards such as RFC5280
    {NID_crl_reason, [BraveCertificateCRLReasonExtensionModel class]},
    {NID_certificate_issuer, [BraveCertificateIssuerExtensionModel class]},
    
    //OCSP Extensions
    {NID_id_pkix_OCSP_Nonce, [BraveCertificatePKIXOCSPNonceExtensionModel class]},
    {NID_id_pkix_OCSP_CrlID, [BraveCertificateGenericExtensionModel class]},
    {NID_id_pkix_OCSP_acceptableResponses, [BraveCertificateGenericExtensionModel class]},
    {NID_id_pkix_OCSP_noCheck, [BraveCertificateGenericExtensionModel class]},
    {NID_id_pkix_OCSP_archiveCutoff, [BraveCertificateGenericExtensionModel class]},
    {NID_id_pkix_OCSP_serviceLocator, [BraveCertificateGenericExtensionModel class]},
    {NID_hold_instruction_code, [BraveCertificateGenericExtensionModel class]},
    
    //Certificate Transparency Extensions
    #ifndef OPENSSL_NO_CT
    {NID_ct_precert_scts, [BraveCertificateSCTExtensionModel class]},
    {NID_ct_cert_scts, [BraveCertificateSCTExtensionModel class]}
    #elif NID_ct_precert_scts && NID_ct_cert_scts // Chromium's BoringSSL is missing so much stuff!
    {NID_ct_precert_scts, [BraveCertificateGenericExtensionModel class]},
    {NID_ct_cert_scts, [BraveCertificateGenericExtensionModel class]}
    #endif
  };
  
  NSMutableArray* parsed_extensions = [[NSMutableArray alloc] init];
  const STACK_OF(X509_EXTENSION)* extensions_list = X509_get0_extensions(x509_cert_);
  if (extensions_list) {
    std::size_t count = sk_X509_EXTENSION_num(extensions_list); //X509_get_ext_count
    
    for (std::size_t i = 0; i < count; ++i) {
      X509_EXTENSION* extension = sk_X509_EXTENSION_value(extensions_list, static_cast<int>(i)); //X509_get_ext
      if (extension) {
        ASN1_OBJECT* object = X509_EXTENSION_get_object(extension);
        int nid = OBJ_obj2nid(object);
        auto supported_extension = supported_extensions.find(nid);
        if (supported_extension != supported_extensions.end() && supported_extension->second != nullptr) {
          // All supported extensions MUST be a BraveCertificateExtensionModel
          // Otherwise they should be parsed as a BraveCertificateGenericExtensionModel or using their own model
          if (![supported_extension->second isSubclassOfClass:[BraveCertificateExtensionModel class]]) {
            NSAssert(NO, @"INVALID EXTENSION CLASS!");
            continue;
          }
          
          // Extension is supported
          BraveCertificateExtensionModel* model = [[supported_extension->second alloc] initWithType:supported_extension->first withExtension:extension];
          [parsed_extensions addObject:model];
        } else {
          // Unsupported extension
          BraveCertificateExtensionModel* model = [[BraveCertificateGenericExtensionModel alloc] initWithType:nid withExtension:extension];
          [parsed_extensions addObject:model];
        }
      }
    }
  }
  
  _extensions = parsed_extensions;
}

- (void)debugExtensions {
  const STACK_OF(X509_EXTENSION)* extensions_list = X509_get0_extensions(x509_cert_);
  if (extensions_list) {
    std::size_t count = sk_X509_EXTENSION_num(extensions_list); //X509_get_ext_count
    
    for (std::size_t i = 0; i < count; ++i) {
      X509_EXTENSION* extension = sk_X509_EXTENSION_value(extensions_list, static_cast<int>(i)); //X509_get_ext
      if (extension) {
        ASN1_OBJECT* object = X509_EXTENSION_get_object(extension);
        bool is_critical = X509_EXTENSION_get_critical(extension);
        
        std::string result = x509_utils::string_from_ASN1_OBJECT(object, false);
        result += ": ";
        result += is_critical ? "true" : "false";
        result += "\n--------------------------\n";
        
        // Extension is possibly supported. Decompose it into the generic models.
        int nid = OBJ_obj2nid(object);
        auto* model = [[BraveCertificateGenericExtensionModel alloc] initWithType:nid withExtension:extension];
        if (model.extensionType == BraveGenericExtensionType_STRING) {
          result += [[model stringValue] UTF8String];
          result += "\n";
        } else if (model.extensionType == BraveGenericExtensionType_KEY_VALUE) {
          for (BraveCertificateGenericExtensionPairModel* pair in model.arrayValue) {
            if (![[pair key] length]) {
              result += [[pair value] UTF8String];
            } else if (![[pair value] length]) {
              result += [[pair key] UTF8String];
            } else {
              result += [[pair key] UTF8String];
              result += ": ";
              result += [[pair value] UTF8String];
            }
            result += "\n";
          }
        } else if (model.extensionType == BraveGenericExtensionType_HEX_STRING) {
          result += [[model stringValue] UTF8String];
          result += "\n";
        } else {
          NSAssert(NO, @"INVALID EXTENSION TYPE!");
        }

        printf("\n%s\n", result.c_str());
      }
    }
  }
}

- (void)debugPrint {
  BIO* bio = BIO_new_fp(stdout, BIO_NOCLOSE);
  X509_print(bio, x509_cert_);
  BIO_free(bio);
}

- (void)debugPrintExtensions {
  const STACK_OF(X509_EXTENSION)* extensions_list = X509_get0_extensions(x509_cert_);
  if (extensions_list) {
    BIO* bio = BIO_new_fp(stdout, BIO_NOCLOSE);
    if (bio) {
      X509V3_extensions_print(bio, "X509v3 extensions", extensions_list, X509_FLAG_COMPAT, 0);
      BIO_free(bio);
    }
  }
}
@end

