#import "brave/ios/browser/api/networking/brave_certificate_model.h"
#include "base/strings/sys_string_conversions.h"
#import "net/base/mac/url_conversions.h"
#include "third_party/boringssl/src/include/openssl/opensslconf.h"
#if TARGET_CPU_ARM
#include "third_party/boringssl/src/include/openssl/arm_arch.h"
#endif
#include "third_party/boringssl/src/include/openssl/bio.h"
#include "third_party/boringssl/src/include/openssl/crypto.h"
#include "third_party/boringssl/src/include/openssl/x509.h"
#include "third_party/boringssl/src/include/openssl/x509v3.h"
#include "third_party/boringssl/src/include/openssl/asn1.h"
#include "third_party/boringssl/src/include/openssl/bn.h"
#include "third_party/boringssl/src/include/openssl/evp.h"
#include "third_party/boringssl/src/include/openssl/rsa.h"
#include "third_party/boringssl/src/include/openssl/dsa.h"
#include "third_party/boringssl/src/include/openssl/dh.h"
#include "third_party/boringssl/src/include/openssl/ec.h"
#include "url/gurl.h"

#include <cctype>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// MARK: - x509_utils

namespace x509_utils {
namespace {
  static const char hex_characters[] = "0123456789ABCDEF";
} // namespace
  
const char* EC_curve_nid2nist(int nid) {
  switch (nid) {
    case NID_sect163r2:
      return "B-163";
    case NID_sect233r1:
      return "B-233";
    case NID_sect283r1:
      return "B-283";
    case NID_sect409r1:
      return "B-409";
    case NID_sect571r1:
      return "B-571";
    case NID_sect163k1:
      return "K-163";
    case NID_sect233k1:
      return "K-233";
    case NID_sect283k1:
      return "K-283";
    case NID_sect409k1:
      return "K-409";
    case NID_sect571k1:
      return "K-571";
    case NID_X9_62_prime192v1:
      return "P-192";
    case NID_secp224r1:
      return "P-224";
    case NID_X9_62_prime256v1:
      return "P-256";
    case NID_secp384r1:
      return "P-384";
    case NID_secp521r1:
      return "P-521";
    default:
      return ::EC_curve_nid2nist(nid);
  }
}

int EC_curve_nid2num_bits(int nid) {
  switch (nid) {
    case NID_sect163r2:
      return 163;
    case NID_sect233r1:
      return 233;
    case NID_sect283r1:
      return 283;
    case NID_sect409r1:
      return 409;
    case NID_sect571r1:
      return 571;
    case NID_sect163k1:
      return 163;
    case NID_sect233k1:
      return 233;
    case NID_sect283k1:
      return 283;
    case NID_sect409k1:
      return 409;
    case NID_sect571k1:
      return 571;
    case NID_X9_62_prime192v1:
      return 192;
    case NID_secp224r1:
      return 224;
    case NID_X9_62_prime256v1:
      return 256;
    case NID_secp384r1:
      return 384;
    case NID_secp521r1:
      return 521;
    default:
      return 0;
  }
}

std::string string_from_ASN1STRING(const ASN1_STRING* string)
{
  if (ASN1_STRING_type(string) != V_ASN1_UTF8STRING) {
    unsigned char* utf8_string = nullptr;
    int length = ASN1_STRING_to_UTF8(&utf8_string, const_cast<ASN1_STRING*>(string));
    if (utf8_string) {
      if (length > 0) {
        std::string result = std::string(reinterpret_cast<char*>(utf8_string), length);
        OPENSSL_free(utf8_string);
        return result;
      }
      OPENSSL_free(utf8_string);
      return std::string();
    }
    return std::string();
  }
  
  std::size_t length = ASN1_STRING_length(string);
  const unsigned char* utf8_string = ASN1_STRING_get0_data(string);
  return std::string(reinterpret_cast<const char*>(utf8_string), length);
}

std::string hex_string_from_ASN1STRING(const ASN1_STRING* string)
{
  std::size_t length = ASN1_STRING_length(string);
  const unsigned char* data = ASN1_STRING_get0_data(string);
  
  if (!data || !length) {
    return std::string();
  }
  
  BIO* bio = BIO_new(BIO_s_mem());
  if (!bio) {
    return std::string();
  }
  
  for (std::size_t i = 0; i < length; ++i) {
    BIO_printf(bio, "%02X", data[i]);
  }
  
  char* bio_memory = nullptr;
  std::size_t total_size = BIO_get_mem_data(bio, &bio_memory);
  
  if (total_size > 0 && bio_memory) {
    std::string result = std::string(bio_memory, total_size);
    BIO_free_all(bio);
    return result;
  }
  BIO_free_all(bio);
  return std::string();
}

std::string hex_string_from_ASN1_BIT_STRING(ASN1_BIT_STRING* string) {
  std::size_t length = ASN1_STRING_length(string);
  const unsigned char* data = ASN1_STRING_get0_data(string);
  
  if (!data || !length) {
    return std::string();
  }
  
  BIO* bio = BIO_new(BIO_s_mem());
  if (!bio) {
    return std::string();
  }
  
  for (std::size_t i = 0; i < length; ++i) {
    BIO_printf(bio, "%02X", data[i]);
  }
  
  char* bio_memory = nullptr;
  std::size_t total_size = BIO_get_mem_data(bio, &bio_memory);
  
  if (total_size > 0 && bio_memory) {
    std::string result = std::string(bio_memory, total_size);
    BIO_free_all(bio);
    return result;
  }
  BIO_free_all(bio);
  return std::string();
}

std::string hex_string_from_ASN1INTEGER(ASN1_INTEGER* integer)
{
  std::string result;
  if (integer) {
    int length = ASN1_STRING_length(integer);
    const unsigned char* data = ASN1_STRING_get0_data(integer);
    if (data) {
      for (int i = 0; i < length; ++i) {
        result += hex_characters[(data[i] & 0xF0) >> 4];
        result += hex_characters[(data[i] & 0x0F) >> 0];
      }
    }
  }
  return result;
}

double date_from_ASN1TIME(const ASN1_TIME* date)
{
  ASN1_TIME* epoch = ASN1_TIME_new();
  ASN1_TIME_set_string(epoch, "700101000000Z");
  
  std::int32_t days = 0;
  std::int32_t seconds = 0;
  ASN1_TIME_diff(&days, &seconds, epoch, date);
  ASN1_STRING_free(epoch);
  return (days * 24 * 60 * 60) + seconds;
}

std::string name_entry_from_nid(X509_NAME* name, std::int32_t nid)
{
  if (name) {
    std::int32_t index = X509_NAME_get_index_by_NID(name, nid, -1);
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, index);
    
    if (entry) {
      const ASN1_STRING* issuerNameASN1 = X509_NAME_ENTRY_get_data(entry);
      const unsigned char* issuerName = ASN1_STRING_get0_data(issuerNameASN1);
      return reinterpret_cast<const char*>(issuerName);
    }
  }
  return std::string();
}

