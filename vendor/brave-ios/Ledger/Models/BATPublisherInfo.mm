// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#import "BATPublisherInfo.h"
#import "Records+Private.h"
#import "CppTransformations.h"
#import "bat/ledger/publisher_info.h"

@implementation BATPublisherInfo
- (instancetype)initWithPublisherInfo:(const ledger::PublisherInfo&)obj {
  if ((self = [super init])) {
    self.id = [NSString stringWithUTF8String:obj.id.c_str()];
    self.duration = obj.duration;
    self.score = obj.score;
    self.visits = obj.visits;
    self.percent = obj.percent;
    self.weight = obj.weight;
    self.excluded = (BATPublisherExclude)obj.excluded;
    self.category = (BATRewardsCategory)obj.category;
    self.reconcileStamp = obj.reconcile_stamp;
    self.verified = obj.verified;
    self.name = [NSString stringWithUTF8String:obj.name.c_str()];
    self.url = [NSString stringWithUTF8String:obj.url.c_str()];
    self.provider = [NSString stringWithUTF8String:obj.provider.c_str()];
    self.faviconUrl = [NSString stringWithUTF8String:obj.favicon_url.c_str()];
//    self.contributions = NSArrayFromVector(obj.contributions, ^BATContributionInfo *(const ledger::mojom::ContributionInfoPtr& o){
//      return [[BATContributionInfo alloc] initWithContributionInfo:*o];
//    });
  }
  return self;
}
@end
