/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "NSURL+Extensions.h"

@implementation NSURL (Extensions)

- (NSString *)bat_normalizedHost
{
  const auto host = self.host;
  const auto range = [host rangeOfString:@"^(www|mobile|m)\\." options:NSRegularExpressionSearch];
  if (range.length > 0) {
    return [host stringByReplacingCharactersInRange:range withString:@""];
  }
  return host;
}

@end