std::unordered_map<std::string, std::string> name_entry_to_map(X509_NAME* name)
{
  std::unordered_map<std::string, std::string> result;
  for (int i = 0; i < X509_NAME_entry_count(name); ++i) {
    X509_NAME_ENTRY* entry = X509_NAME_get_entry(name, i);
    if (entry) {
      ASN1_OBJECT* object = X509_NAME_ENTRY_get_object(entry);
      ASN1_STRING* data = X509_NAME_ENTRY_get_data(entry);
      
      if (object) {
        int nid = OBJ_obj2nid(object);
        if (nid != NID_undef) {
          const char* key_name = OBJ_nid2sn(nid); //OBJ_nid_2ln
          result[key_name] = string_from_ASN1STRING(data);
        }
      }
    }
  }
  return result;
}

std::string serial_number_from_certificate(X509* certificate) {
  ASN1_INTEGER* serial_number = X509_get_serialNumber(certificate);
  if (serial_number) {
    BIGNUM* big_num_serial = ASN1_INTEGER_to_BN(serial_number, nullptr);
    if (big_num_serial) {
      char* dec = BN_bn2dec(big_num_serial);
      if (dec) { //BoringSSL doesn't have ASN1_INTEGER_get_int64/ASN1_INTEGER_get_uint64
        errno = 0;
        char* endptr = nullptr;
        long long int llres = std::strtoll(dec, &endptr, 10);
        bool success = endptr != dec && errno == 0 && endptr && *endptr == '\0';
        if (success) {
          OPENSSL_free(dec);
          BN_free(big_num_serial);
          return std::to_string(llres);
        }
        
        errno = 0;
        endptr = nullptr;
        unsigned long long int ullres = std::strtoull(dec, &endptr, 10);
        success = endptr != dec && errno == 0 && endptr && *endptr == '\0';
        if (success) {
          OPENSSL_free(dec);
          BN_free(big_num_serial);
          return std::to_string(ullres);
        }
        
        OPENSSL_free(dec);
        dec = nullptr;
      }
      
      // Could not convert to decimal, so we convert to hex
      char* hex = BN_bn2hex(big_num_serial);
      if (hex) {
        std::string result = hex;
        OPENSSL_free(hex);
        BN_free(big_num_serial);
        std::transform(result.begin(),
                       result.end(),
                       result.begin(),
                       [](unsigned char c) -> unsigned char { return std::toupper(c); });
        return result;
      }
      BN_free(big_num_serial);
    }
  }
  return std::string();
}

