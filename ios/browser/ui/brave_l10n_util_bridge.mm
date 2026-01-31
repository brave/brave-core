/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/ui/brave_l10n_util_bridge.h"

#import "base/strings/sys_string_conversions.h"
#import "ui/base/l10n/l10n_util_mac.h"

@implementation BraveL10nUtils

+ (NSString*)stringForBraveMessageID:(BraveMessageID)messageID {
  return l10n_util::GetNSString(messageID);
}

+ (NSString*)stringWithFixupForBraveMessageID:(BraveMessageID)messageID {
  return l10n_util::GetNSStringWithFixup(messageID);
}

+ (NSString*)formatStringForBraveMessageID:(BraveMessageID)messageID
                                  argument:(NSString*)argument {
  return l10n_util::GetNSStringF(messageID, base::SysNSStringToUTF16(argument));
}

+ (NSString*)pluralStringForBraveMessageID:(BraveMessageID)messageID
                                    number:(NSInteger)number {
  int numberInt = static_cast<int>(number);
  return l10n_util::GetPluralNSStringF(messageID, numberInt);
}

@end
