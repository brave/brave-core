/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifdef UNSAFE_BUFFERS_BUILD
// TODO(https://github.com/brave/brave-browser/issues/41661): Remove this and
// convert code to safer constructs.
#pragma allow_unsafe_buffers
#endif

#include "brave/ios/browser/api/storekit_receipt/storekit_receipt.h"

#include <vector>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "net/base/net_export.h"
#include "third_party/boringssl/src/include/openssl/asn1.h"
#include "third_party/boringssl/src/include/openssl/bio.h"
#include "third_party/boringssl/src/include/openssl/mem.h"
#include "third_party/boringssl/src/include/openssl/nid.h"
#include "third_party/boringssl/src/include/openssl/obj.h"
#include "third_party/boringssl/src/include/openssl/pkcs7.h"
#include "third_party/boringssl/src/include/openssl/pool.h"
#include "third_party/boringssl/src/pki/input.h"
#include "third_party/boringssl/src/pki/parse_values.h"
#include "third_party/boringssl/src/pki/parser.h"

// MARK: - Implementation

namespace brave {
namespace storekit_receipt {
bssl::UniquePtr<PKCS7> pkcs7_from_data(NSData* data) {
  const std::uint8_t* der_bytes =
      static_cast<const std::uint8_t*>([data bytes]);
  return bssl::UniquePtr<PKCS7>(d2i_PKCS7(nullptr, &der_bytes, [data length]));
}

bool decode_asn1_boolean(bssl::der::Input value) {
  std::uint8_t result = 0;
  bssl::der::Parser parser(value);
  if (parser.ReadUint8(&result)) {
    return static_cast<bool>(result);
  }
  return false;
}

NSUInteger decode_asn1_integer(bssl::der::Input value) {
  std::uint64_t result = 0;
  bssl::der::Parser parser(value);
  if (parser.ReadUint64(&result)) {
    return result;
  }
  return 0;
}

NSString* decode_asn1_string(bssl::der::Input value) {
  if (value.empty()) {
    return nullptr;
  }

  int cls = 0;
  int type = 0;
  long length = 0;

  const std::uint8_t* data = value.data();
  ASN1_get_object(&data, &length, &type, &cls, value.size());

  if (type == V_ASN1_IA5STRING) {
    std::string result;
    if (bssl::der::ParseIA5String(bssl::der::Input(base::span(
                                      data, static_cast<std::size_t>(length))),
                                  &result)) {
      return base::SysUTF8ToNSString(result);
    }
    return nullptr;
  }

  return base::SysUTF8ToNSString(
      std::string(reinterpret_cast<const char*>(data), length));
}

NSDate* decode_asn1_date(bssl::der::Input value) {
  NSString* date_string = decode_asn1_string(value);
  if (date_string) {
    NSISO8601DateFormatter* formatter = [[NSISO8601DateFormatter alloc] init];
    return [formatter dateFromString:date_string];
  }
  return nullptr;
}

bool pkcs7_get_signed_content(
    NSData* data,
    std::vector<bssl::UniquePtr<ASN1_STRING>>& result) {
  // 1.2.840.113549.1.7.2
  static const std::uint8_t kPKCS7SignedData[] = {0x2a, 0x86, 0x48, 0x86, 0xf7,
                                                  0x0d, 0x01, 0x07, 0x02};

  // Initialize cbs - No BER to DER conversion required
  CBS cbs;
  CBS_init(&cbs, static_cast<const std::uint8_t*>([data bytes]), [data length]);

  // Parse content info
  CBS content_info;
  if (!CBS_get_asn1(&cbs, &content_info, CBS_ASN1_SEQUENCE)) {
    return false;
  }

  // Parse content type
  CBS content_type;
  if (!CBS_get_asn1(&content_info, &content_type, CBS_ASN1_OBJECT)) {
    return false;
  }

  // We only care about signed data
  if (!CBS_mem_equal(&content_type, kPKCS7SignedData,
                     sizeof(kPKCS7SignedData))) {
    return false;
  }

  if (OBJ_cbs2nid(&content_type) != NID_pkcs7_signed) {
    return false;
  }

  // Unwrap the signed data
  CBS wrapped_signed_data;
  if (!CBS_get_asn1(&content_info, &wrapped_signed_data,
                    CBS_ASN1_CONTEXT_SPECIFIC | CBS_ASN1_CONSTRUCTED | 0)) {
    return false;
  }

  CBS signed_data;
  if (!CBS_get_asn1(&wrapped_signed_data, &signed_data, CBS_ASN1_SEQUENCE)) {
    return false;
  }

  // Parse version
  std::uint64_t version = 0;
  if (!CBS_get_asn1_uint64(&signed_data, &version) || version < 1) {
    return false;
  }

  // Parse Digests - We don't care about it
  if (!CBS_get_asn1(&signed_data, nullptr, CBS_ASN1_SET)) {
    return false;
  }

  // Parse EncapsulatedContentInfo
  CBS encapContentInfo;
  if (!CBS_get_asn1(&signed_data, &encapContentInfo, CBS_ASN1_SEQUENCE)) {
    return false;
  }

  // Parse ContentType
  CBS eContentType;
  if (!CBS_get_asn1(&encapContentInfo, &eContentType, CBS_ASN1_OBJECT)) {
    return false;
  }

  // Check if eContentType is data.
  if (OBJ_cbs2nid(&eContentType) != NID_pkcs7_data) {
    return false;
  }

  // Parse eContent Optional Container
  CBS eContentContainer;
  int has_content = 0;
  if (!CBS_get_optional_asn1(
          &encapContentInfo, &eContentContainer, &has_content,
          CBS_ASN1_CONTEXT_SPECIFIC | CBS_ASN1_CONSTRUCTED | 0)) {
    return false;
  }

  if (!has_content) {
    return false;
  }

  while (CBS_len(&eContentContainer) > 0) {
    CBS eContent;
    if (!CBS_get_asn1(&eContentContainer, &eContent, CBS_ASN1_OCTETSTRING)) {
      return false;
    }

    ASN1_STRING* octet = ASN1_STRING_type_new(V_ASN1_OCTET_STRING);
    if (octet) {
      ASN1_STRING_set(octet, CBS_data(&eContent), CBS_len(&eContent));
      result.push_back(bssl::UniquePtr<ASN1_STRING>(octet));
    }
  }

  return true;
}
}  // namespace storekit_receipt
}  // namespace brave

