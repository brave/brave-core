// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATPromotionSolution.h"

@implementation BATPromotionSolution

- (NSString *)JSONPayload
{
  NSDictionary *payload = @{
    @"nonce": self.noonce,
    @"blob": self.blob,
    @"signature": self.signature
  };
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
  if (!jsonData) {
    NSLog(@"Missing JSON payload while attempting to attest promotion");
    return @"";
  }
  return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

@end
