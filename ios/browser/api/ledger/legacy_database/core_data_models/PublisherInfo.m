/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "PublisherInfo.h"

@implementation PublisherInfo

+ (NSFetchRequest<PublisherInfo*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"PublisherInfo"];
}

@dynamic excluded;
@dynamic faviconURL;
@dynamic name;
@dynamic provider;
@dynamic publisherID;
@dynamic url;
@dynamic activities;
@dynamic contributions;
@dynamic recurringDonations;
@dynamic pendingContributions;

@end