std::string fingerprint_with_nid(X509* certificate, int nid) {
  const EVP_MD* digest = EVP_get_digestbynid(nid);
  if (digest) {
    unsigned int length = 0;
    unsigned char md[EVP_MAX_MD_SIZE] = {0};
    X509_digest(certificate, digest, &md[0], &length);
    
    std::string result;
    for (unsigned int i = 0; i < length; ++i) {
      result += hex_characters[(md[i] & 0xF0) >> 4];
      result += hex_characters[(md[i] & 0x0F) >> 0];
    }
    return result;
  }
  return std::string();
}

std::string string_from_x509_algorithm(const X509_ALGOR* algorithm) {
  std::string algorithm_string;
  const ASN1_OBJECT* oid = nullptr;
  X509_ALGOR_get0(&oid, nullptr, nullptr, algorithm);
  
  if (oid) {
    BIO* bio = BIO_new(BIO_s_mem());
    if (bio) {
      i2a_ASN1_OBJECT(bio, oid);
      
      char* bio_memory = nullptr;
      std::size_t total_size = BIO_get_mem_data(bio, &bio_memory);
      
      if (total_size > 0 && bio_memory) {
        algorithm_string = std::string(bio_memory, total_size);
      }
      
      BIO_free_all(bio);
    }
  }
  return algorithm_string;
}

std::string string_from_ASN1_OBJECT(const ASN1_OBJECT* object, bool no_name) {
  if (OBJ_length(object) > 0) {
    const int initial_buffer_size = 128; // spec says 80 should be large enough
    std::string result = std::string(initial_buffer_size, '\0');
    int max_size = OBJ_obj2txt(&result[0], initial_buffer_size, object, no_name);
    if (max_size > initial_buffer_size - 1) {
      result.resize(max_size + 1);
      OBJ_obj2txt(&result[0], max_size + 1, object, no_name);
    }
    result.erase(result.find('\0'));
    return result;
  }
  return std::string();
}

class X509_EXTENSION_INFO {
public:
  enum TYPE {
    STRING,
    MULTI_VALUE
  };
  
  X509_EXTENSION_INFO(const std::string& value) {
    this->value_type = TYPE::STRING;
    this->value = new std::string(value);
    this->is_multi_line = false;
  }
  
  X509_EXTENSION_INFO(const std::vector<std::pair<std::string,
                                                  std::string>>& values,
                                            bool is_multi_line) {
    this->value_type = TYPE::MULTI_VALUE;
    this->value = new std::vector<std::pair<std::string,
                                            std::string>>(values);
    this->is_multi_line = is_multi_line;
  }
  
  ~X509_EXTENSION_INFO() {
    switch (value_type) {
      case TYPE::STRING: {
        delete static_cast<std::string*>(value);
      }
        break;
        
      case TYPE::MULTI_VALUE: {
        delete static_cast<std::vector<
                              std::pair<std::string,
                                        std::string>>*>(value);
      }
        break;
    }
  }
  
  TYPE get_type() {
    return value_type;
  }
  
  bool has_multi_line_flag() {
    return is_multi_line;
  }
  
  std::string* get_extension_string() {
    if (value_type == TYPE::STRING) {
      return static_cast<std::string*>(value);
    }
    return nullptr;
  }
  
  std::vector<std::pair<std::string, std::string>>* get_extension_multi_value() {
    if (value_type == TYPE::MULTI_VALUE) {
      return static_cast<std::vector<
                            std::pair<std::string,
                                      std::string>>*>(value);
    }
    return nullptr;
  }
  
private:
  void* value;
  TYPE value_type;
  bool is_multi_line;
};