@implementation BraveStoreKitPurchase
- (instancetype)initWithData:(bssl::der::Input)data {
  if ((self = [super init])) {
    _quantity = 0;
    _productId = @"";
    _transactionId = @"";
    _originalTransactionId = @"";
    _purchaseDate = nil;
    _originalPurchaseDate = nil;
    _subscriptionExpirationDate = nil;
    _cancellationDate = nil;
    _webOrderLineItemId = 0;
    _isInIntroOfferPeriod = false;

    bssl::der::Parser set_parser;
    bssl::der::Parser octet_parser(data);

    if (!octet_parser.ReadConstructed(CBS_ASN1_SET, &set_parser)) {
      VLOG(1) << "Cannot parse receipt InAppPurchase ASN1_SET";
      return nullptr;
    }

    while (set_parser.HasMore()) {
      bssl::der::Parser sequence_parser;
      if (!set_parser.ReadSequence(&sequence_parser)) {
        VLOG(1) << "Cannot parse receipt InAppPurchase ASN1_SET -> Sequence";
        break;
      }

      std::uint64_t attribute_type = 0;
      if (!sequence_parser.ReadUint64(&attribute_type)) {
        break;
      }

      std::uint64_t unknown = 0;
      if (!sequence_parser.ReadUint64(&unknown)) {
        break;
      }

      bssl::der::Input value;
      if (!sequence_parser.ReadTag(CBS_ASN1_OCTETSTRING, &value)) {
        return nullptr;
      }

      switch (attribute_type) {
        case 1701: {
          _quantity = brave::storekit_receipt::decode_asn1_integer(value);
        } break;

        case 1702: {
          _productId = brave::storekit_receipt::decode_asn1_string(value);
        } break;

        case 1703: {
          _transactionId = brave::storekit_receipt::decode_asn1_string(value);
        } break;

        case 1704: {
          _purchaseDate = brave::storekit_receipt::decode_asn1_date(value);
        } break;

        case 1705: {
          _originalTransactionId =
              brave::storekit_receipt::decode_asn1_string(value);
        } break;

        case 1706: {
          _originalPurchaseDate =
              brave::storekit_receipt::decode_asn1_date(value);
        } break;

        case 1708: {
          _subscriptionExpirationDate =
              brave::storekit_receipt::decode_asn1_date(value);
        } break;

        case 1711: {
          _webOrderLineItemId =
              brave::storekit_receipt::decode_asn1_integer(value);
        } break;

        case 1712: {
          _cancellationDate = brave::storekit_receipt::decode_asn1_date(value);
        } break;

        case 1719: {
          _isInIntroOfferPeriod =
              brave::storekit_receipt::decode_asn1_boolean(value);
        } break;

        default:
          break;
      }
    }
  }
  return self;
}
@end

