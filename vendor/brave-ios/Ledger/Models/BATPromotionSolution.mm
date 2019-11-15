// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATPromotionSolution.h"

#import "ledger.mojom.objc.h"
#import "RewardsLogStream.h"
#define BLOG(__severity) RewardsLogStream(__FILE__, __LINE__, __severity).stream()

@implementation BATPromotionSolution

- (NSString *)JSONPayload
{
  NSDictionary *payload = @{
    @"nonce": self.nonce,
    @"blob": self.blob,
    @"signature": self.signature
  };
  NSData *jsonData = [NSJSONSerialization dataWithJSONObject:payload options:0 error:nil];
  if (!jsonData) {
    BLOG(ledger::LogLevel::LOG_WARNING) << "Missing JSON payload while attempting to attest promotion" << std::endl;
    return @"";
  }
  return [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
}

@end