std::unique_ptr<X509_EXTENSION_INFO> decode_extension_info(X509_EXTENSION* extension) {
  auto cleanup_extension_string = [](const X509V3_EXT_METHOD* method,
                                     void* extension_string) {
    if (method->it) {
      ASN1_item_free(static_cast<ASN1_VALUE*>(extension_string),
                     ASN1_ITEM_ptr(method->it));
    } else {
      method->ext_free(extension_string);
    }
  };
  
  const X509V3_EXT_METHOD* method = X509V3_EXT_get(extension);
  if (method) {
    void* extension_string = nullptr;
    ASN1_OCTET_STRING* extension_data = X509_EXTENSION_get_data(extension);
    const unsigned char* der_data = ASN1_STRING_get0_data(extension_data);
    if (method->it) {
      // ASN1_ITEM from BER encoding
      extension_string = ASN1_item_d2i(nullptr,
                                       &der_data,
                                       ASN1_STRING_length(extension_data),
                                       ASN1_ITEM_ptr(method->it));
    } else {
      extension_string = method->d2i(nullptr,
                                     &der_data,
                                     ASN1_STRING_length(extension_data));
    }
    
    if (extension_string) {
      if (method->i2s) { // STRING EXTENSION
        char* value = method->i2s(method, extension_string);
        if (value) {
          std::string result = value;
          OPENSSL_free(value);
          cleanup_extension_string(method, extension_string);
          return std::make_unique<X509_EXTENSION_INFO>(result);
        }
      } else if (method->i2v) { // MULTI-PAIR EXTENSION
        STACK_OF(CONF_VALUE)* stack = method->i2v(method, extension_string, nullptr);
        if (stack) {
          bool is_multi_line = method->ext_flags & X509V3_EXT_MULTILINE;
          std::size_t count = sk_CONF_VALUE_num(stack);
          if (!count) {
            sk_CONF_VALUE_pop_free(stack, X509V3_conf_free);
            cleanup_extension_string(method, extension_string);
            return nullptr;
          }
          
          std::vector<std::pair<std::string, std::string>> values;
          for (std::size_t i = 0; i < count; ++i) {
            CONF_VALUE* item = sk_CONF_VALUE_value(stack, static_cast<int>(i));
            if (item) {
              if (!item->name) {
                values.push_back(std::make_pair("", item->value));
              } else if (!item->value) {
                values.push_back(std::make_pair(item->name, ""));
              } else {
                values.push_back(std::make_pair(item->name, item->value));
              }
            }
          }
          sk_CONF_VALUE_pop_free(stack, X509V3_conf_free);
          cleanup_extension_string(method, extension_string);
          return std::make_unique<X509_EXTENSION_INFO>(values, is_multi_line);
        }
      } else if (method->i2r) { // RAW EXTENSION
        std::string result;
        BIO* bio = BIO_new(BIO_s_mem());
        if (bio) {
          if (method->i2r(method, extension_string, bio, 0)) {
            char* bio_memory = nullptr;
            std::size_t total_size = BIO_get_mem_data(bio, &bio_memory);
            
            if (total_size > 0 && bio_memory) {
              result = std::string(bio_memory, total_size);
            }
            
            BIO_free_all(bio);
            cleanup_extension_string(method, extension_string);
            return std::make_unique<X509_EXTENSION_INFO>(result);
          }
          BIO_free_all(bio);
        }
      }
      
      cleanup_extension_string(method, extension_string);
    }
  }
  return nullptr;
}

NSString* string_to_ns(const std::string& str)
{
  if (str.empty()) {
    return [[NSString alloc] init];
  }
  return [NSString stringWithUTF8String:str.c_str()];
}

NSDate* date_to_ns(double time_interval)
{
  return [NSDate dateWithTimeIntervalSince1970:time_interval];
}
} // x509_utils

// MARK: - Implementation