@implementation BraveStoreKitReceipt
- (nullable instancetype)initWithData:(NSData*)data {
  if ((self = [super init])) {
    _bundleId = @"";
    _appVersion = @"";
    _opaqueData = nil;
    _sha1Hash = @"";
    _inAppPurchaseReceipts = [[NSMutableArray alloc] init];
    _originalApplicationVersion = @"";
    _receiptCreationDate = nil;
    _receiptExpirationDate = nil;

    bssl::UniquePtr<PKCS7> pkcs =
        brave::storekit_receipt::pkcs7_from_data(data);

    if (!pkcs) {
      VLOG(1) << "Cannot parse receipt pkcs7 container";
      return nullptr;
    }

    if (!PKCS7_type_is_signed(pkcs.get())) {
      VLOG(1) << "PKCS7 container is contains no signature";
      return nullptr;
    }

    std::vector<bssl::UniquePtr<ASN1_STRING>> result;
    if (!brave::storekit_receipt::pkcs7_get_signed_content(data, result)) {
      return nullptr;
    }

    for (auto&& octet : result) {
      auto receipt_input_ = bssl::der::Input(base::span(
          ASN1_STRING_data(octet.get()),
          static_cast<std::size_t>(ASN1_STRING_length(octet.get()))));

      if (receipt_input_.empty()) {
        VLOG(1) << "Cannot parse receipt data from PKCS7 container";
        return nullptr;
      }

      bssl::der::Parser set_parser;
      bssl::der::Parser octet_parser(receipt_input_);

      if (!octet_parser.ReadConstructed(CBS_ASN1_SET, &set_parser)) {
        VLOG(1) << "Cannot parse receipt PKCS7 container's ASN1_SET";
        return nullptr;
      }

      while (set_parser.HasMore()) {
        bssl::der::Parser sequence_parser;
        if (!set_parser.ReadSequence(&sequence_parser)) {
          VLOG(1)
              << "Cannot parse receipt PKCS7 container's ASN1_SET -> Sequence";
          break;
        }

        std::uint64_t attribute_type = 0;
        if (!sequence_parser.ReadUint64(&attribute_type)) {
          return nullptr;
        }

        std::uint64_t unknown = 0;
        if (!sequence_parser.ReadUint64(&unknown)) {
          return nullptr;
        }

        bssl::der::Input value;
        if (!sequence_parser.ReadTag(CBS_ASN1_OCTETSTRING, &value)) {
          return nullptr;
        }

        switch (attribute_type) {
          case 2: {
            _bundleId = brave::storekit_receipt::decode_asn1_string(value);
          } break;

          case 3: {
            _appVersion = brave::storekit_receipt::decode_asn1_string(value);
          } break;

          case 4: {
            _opaqueData = [NSData
                dataWithBytes:value.data()
                       length:value.size()];  // Data used to compute SHA1-Hash
          } break;

          case 5: {
            _sha1Hash = base::SysUTF8ToNSString(
                base::HexEncode(value.data(), value.size()));
          } break;

          case 12: {
            _receiptCreationDate =
                brave::storekit_receipt::decode_asn1_date(value);
          } break;

          case 17: {
            BraveStoreKitPurchase* purchase =
                [[BraveStoreKitPurchase alloc] initWithData:value];
            if (purchase) {
              NSMutableArray* purchases =
                  static_cast<NSMutableArray*>(_inAppPurchaseReceipts);
              [purchases addObject:purchase];
            }
          } break;

          case 19: {
            _originalApplicationVersion =
                brave::storekit_receipt::decode_asn1_string(value);
          } break;

          case 21: {
            _receiptExpirationDate =
                brave::storekit_receipt::decode_asn1_date(value);
          } break;

          default:
            break;
        }
      }
    }
  }
  return self;
}
@end
