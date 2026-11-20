// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/saved_tab_groups/tab_group_id.h"

#include "base/token.h"
#import "brave/ios/browser/saved_tab_groups/brave_tab_group_id.h"
#include "ios/web/public/web_state_id.h"

namespace brave {
namespace {

uint64_t ReadBigEndian(base::span<const uint8_t, 8> bytes) {
  uint64_t value = 0;
  for (uint8_t byte : bytes) {
    value = (value << 8) | byte;
  }
  return value;
}

void WriteBigEndian(uint64_t value, base::span<uint8_t, 8> bytes) {
  for (size_t i = bytes.size(); i > 0; --i) {
    bytes[i - 1] = static_cast<uint8_t>(value & 0xff);
    value >>= 8;
  }
}

}  // namespace

tab_groups::TabGroupId TabGroupIdFromUuidBytes(
    base::span<const uint8_t, 16> bytes) {
  return tab_groups::TabGroupId::FromRawToken(base::Token(
      ReadBigEndian(bytes.first<8>()), ReadBigEndian(bytes.last<8>())));
}

void UuidBytesFromTabGroupId(const tab_groups::TabGroupId& group_id,
                             base::span<uint8_t, 16> bytes) {
  WriteBigEndian(group_id.token().high(), bytes.first<8>());
  WriteBigEndian(group_id.token().low(), bytes.last<8>());
}

int32_t MakeSyncTabID() {
  return web::WebStateID::NewUnique().identifier();
}

}  // namespace brave

@implementation BraveTabGroupID

+ (int32_t)makeSyncTabID {
  return brave::MakeSyncTabID();
}

+ (BOOL)tokenForGroupID:(NSUUID*)groupID
                   high:(uint64_t*)high
                    low:(uint64_t*)low {
  if (!groupID || !high || !low) {
    return NO;
  }
  uint8_t bytes[16];
  [groupID getUUIDBytes:bytes];
  const tab_groups::TabGroupId group_id =
      brave::TabGroupIdFromUuidBytes(base::span<const uint8_t, 16>(bytes));
  *high = group_id.token().high();
  *low = group_id.token().low();
  return YES;
}

+ (NSUUID*)groupIDForTokenHigh:(uint64_t)high low:(uint64_t)low {
  uint8_t bytes[16];
  const tab_groups::TabGroupId group_id =
      tab_groups::TabGroupId::FromRawToken(base::Token(high, low));
  brave::UuidBytesFromTabGroupId(group_id, base::span<uint8_t, 16>(bytes));
  return [[NSUUID alloc] initWithUUIDBytes:bytes];
}

@end