@implementation BraveCertificateSubjectModel
- (instancetype)initWithCertificate:(X509*)certificate {
  if ((self = [super init])) {
    X509_NAME* subject_name = X509_get_subject_name(certificate);
    _countryOrRegion = x509_utils::string_to_ns(
                           x509_utils::name_entry_from_nid(subject_name,
                                                           NID_countryName));
    _stateOrProvince = x509_utils::string_to_ns(
                           x509_utils::name_entry_from_nid(subject_name,
                                                           NID_stateOrProvinceName));
    _locality = x509_utils::string_to_ns(
                    x509_utils::name_entry_from_nid(subject_name,
                                                    NID_localityName));
    _organization = x509_utils::string_to_ns(
                        x509_utils::name_entry_from_nid(subject_name,
                                                        NID_organizationName));
    _organizationalUnit = x509_utils::string_to_ns(
                              x509_utils::name_entry_from_nid(subject_name,
                                                              NID_organizationalUnitName));
    _commonName =  x509_utils::string_to_ns(
                       x509_utils::name_entry_from_nid(subject_name,
                                                       NID_commonName));
    _streetAddress =  x509_utils::string_to_ns(
                          x509_utils::name_entry_from_nid(subject_name,
                                                          NID_streetAddress));
    _domainComponent =  x509_utils::string_to_ns(
                            x509_utils::name_entry_from_nid(subject_name,
                                                            NID_domainComponent));
    _userId =  x509_utils::string_to_ns(
                   x509_utils::name_entry_from_nid(subject_name,
                                                   NID_userId));
  }
  return self;
}
@end

@implementation BraveCertificateIssuerName
- (instancetype)initWithCertificate:(X509*)certificate {
  if ((self = [super init])) {
    X509_NAME* issuer_name = X509_get_issuer_name(certificate);
    _countryOrRegion = x509_utils::string_to_ns(
                           x509_utils::name_entry_from_nid(issuer_name,
                                                           NID_countryName));
    _stateOrProvince = x509_utils::string_to_ns(
                           x509_utils::name_entry_from_nid(issuer_name,
                                                           NID_stateOrProvinceName));
    _locality = x509_utils::string_to_ns(
                    x509_utils::name_entry_from_nid(issuer_name,
                                                    NID_localityName));
    _organization = x509_utils::string_to_ns(
                        x509_utils::name_entry_from_nid(issuer_name,
                                                        NID_organizationName));
    _organizationalUnit = x509_utils::string_to_ns(
                              x509_utils::name_entry_from_nid(issuer_name,
                                                              NID_organizationalUnitName));
    _commonName =  x509_utils::string_to_ns(
                       x509_utils::name_entry_from_nid(issuer_name,
                                                       NID_commonName));
    _streetAddress =  x509_utils::string_to_ns(
                          x509_utils::name_entry_from_nid(issuer_name,
                                                          NID_streetAddress));
    _domainComponent =  x509_utils::string_to_ns(
                            x509_utils::name_entry_from_nid(issuer_name,
                                                            NID_domainComponent));
    _userId =  x509_utils::string_to_ns(
                   x509_utils::name_entry_from_nid(issuer_name,
                                                   NID_userId));
  }
  return self;
}
@end

@implementation BraveCertificateSignature
- (instancetype)initWithCertificate:(X509*)certificate {
  if ((self = [super init])) {
    _algorithm = [[NSString alloc] init];
    _objectIdentifier = [[NSString alloc] init];
    _signatureHexEncoded = [[NSString alloc] init];
    _parameters = [[NSString alloc] init];
    
    const ASN1_BIT_STRING* psig = nullptr;
    const X509_ALGOR* palg = nullptr;
    X509_get0_signature(&psig, &palg, certificate);
    
    if (psig) {
      std::string signature_hex_string = x509_utils::hex_string_from_ASN1STRING(psig);
      _signatureHexEncoded = x509_utils::string_to_ns(signature_hex_string);
      _bytesSize = ASN1_STRING_length(psig);
    }
    
    if (palg) {
      _algorithm = x509_utils::string_to_ns(
                                    x509_utils::string_from_x509_algorithm(palg));
      
      int param_type = 0;
      const void* param = nullptr;
      const ASN1_OBJECT* object = nullptr;
      X509_ALGOR_get0(&object, &param_type, &param, palg);
      
      if (object) {
        _objectIdentifier = x509_utils::string_to_ns(
                                             x509_utils::string_from_ASN1_OBJECT(object, true));
      }
      
      //const RSA_PSS_PARAMS *RSA_get0_pss_params(const RSA *r)
      //ASN1_TYPE_unpack_sequence(param, param_type);
      
      if (param) {
        if (param_type == V_ASN1_SEQUENCE) {
          const ASN1_STRING* seq_string = static_cast<const ASN1_STRING*>(param);
          _parameters = x509_utils::string_to_ns(
                                         x509_utils::hex_string_from_ASN1STRING(seq_string));
        }
      }
    }
  }
  return self;
}
@end

