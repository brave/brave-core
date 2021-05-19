//
//  brave_certificate_utils.mm
//  CertificateViewer
//
//  Created by Brandon on 2021-05-18.
//

#include "brave_certificate_utils.h"

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
  #include "third_party/boringssl/src/include/openssl/bn.h"
  #include "third_party/boringssl/src/include/openssl/evp.h"
  #include "third_party/boringssl/src/include/openssl/rsa.h"
  #include "third_party/boringssl/src/include/openssl/dsa.h"
  #include "third_party/boringssl/src/include/openssl/dh.h"
  #include "third_party/boringssl/src/include/openssl/ec.h"
  #include "third_party/boringssl/src/include/openssl/objects.h"

  #ifndef OPENSSL_NO_OCSP
    #include "third_party/boringssl/src/include/openssl/ocsp.h"
  #endif

  #ifndef OPENSSL_NO_CT
    #include "third_party/boringssl/src/include/openssl/ct.h"
  #endif
#else
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
  #include <openssl/bn.h>
  #include <openssl/evp.h>
  #include <openssl/rsa.h>
  #include <openssl/dsa.h>
  #include <openssl/dh.h>
  #include <openssl/ec.h>
  #include <openssl/objects.h>

  #ifndef OPENSSL_NO_OCSP
    #include <openssl/ocsp.h>
  #endif

  #ifndef OPENSSL_NO_CT
    #include <openssl/ct.h>
  #endif
#endif

#include <cctype>
#include <unordered_map>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

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

std::string int_to_hex_string(std::uint64_t value) {
  static const std::size_t char_bit = std::numeric_limits<unsigned char>::digits;
  std::string result((sizeof(value) * char_bit + 3) / 4 + 1, '\0');
  snprintf(&result[0], result.size(), "%llX", value);
  return result;
}

