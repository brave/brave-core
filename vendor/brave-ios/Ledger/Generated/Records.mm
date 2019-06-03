/* WARNING: THIS FILE IS GENERATED. ANY CHANGES TO THIS FILE WILL BE OVERWRITTEN
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "Records.h"
#import "Records+Private.h"
#import "CppTransformations.h"

#import <vector>
#import <map>
#import <string>

@implementation BATAutoContributeProps
- (instancetype)initWithAutoContributeProps:(const ledger::AutoContributeProps&)obj {
  if ((self = [super init])) {
    self.enabledContribute = obj.enabled_contribute;
    self.contributionMinTime = obj.contribution_min_time;
    self.contributionMinVisits = obj.contribution_min_visits;
    self.contributionNonVerified = obj.contribution_non_verified;
    self.contributionVideos = obj.contribution_videos;
    self.reconcileStamp = obj.reconcile_stamp;
  }
  return self;
}
@end

@implementation BATBalanceReportInfo
- (instancetype)initWithBalanceReportInfo:(const ledger::BalanceReportInfo&)obj {
  if ((self = [super init])) {
    self.openingBalance = [NSString stringWithUTF8String:obj.opening_balance_.c_str()];
    self.closingBalance = [NSString stringWithUTF8String:obj.closing_balance_.c_str()];
    self.deposits = [NSString stringWithUTF8String:obj.deposits_.c_str()];
    self.grants = [NSString stringWithUTF8String:obj.grants_.c_str()];
    self.earningFromAds = [NSString stringWithUTF8String:obj.earning_from_ads_.c_str()];
    self.autoContribute = [NSString stringWithUTF8String:obj.auto_contribute_.c_str()];
    self.recurringDonation = [NSString stringWithUTF8String:obj.recurring_donation_.c_str()];
    self.oneTimeDonation = [NSString stringWithUTF8String:obj.one_time_donation_.c_str()];
    self.total = [NSString stringWithUTF8String:obj.total_.c_str()];
  }
  return self;
}
@end

@implementation BATContributionInfo
- (instancetype)initWithContributionInfo:(const ledger::ContributionInfo&)obj {
  if ((self = [super init])) {
    self.publisher = [NSString stringWithUTF8String:obj.publisher.c_str()];
    self.value = obj.value;
    self.date = obj.date;
  }
  return self;
}
@end

@implementation BATGrant
- (instancetype)initWithGrant:(const ledger::Grant&)obj {
  if ((self = [super init])) {
    self.altcurrency = [NSString stringWithUTF8String:obj.altcurrency.c_str()];
    self.probi = [NSString stringWithUTF8String:obj.probi.c_str()];
    self.promotionId = [NSString stringWithUTF8String:obj.promotionId.c_str()];
    self.expiryTime = obj.expiryTime;
    self.type = [NSString stringWithUTF8String:obj.type.c_str()];
  }
  return self;
}
@end

@implementation BATPublisherBanner
- (instancetype)initWithPublisherBanner:(const ledger::PublisherBanner&)obj {
  if ((self = [super init])) {
    self.publisherKey = [NSString stringWithUTF8String:obj.publisher_key.c_str()];
    self.title = [NSString stringWithUTF8String:obj.title.c_str()];
    self.name = [NSString stringWithUTF8String:obj.name.c_str()];
    self.description = [NSString stringWithUTF8String:obj.description.c_str()];
    self.background = [NSString stringWithUTF8String:obj.background.c_str()];
    self.logo = [NSString stringWithUTF8String:obj.logo.c_str()];
    self.amounts = NSArrayFromVector(obj.amounts);
    self.provider = [NSString stringWithUTF8String:obj.provider.c_str()];
    self.social = NSDictionaryFromMap(obj.social);
    self.verified = obj.verified;
  }
  return self;
}
@end

@implementation BATReconcileInfo
- (instancetype)initWithReconcileInfo:(const ledger::ReconcileInfo&)obj {
  if ((self = [super init])) {
    self.viewingid = [NSString stringWithUTF8String:obj.viewingId_.c_str()];
    self.amount = [NSString stringWithUTF8String:obj.amount_.c_str()];
    self.retryStep = (BATContributionRetry)obj.retry_step_;
    self.retryLevel = obj.retry_level_;
  }
  return self;
}
@end

@implementation BATRewardsInternalsInfo
- (instancetype)initWithRewardsInternalsInfo:(const ledger::RewardsInternalsInfo&)obj {
  if ((self = [super init])) {
    self.paymentId = [NSString stringWithUTF8String:obj.payment_id.c_str()];
    self.isKeyInfoSeedValid = obj.is_key_info_seed_valid;
    self.personaId = [NSString stringWithUTF8String:obj.persona_id.c_str()];
    self.userId = [NSString stringWithUTF8String:obj.user_id.c_str()];
    self.bootStamp = obj.boot_stamp;
    self.currentReconciles = NSDictionaryFromMap(obj.current_reconciles, ^BATReconcileInfo *(ledger::ReconcileInfo o){ return [[BATReconcileInfo alloc] initWithReconcileInfo:o]; });
  }
  return self;
}
@end

@implementation BATTransactionInfo
- (instancetype)initWithTransactionInfo:(const ledger::TransactionInfo&)obj {
  if ((self = [super init])) {
    self.timestampInSeconds = obj.timestamp_in_seconds;
    self.estimatedRedemptionValue = obj.estimated_redemption_value;
    self.confirmationType = [NSString stringWithUTF8String:obj.confirmation_type.c_str()];
  }
  return self;
}
@end

@implementation BATTransactionsInfo
- (instancetype)initWithTransactionsInfo:(const ledger::TransactionsInfo&)obj {
  if ((self = [super init])) {
    self.transactions = NSArrayFromVector(obj.transactions, ^BATTransactionInfo *(const ledger::TransactionInfo& o){ return [[BATTransactionInfo alloc] initWithTransactionInfo: o]; });
  }
  return self;
}
@end

@implementation BATTwitchEventInfo
- (instancetype)initWithTwitchEventInfo:(const ledger::TwitchEventInfo&)obj {
  if ((self = [super init])) {
    self.event = [NSString stringWithUTF8String:obj.event_.c_str()];
    self.time = [NSString stringWithUTF8String:obj.time_.c_str()];
    self.status = [NSString stringWithUTF8String:obj.status_.c_str()];
  }
  return self;
}
@end

@implementation BATVisitData
- (instancetype)initWithVisitData:(const ledger::VisitData&)obj {
  if ((self = [super init])) {
    self.tld = [NSString stringWithUTF8String:obj.tld.c_str()];
    self.domain = [NSString stringWithUTF8String:obj.domain.c_str()];
    self.path = [NSString stringWithUTF8String:obj.path.c_str()];
    self.tabId = obj.tab_id;
    self.name = [NSString stringWithUTF8String:obj.name.c_str()];
    self.url = [NSString stringWithUTF8String:obj.url.c_str()];
    self.provider = [NSString stringWithUTF8String:obj.provider.c_str()];
    self.faviconUrl = [NSString stringWithUTF8String:obj.favicon_url.c_str()];
  }
  return self;
}
@end

@implementation BATWalletInfo
- (instancetype)initWithWalletInfo:(const ledger::WalletInfo&)obj {
  if ((self = [super init])) {
    self.altcurrency = [NSString stringWithUTF8String:obj.altcurrency_.c_str()];
    self.probi = [NSString stringWithUTF8String:obj.probi_.c_str()];
    self.balance = obj.balance_;
    self.feeAmount = obj.fee_amount_;
    self.rates = NSDictionaryFromMap(obj.rates_);
    self.parametersChoices = NSArrayFromVector(obj.parameters_choices_);
    self.parametersRange = NSArrayFromVector(obj.parameters_range_);
    self.parametersDays = obj.parameters_days_;
    self.grants = NSArrayFromVector(obj.grants_, ^BATGrant *(const ledger::Grant& o){ return [[BATGrant alloc] initWithGrant: o]; });
  }
  return self;
}
@end
