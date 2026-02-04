/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/ui/l10n_utils_typed_bridge.h"

@implementation L10nUtils (Typed)

+ (NSString*)stringForMessageIDTyped:(MessageIDTyped)messageID {
  return [self stringForMessageID:(MessageID)messageID];
}

+ (NSString*)stringWithFixupForMessageIDTyped:(MessageIDTyped)messageID {
  return [self stringWithFixupForMessageID:(MessageID)messageID];
}

+ (NSString*)formatStringForMessageIDTyped:(MessageIDTyped)messageID
                                  argument:(NSString*)argument {
  return [self formatStringForMessageID:(MessageID)messageID argument:argument];
}

+ (NSString*)pluralStringForMessageIDTyped:(MessageIDTyped)messageID
                                    number:(NSInteger)number {
  return [self pluralStringForMessageID:(MessageID)messageID number:number];
}

@end