@implementation BraveCertificatePublicKeyInfo
// x509v3.h
//#define KU_DIGITAL_SIGNATURE 0x0080
//#define KU_NON_REPUDIATION 0x0040
//#define KU_KEY_ENCIPHERMENT 0x0020
//#define KU_DATA_ENCIPHERMENT 0x0010
//#define KU_KEY_AGREEMENT 0x0008
//#define KU_KEY_CERT_SIGN 0x0004
//#define KU_CRL_SIGN 0x0002
//#define KU_ENCIPHER_ONLY 0x0001
//#define KU_DECIPHER_ONLY 0x8000

- (instancetype)initWithCertificate:(X509*)certificate {
  if ((self = [super init])) {
    _type = BravePublicKeyType_UNKNOWN;
    _keyUsage = BravePublicKeyUsage_Invalid;
    _algorithm = [[NSString alloc] init];
    _objectIdentifier = [[NSString alloc] init];
    _curveName = [[NSString alloc] init];
    _nistCurveName = [[NSString alloc] init];
    _parameters = [[NSString alloc] init];
    _keyHexEncoded = [[NSString alloc] init];
    
    // Sometimes a key cannot be "decoded"
    // but you can still use X509_get_X509_PUBKEY to get its raw form
    EVP_PKEY* key = X509_get_pubkey(certificate);
    if (key) {
      _keySizeInBits = EVP_PKEY_bits(key);
      _keyBytesSize = _keySizeInBits / 8;
      int key_type = EVP_PKEY_base_id(key);
      
      switch (key_type) {
        case EVP_PKEY_RSA: {
          _type = BravePublicKeyType_RSA;
          const RSA* rsa_key = EVP_PKEY_get0_RSA(key);
          if (rsa_key) {
            const BIGNUM* n = nullptr;
            const BIGNUM* e = nullptr;
            const BIGNUM* d = nullptr;
            RSA_get0_key(rsa_key, &n, &e, &d);
            if (n && e) {
              _exponent = BN_get_word(e);
              //_keySizeInBits = BN_num_bits(n);
            }
          }
        }
          break;
        case EVP_PKEY_DSA: {
          _type = BravePublicKeyType_DSA;
          const DSA* dsa_key = EVP_PKEY_get0_DSA(key);
          if (dsa_key) {
            const BIGNUM* p = nullptr;
            const BIGNUM* q = nullptr;
            const BIGNUM* g = nullptr;
            DSA_get0_pqg(dsa_key, &p, &q, &g);
            if (p) {
              _keySizeInBits = BN_num_bits(p);
            }
          }
        }
          break;
        case EVP_PKEY_DH: {
          _type = BravePublicKeyType_DH;
          const DH* dh_key = EVP_PKEY_get0_DH(key);
          if (dh_key) {
            const BIGNUM* p = nullptr;
            const BIGNUM* q = nullptr;
            const BIGNUM* g = nullptr;
            DH_get0_pqg(dh_key, &p, &q, &g);
            if (p) {
              _keySizeInBits = BN_num_bits(p);
            }
          }
        }
          break;
        case EVP_PKEY_EC: {
          _type = BravePublicKeyType_EC;
          const EC_KEY* ec_key = EVP_PKEY_get0_EC_KEY(key);
          if (ec_key) {
            const EC_GROUP* group = EC_KEY_get0_group(ec_key);
            if (group) {
              //_keySizeInBits = EC_GROUP_get_degree(group); // EC_GROUP_order_bits //BN_num_bytes(EC_GROUP_get0_order());
              
              int name_nid = EC_GROUP_get_curve_name(group);
              _curveName = x509_utils::string_to_ns(OBJ_nid2sn(name_nid));
              _nistCurveName = x509_utils::string_to_ns(
                                                x509_utils::EC_curve_nid2nist(name_nid));
            }
          }
        }
          break;
      }
      EVP_PKEY_free(key);
    }
    
    // X509_PUB_KEY->public_key != EVP_PKEY
    ASN1_BIT_STRING* key_bits = X509_get0_pubkey_bitstr(certificate);
    if (key_bits) {
      _keyHexEncoded = x509_utils::string_to_ns(
                                    x509_utils::hex_string_from_ASN1_BIT_STRING(key_bits));
    }
    
    X509_PUBKEY* pub_key = X509_get_X509_PUBKEY(certificate);
    if (pub_key) {
      ASN1_OBJECT* object = nullptr;
      X509_ALGOR* palg = nullptr;
      X509_PUBKEY_get0_param(&object, nullptr, nullptr, &palg, pub_key);
      
      if (palg) {
        // same as x509_utils::string_from_ASN1_OBJECT(object, false)
        _algorithm = x509_utils::string_to_ns(
                                  x509_utils::string_from_x509_algorithm(palg));
      }
      
      if (object) {
        _objectIdentifier = x509_utils::string_to_ns(
                                 x509_utils::string_from_ASN1_OBJECT(object, true));
      }
      
      int param_type = 0;
      const void* param = nullptr;
      X509_ALGOR_get0(nullptr, &param_type, &param, palg);
      
      if (param) {
        if (param_type == V_ASN1_SEQUENCE) {
          const ASN1_STRING* seq_string = static_cast<const ASN1_STRING*>(param);
          _parameters = x509_utils::string_to_ns(
                                         x509_utils::hex_string_from_ASN1STRING(seq_string));
        }
        
        // Possible ECC certificate & Unsupported algorithms
        if (object && !key) {
          // Retrieve public key info
          int algorithm_key_type = OBJ_obj2nid(object);
          switch (algorithm_key_type) {
            case NID_rsaEncryption: {
              _type = BravePublicKeyType_RSA;
            }
              break;
            case NID_dsa: {
              _type = BravePublicKeyType_DSA;
            }
              break;
            case NID_dhpublicnumber:{
              _type = BravePublicKeyType_DH;
            }
              break;
            case NID_X9_62_id_ecPublicKey: {
              _type = BravePublicKeyType_EC;
              
              if (param_type == V_ASN1_SEQUENCE) {
                const ASN1_STRING* sequence = reinterpret_cast<const ASN1_STRING*>(param);
                _curveName = x509_utils::string_to_ns(
                                              x509_utils::string_from_ASN1STRING(sequence));
                //EC_KEY* ec_key = d2i_ECParameters(nullptr, &data, length);
              } else if (param_type == V_ASN1_OBJECT) {
                const ASN1_OBJECT* object = reinterpret_cast<const ASN1_OBJECT*>(param);
                _curveName = x509_utils::string_to_ns(
                                              x509_utils::string_from_ASN1_OBJECT(object, false));
                int nid = OBJ_obj2nid(object);
                _nistCurveName = x509_utils::string_to_ns(
                                                  x509_utils::EC_curve_nid2nist(nid));
                _keySizeInBits = x509_utils::EC_curve_nid2num_bits(nid);
                _keyBytesSize = _keySizeInBits / 8;
                //EC_GROUP* group = EC_GROUP_new_by_curve_name(nid);
              }
            }
              break;
              
            case V_ASN1_UNDEF: {
              _type = BravePublicKeyType_UNKNOWN;
            }
              break;
              
            default: {
              _type = BravePublicKeyType_UNKNOWN;
            }
              break;
          }
        }
      }
    }
    
    //int key_usage = X509_get_key_usage(certificate)
    ASN1_BIT_STRING* key_usage = static_cast<ASN1_BIT_STRING*>(
                                                 X509_get_ext_d2i(certificate,
                                                                  NID_key_usage,
                                                                  nullptr,
                                                                  nullptr));
    
    if (key_usage) {
      const unsigned char* data = ASN1_STRING_get0_data(key_usage);
      if (data && ASN1_STRING_length(key_usage) > 0) {
        if (data[0] & KU_DIGITAL_SIGNATURE) {
          //verify/sign
          _keyUsage |= BravePublicKeyUsage_Verify;
        }
        
        if (data[0] & KU_KEY_ENCIPHERMENT) {
          //wrap
          _keyUsage |= BravePublicKeyUsage_Wrap;
        }
        
        if (data[0] & KU_KEY_AGREEMENT) {
          //derive
          _keyUsage |= BravePublicKeyUsage_Derive;
        }
        
        if (data[0] & KU_DATA_ENCIPHERMENT ||
            (data[0] & KU_KEY_AGREEMENT &&
             data[0] & KU_KEY_ENCIPHERMENT)) {
          //encrypt
          _keyUsage |= BravePublicKeyUsage_Encrpyt;
        }
      }
      ASN1_BIT_STRING_free(key_usage);
    } else {
      //encrypt, verify, derive, wrap
      _keyUsage = BravePublicKeyUsage_Any;
    }
  }
  return self;
}

