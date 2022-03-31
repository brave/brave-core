/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "ContributionInfo.h"

@implementation ContributionInfo

+ (NSFetchRequest<ContributionInfo*>*)fetchRequest {
  return [NSFetchRequest fetchRequestWithEntityName:@"ContributionInfo"];
}

@dynamic type;
@dynamic date;
@dynamic month;
@dynamic probi;
@dynamic publisherID;
@dynamic year;
@dynamic publisher;

@end
