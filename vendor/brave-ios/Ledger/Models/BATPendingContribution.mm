// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATPendingContribution.h"
#import "Records.h"
#import "Enums.h"
#import "bat/ledger/pending_contribution.h"

@implementation BATPendingContribution
- (instancetype)initWithPendingContribution:(const ledger::PendingContribution&)obj {
  if ((self = [super init])) {
    self.publisherKey = [NSString stringWithUTF8String:obj.publisher_key.c_str()];
    self.amount = obj.amount;
    self.addedDate = obj.added_date;
    self.viewingId = [NSString stringWithUTF8String:obj.viewing_id.c_str()];
    self.category = (BATRewardsCategory)obj.category;
  }
  return self;
}
@end