- (bool)isValidRSAKey:(RSA*)rsa_key {
  const BIGNUM* n = nullptr;
  const BIGNUM* e = nullptr;
  RSA_get0_key(rsa_key, &n, &e, nullptr);
  
  uint64_t exponent = BN_get_word(e);
  int modulus = BN_num_bits(n);
  
  if (exponent != 3 && exponent != 65537) {
    return false;
  }
  
  if (modulus != 1024 && modulus != 2048 && modulus != 3072 && modulus != 4096 && modulus != 8192) {
    return false;
  }
  return true;
}

- (NSString *)getECCurveName:(EC_KEY*)ec_key {
  const EC_GROUP* group = EC_KEY_get0_group(ec_key);
  if (group) {
    int name_nid = EC_GROUP_get_curve_name(group);
    return x509_utils::string_to_ns(OBJ_nid2sn(name_nid));
  }
  return [[NSString alloc] init];
}
@end

@implementation BraveCertificateFingerprint
- (instancetype)initWithCertificate:(X509*)certificate withType:(BraveFingerprintType)type {
  if ((self = [super init])) {
    //OpenSSL_add_all_digests()
    _type = type;
    
    switch (type) {
      case BraveFingerprintType_SHA1: {
        _fingerprintHexEncoded =
            x509_utils::string_to_ns(
                              x509_utils::fingerprint_with_nid(certificate,
                                                               NID_sha1));
      }
        break;
      case BraveFingerprintType_SHA256:{
        _fingerprintHexEncoded =
            x509_utils::string_to_ns(
                              x509_utils::fingerprint_with_nid(certificate,
                                                               NID_sha256));
      }
        break;
    }
  }
  return self;
}
@end

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
  _serialNumber = x509_utils::string_to_ns(x509_utils::serial_number_from_certificate(x509_cert_));
  _version = X509_get_version(x509_cert_) + 1;
  _signature = [[BraveCertificateSignature alloc] initWithCertificate:x509_cert_];
  _notValidBefore = x509_utils::date_to_ns(x509_utils::date_from_ASN1TIME(X509_get0_notBefore(x509_cert_)));
  _notValidAfter = x509_utils::date_to_ns(x509_utils::date_from_ASN1TIME(X509_get0_notAfter(x509_cert_)));
  _publicKeyInfo = [[BraveCertificatePublicKeyInfo alloc] initWithCertificate:x509_cert_];
  _extensions = nullptr;
  _sha1Fingerprint = [[BraveCertificateFingerprint alloc] initWithCertificate:x509_cert_
                                                                     withType:BraveFingerprintType_SHA1];
  _sha256Fingerprint = [[BraveCertificateFingerprint alloc] initWithCertificate:x509_cert_
                                                                       withType:BraveFingerprintType_SHA256];
  
  [self debugExtensions];
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
        
        // NOTE: BoringSSL doesn't support SCT (Signed Certificate Timestamps) yet..
        // So it will print the raw ASN.1 structure of 1.3.6.1.4.1.11129.2.4.2 instead of the Embedded Certificate Info
        // So we will just put the name, and the hex encoded extension information of the sequence.
        
        // Handle unsupported extension: Such as SCT, etc.
//        if (!X509_supported_extension(extension)) {
//          BIO* bio = BIO_new_fp(stderr, BIO_NOCLOSE);
//          if (bio) {
//            ASN1_OCTET_STRING* value = X509_EXTENSION_get_data(extension); // extension->value
//            ASN1_STRING_print(bio, value);
//            BIO_free(bio);
//          }
//          continue;
//        }
        
        // Extension is supported. Decompose it into the correct models.
        std::unique_ptr<x509_utils::X509_EXTENSION_INFO> info =
                                            x509_utils::decode_extension_info(extension);
        if (info) {
          if (info->get_type() == x509_utils::X509_EXTENSION_INFO::TYPE::STRING) {
            result += *info->get_extension_string();
            result += "\n";
          } else if (info->get_type() == x509_utils::X509_EXTENSION_INFO::TYPE::MULTI_VALUE) {
            auto vec = info->get_extension_multi_value();
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