std::string hex_string_from_bytes(const std::vector<std::uint8_t>& bytes) {
  std::string result;
  for (std::size_t i = 0; i < bytes.size(); ++i) {
    result += hex_characters[(bytes[i] & 0xF0) >> 4];
    result += hex_characters[(bytes[i] & 0x0F) >> 0];
  }
  return result;
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

std::string string_from_ASN1INTEGER(ASN1_INTEGER* integer)
{
  std::string result;
  if (integer) {
    char* res = i2s_ASN1_INTEGER(nullptr, integer);
    if (res) {
      result = res;
      OPENSSL_free(res);
    }
  }
  return result;
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

std::string string_from_GENERAL_NAME(const GENERAL_NAME* name) {
  int type = 0;
  void* data = GENERAL_NAME_get0_value(name, &type);
  if (data) {
    switch (type) {
      case GEN_OTHERNAME: {
        OTHERNAME* other_name = static_cast<OTHERNAME*>(data);
        int nid = OBJ_obj2nid(other_name->type_id);
        if (nid == NID_ms_upn) {
          return x509_utils::string_from_ASN1STRING(other_name->value->value.asn1_string);
        }
        
        const char* name_string = OBJ_nid2ln(nid);
        if (name_string) {
          return "OTHER - UNSUPPORTED (" + std::string(name_string) + ")";
        }
      }
        break;
        
      case GEN_X400: {
        return "X400_NAME - UNSUPPORTED";
      }
      case GEN_EDIPARTY: {
        EDIPARTYNAME* edi_party_name = static_cast<EDIPARTYNAME*>(data);
        if (edi_party_name) {
          std::string result;
          ASN1_STRING* name_assigner = edi_party_name->nameAssigner;
          if (name_assigner) {
            result += string_from_ASN1STRING(name_assigner);
          }
          
          ASN1_STRING* party_name = edi_party_name->partyName;
          if (party_name) {
            if (name_assigner) {
              result += "\n";
            }
            result += string_from_ASN1STRING(party_name);
          }
          return result;
        }
      }
        break;
        
      case GEN_EMAIL: {
        if (name->d.ia5) {
          return x509_utils::string_from_ASN1STRING(name->d.ia5);
        }
      }
        break;
        
      case GEN_DNS: {
        if (name->d.ia5) {
          return x509_utils::string_from_ASN1STRING(name->d.ia5);
        }
      }
        break;
        
      case GEN_URI: {
        if (name->d.ia5) {
          return x509_utils::string_from_ASN1STRING(name->d.ia5);
        }
      }
        break;
        
      case GEN_DIRNAME: {
        X509_NAME* dir_name = name->d.directoryName;
        if (dir_name) {
          std::unordered_map<std::string, std::string> map_from_X509_NAME(X509_NAME* name);
          auto result = map_from_X509_NAME(dir_name);
        }
      }
        break;
        
      case GEN_IPADD: {
        ASN1_OCTET_STRING* ip_address = name->d.ip;
        if (ip_address) {
          const unsigned char* data = ASN1_STRING_get0_data(ip_address);
          if (data) {
            if (ASN1_STRING_length(ip_address) == 4) {
              std::string result;
              result += std::to_string(static_cast<int>(data[0]));
              result += ".";
              result += std::to_string(static_cast<int>(data[1]));
              result += ".";
              result += std::to_string(static_cast<int>(data[2]));
              result += ".";
              result += std::to_string(static_cast<int>(data[3]));
            } else if (ASN1_STRING_length(ip_address) == 16) {
              std::string result;
              for (std::int32_t i = 0; i < 8; ++i) {
                result += int_to_hex_string(static_cast<std::uint64_t>(data[0] << 8 | data[1]));
                data += 2;
              }
            }
          }
          return string_from_ASN1STRING(ip_address);
        }
      }
        break;
        
      case GEN_RID: {
        if (name->d.rid) {
          return string_from_ASN1_OBJECT(name->d.rid, false);
        }
      }
        break;
    }
  }
  return std::string();
}

void string_from_GENERAL_NAME(const GENERAL_NAME* name,
                              int& type,
                              std::string& other,
                              std::string& name_assigner,
                              std::string& party_name,
                              std::unordered_map<std::string, std::string>& dir_name) {
  void* data = GENERAL_NAME_get0_value(name, &type);
  if (!data) {
    other = std::string();
    name_assigner = std::string();
    party_name = std::string();
    dir_name = std::unordered_map<std::string, std::string>();
    return;
  }

  switch (type) {
    case GEN_OTHERNAME: {
      OTHERNAME* other_name = static_cast<OTHERNAME*>(data);
      int nid = OBJ_obj2nid(other_name->type_id);
      if (nid == NID_ms_upn) {
        other = x509_utils::string_from_ASN1STRING(other_name->value->value.asn1_string);
      } else {
        const char* name_string = OBJ_nid2ln(nid);
        other = name_string ? std::string(name_string) : std::string();
      }
    }
      break;
      
    case GEN_X400: {
      other = std::string();
    }
      break;
      
    case GEN_EDIPARTY: {
      EDIPARTYNAME* edi_party_name = static_cast<EDIPARTYNAME*>(data);
      if (edi_party_name) {
        if (edi_party_name->nameAssigner) {
          name_assigner = string_from_ASN1STRING(edi_party_name->nameAssigner);
        } else {
          name_assigner = std::string();
        }
        
        if (edi_party_name->partyName) {
          party_name = string_from_ASN1STRING(edi_party_name->partyName);
        } else {
          party_name = std::string();
        }
      } else {
        name_assigner = std::string();
        party_name = std::string();
      }
    }
      break;
      
    case GEN_EMAIL: {
      if (name->d.ia5) {
        other = x509_utils::string_from_ASN1STRING(name->d.ia5);
      } else {
        other = std::string();
      }
    }
      break;
      
    case GEN_DNS: {
      if (name->d.ia5) {
        other = x509_utils::string_from_ASN1STRING(name->d.ia5);
      } else {
        other = std::string();
      }
    }
      break;
      
    case GEN_URI: {
      if (name->d.ia5) {
        other = x509_utils::string_from_ASN1STRING(name->d.ia5);
      } else {
        other = std::string();
      }
    }
      break;
      
    case GEN_DIRNAME: {
      if (name->d.directoryName) {
        std::unordered_map<std::string, std::string> map_from_X509_NAME(X509_NAME* name);
        dir_name = map_from_X509_NAME(name->d.directoryName);
      } else {
        dir_name = std::unordered_map<std::string, std::string>();
      }
    }
      break;
      
    case GEN_IPADD: {
      ASN1_OCTET_STRING* ip_address = name->d.ip;
      if (ip_address) {
        const unsigned char* data = ASN1_STRING_get0_data(ip_address);
        if (data) {
          if (ASN1_STRING_length(ip_address) == 4) {
            other = std::string();
            other += std::to_string(static_cast<int>(data[0]));
            other += ".";
            other += std::to_string(static_cast<int>(data[1]));
            other += ".";
            other += std::to_string(static_cast<int>(data[2]));
            other += ".";
            other += std::to_string(static_cast<int>(data[3]));
            return;
          } else if (ASN1_STRING_length(ip_address) == 16) {
            other = std::string();
            for (std::int32_t i = 0; i < 8; ++i) {
              other += int_to_hex_string(data[0] << 8 | data[1]);
              data += 2;
            }
            return;
          }
        }
        other = string_from_ASN1STRING(ip_address);
      } else {
        other = std::string();
      }
    }
      break;
      
    case GEN_RID: {
      if (name->d.rid) {
        other = string_from_ASN1_OBJECT(name->d.rid, false);
      } else {
        other = std::string();
      }
    }
      break;
  }
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

std::unordered_map<std::string, std::string> map_from_X509_NAME(X509_NAME* name)
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

std::unordered_map<std::string, std::string> map_from_X509_NAME_ENTRIES(STACK_OF(X509_NAME_ENTRY)* entries)
{
  std::unordered_map<std::string, std::string> result;
  for (std::size_t i = 0; i < sk_X509_NAME_ENTRY_num(entries); ++i) {
    X509_NAME_ENTRY* entry = sk_X509_NAME_ENTRY_value(entries, static_cast<int>(i));
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
} // namespace x509_utils


namespace x509_utils {

X509_EXTENSION_INFO::X509_EXTENSION_INFO(const std::string& value) {
  this->value_type = TYPE::STRING;
  this->value = new std::string(value);
  this->is_multi_line = false;
}
  
X509_EXTENSION_INFO::X509_EXTENSION_INFO(const std::vector<std::pair<std::string,
                                                                std::string>>& values,
                                         bool is_multi_line) {
  this->value_type = TYPE::MULTI_VALUE;
  this->value = new std::vector<std::pair<std::string,
                                          std::string>>(values);
  this->is_multi_line = is_multi_line;
}

X509_EXTENSION_INFO::X509_EXTENSION_INFO(X509_EXTENSION_INFO&& other) :
                                          value(nullptr),
                                          value_type(TYPE::STRING),
                                          is_multi_line(false) {
                                            
  using std::swap;
  swap(value_type, other.value_type);
  swap(value, other.value);
  swap(is_multi_line, other.is_multi_line);
}
  
X509_EXTENSION_INFO::~X509_EXTENSION_INFO() {
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
  
X509_EXTENSION_INFO::TYPE X509_EXTENSION_INFO::get_type() {
  return value_type;
}

bool X509_EXTENSION_INFO::has_multi_line_flag() {
  return is_multi_line;
}

std::string* X509_EXTENSION_INFO::get_extension_string() {
  if (value_type == TYPE::STRING) {
    return static_cast<std::string*>(value);
  }
  return nullptr;
}

std::vector<std::pair<std::string, std::string>>* X509_EXTENSION_INFO::get_extension_multi_value() {
  if (value_type == TYPE::MULTI_VALUE) {
    return static_cast<std::vector<
                          std::pair<std::string,
                                    std::string>>*>(value);
  }
  return nullptr;
}

X509_EXTENSION_INFO& X509_EXTENSION_INFO::operator=(X509_EXTENSION_INFO&& other) {
  if (this != &other) {
    using std::swap;
    swap(value_type, other.value_type);
    swap(value, other.value);
    swap(is_multi_line, other.is_multi_line);
  }
  return *this;
}
} // namespace x509_utils
