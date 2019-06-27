/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "BATPublisherInfo+Private.h"

@implementation BATPublisherInfo (PrivateCpp)

- (ledger::PublisherInfoPtr)cppObj
{
  auto info = ledger::PublisherInfo::New();
  info->id = std::string(self.id.UTF8String);
  info->duration = self.duration;
  info->score = self.score;
  info->visits = self.visits;
  info->percent = self.percent;
  info->weight = self.weight;
  info->excluded = (ledger::PUBLISHER_EXCLUDE)self.excluded;
  info->category = (ledger::REWARDS_CATEGORY)self.category;
  info->reconcile_stamp = self.reconcileStamp;
  info->verified = self.verified;
  info->name = std::string(self.name.UTF8String);
  info->url = std::string(self.url.UTF8String);
  info->provider = std::string(self.provider.UTF8String);
  info->favicon_url = std::string(self.faviconUrl.UTF8String);
  return info;
}

@end

@implementation BATPendingContributionInfo (PrivateCpp)

- (ledger::PendingContributionInfoPtr)cppObj
{
  auto info = ledger::PendingContributionInfo::New();
  info->publisher_key = std::string(self.publisherKey.UTF8String);
  info->viewing_id = std::string(self.viewingId.UTF8String);
  info->category = self.category;
  info->name = std::string(self.name.UTF8String);
  info->url = std::string(self.url.UTF8String);
  info->favicon_url = std::string(self.faviconUrl.UTF8String);
  info->amount = self.amount;
  info->provider = std::string(self.provider.UTF8String);
  info->verified = self.verified;
  info->added_date = self.addedDate;
  info->expiration_date = self.expirationDate;
  return info;
}

@end
