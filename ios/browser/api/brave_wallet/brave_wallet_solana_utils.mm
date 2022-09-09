/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/brave_wallet/brave_wallet_solana_utils.h"

#include <string>
#include <vector>
#include "base/strings/sys_string_conversions.h"
#include "brave/components/brave_wallet/common/solana_utils.h"

@implementation NSData (Base58)

/// Initializes a Data object with the given Base58 encoded string.
+ (nullable instancetype)dataWithBase58EncodedString:(NSString*)base58String {
  return [self dataWithBase58EncodedString:base58String
                                   options:NSDataBase58DecodingOptionsNone];
}

+ (nullable instancetype)
    dataWithBase58EncodedString:(NSString*)base58String
                        options:(NSDataBase58DecodingOptions)options {
  std::vector<std::uint8_t> result;
  const bool isStrict = options & NSDataBase58DecodingOptionsStrict;
  if (!brave_wallet::Base58Decode(
          base::SysNSStringToUTF8(base58String), &result,
          static_cast<int>(base58String.length), isStrict)) {
    return nil;
  }
  return [NSData dataWithBytes:static_cast<const void*>(result.data())
                        length:static_cast<NSUInteger>(result.size())];
}

/// Returns a Base 58 encoded string representation of the current Data object
- (NSString*)base58EncodedString {
  std::vector<std::uint8_t> bytes(self.length);
  [self getBytes:bytes.data() length:self.length];
  const auto base58String = brave_wallet::Base58Encode(bytes);
  return base::SysUTF8ToNSString(base58String);
}

@end

@implementation NSString (Base58)

- (bool)isBase58EncodedSolanaPubkey {
  return brave_wallet::IsBase58EncodedSolanaPubkey(
      base::SysNSStringToUTF8(self));
}

@end
